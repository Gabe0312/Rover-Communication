#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define BRIDGE_ADDR "127.0.0.1"
#define BRIDGE_PORT 9091
#define JSON_BUF_SIZE 1024
#define SERIAL_DEVICE "/dev/ttyACM0"

int serial_fd = -1;
int bridge_listen_fd = -1;
int bridge_client_fd = -1;

enum RoverState {
  IDLE,
  TELEOP,
  AUTO
};

int currentState = IDLE;
unsigned long stateStartTime = 0;
unsigned long lastPrintTime = 0;
const unsigned long printInterval = 2500;

unsigned long millis() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (unsigned long)(ts.tv_sec * 1000) + (unsigned long)(ts.tv_nsec / 1000000);
}

void changeState(enum RoverState newState) {
  currentState = newState;
  stateStartTime = millis();
  lastPrintTime = millis();

  switch (currentState) {
    case IDLE:
      printf("IDLE\n");
      break;
    case TELEOP:
      printf("TELEOP\n");
      break;
    case AUTO:
      printf("AUTO\n");
      break;
  }
}

void configure_serial_9600(int fd) {
  struct termios tty;
  if (tcgetattr(fd, &tty) != 0) {
    perror("tcgetattr");
    return;
  }

  cfsetospeed(&tty, B9600);
  cfsetispeed(&tty, B9600);

  tty.c_cflag &= (tcflag_t)~PARENB;
  tty.c_cflag &= (tcflag_t)~CSTOPB;
  tty.c_cflag &= (tcflag_t)~CSIZE;
  tty.c_cflag |= CS8;
  tty.c_cflag |= CREAD | CLOCAL;

  tty.c_lflag &= (tcflag_t)~(ICANON | ECHO | ECHOE | ISIG);
  tty.c_iflag &= (tcflag_t)~(IXON | IXOFF | IXANY);
  tty.c_oflag &= (tcflag_t)~OPOST;

  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 1;

  if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    perror("tcsetattr");
  }
}

void setup_serial_output() {
  serial_fd = open(SERIAL_DEVICE, O_RDWR | O_NOCTTY | O_SYNC);
  if (serial_fd == -1) {
    printf("Warning: Could not open serial device %s (%s)\n", SERIAL_DEVICE, strerror(errno));
    printf("Continuing in debug mode without serial writes.\n");
    return;
  }

  configure_serial_9600(serial_fd);
  printf("Connected to serial device %s\n", SERIAL_DEVICE);
}

void setup_bridge_socket() {
  bridge_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (bridge_listen_fd < 0) {
    perror("socket");
    return;
  }

  int opt = 1;
  if (setsockopt(bridge_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt");
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(BRIDGE_PORT);
  if (inet_pton(AF_INET, BRIDGE_ADDR, &addr.sin_addr) <= 0) {
    perror("inet_pton");
    return;
  }

  if (bind(bridge_listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return;
  }

  if (listen(bridge_listen_fd, 1) < 0) {
    perror("listen");
    return;
  }

  printf("Waiting for Go bridge at %s:%d ...\n", BRIDGE_ADDR, BRIDGE_PORT);
}

int accept_bridge_client() {
  if (bridge_listen_fd < 0) {
    return -1;
  }

  bridge_client_fd = accept(bridge_listen_fd, NULL, NULL);
  if (bridge_client_fd < 0) {
    perror("accept");
    return -1;
  }

  printf("Connected to Go bridge\n");
  return 0;
}

int read_json_line(char *buf, size_t cap) {
  size_t i = 0;
  while (i + 1 < cap) {
    char ch;
    ssize_t n = read(bridge_client_fd, &ch, 1);
    if (n <= 0) {
      return -1;
    }
    if (ch == '\n') {
      break;
    }
    buf[i++] = ch;
  }
  buf[i] = '\0';
  return (int)i;
}

int extract_u8(const char *json, const char *key, unsigned char *out) {
  char pattern[32];
  snprintf(pattern, sizeof(pattern), "\"%s\":", key);

  const char *p = strstr(json, pattern);
  if (p == NULL) {
    return 0;
  }

  p += strlen(pattern);
  int value = 0;
  if (sscanf(p, "%d", &value) != 1) {
    return 0;
  }

  if (value < 0) {
    value = 0;
  }
  if (value > 255) {
    value = 255;
  }

  *out = (unsigned char)value;
  return 1;
}

void build_packet(unsigned char ljoyX, unsigned char ljoyY, unsigned char rjoyY,
                  unsigned char rt, unsigned char lb, unsigned char rb,
                  unsigned char north, unsigned char out[6]) {
  out[0] = 0xA8;
  out[1] = ljoyX;
  out[2] = ljoyY;
  out[3] = rjoyY;
  out[4] = rt;
  out[5] = 0x15;

  if (lb != 0) {
    out[5] |= (1u << 5);
  }
  if (rb != 0) {
    out[5] |= (1u << 6);
  }
  if (north != 0) {
    out[5] |= (1u << 7);
  }
}

void setup() {
  setup_serial_output();
  setup_bridge_socket();
  changeState(IDLE);
}

void loop() {
  unsigned long now = millis();

  if (bridge_client_fd < 0) {
    if (accept_bridge_client() != 0) {
      usleep(100000);
      return;
    }
  }

  char jsonBuf[JSON_BUF_SIZE];
  int lineLen = read_json_line(jsonBuf, sizeof(jsonBuf));
  if (lineLen <= 0) {
    printf("Bridge disconnected, waiting for reconnect...\n");
    close(bridge_client_fd);
    bridge_client_fd = -1;
    return;
  }

  unsigned char ljoyX = 128;
  unsigned char ljoyY = 0;
  unsigned char rjoyY = 0;
  unsigned char rt = 0;
  unsigned char lb = 0;
  unsigned char rb = 0;
  unsigned char north = 0;

  extract_u8(jsonBuf, "LjoyX", &ljoyX);
  extract_u8(jsonBuf, "LjoyY", &ljoyY);
  extract_u8(jsonBuf, "RjoyY", &rjoyY);
  extract_u8(jsonBuf, "RT", &rt);
  extract_u8(jsonBuf, "LB", &lb);
  extract_u8(jsonBuf, "RB", &rb);
  extract_u8(jsonBuf, "N", &north);

  unsigned char packet[6];
  build_packet(ljoyX, ljoyY, rjoyY, rt, lb, rb, north, packet);

  if (serial_fd >= 0) {
    ssize_t n = write(serial_fd, packet, sizeof(packet));
    if (n != (ssize_t)sizeof(packet)) {
      printf("Serial write failed (%zd/%zu): %s\n", n, sizeof(packet), strerror(errno));
    }
  }

  if (now - lastPrintTime >= printInterval) {
    printf("JSON: %s\n", jsonBuf);
    printf("Packet: [%02X %02X %02X %02X %02X %02X]\n",
           packet[0], packet[1], packet[2], packet[3], packet[4], packet[5]);
    lastPrintTime = now;
  }

  changeState(TELEOP);
}

int main(void) {
  setup();
  while (1) {
    loop();
  }
}