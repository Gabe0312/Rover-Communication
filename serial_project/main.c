#define _POSIX_C_SOURCE 200809L // defines the POSIX version, most recent, to implement clock_gettime() function for millis() implementation
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <termio.h>

int serial_fd = -1; // file descriptor for the serial port, initialized to -1 to indicate it's not open

enum RoverState{
  IDLE,//0
  TELEOP, //1
  AUTO//2
}; // think of enum as describing ints with names EX: RED = 0

int currentState = IDLE;// the enum object creation, system will start in IDLE state in the setup() stage
unsigned long stateStartTime = 0;
unsigned long lastPrintTime = 0;

const unsigned long printInterval = 2500; // how often to print the remaining time in milliseconds, in this case every second

// writing functions because the functions must be above the setup and loop

unsigned long millis(){

  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

void changeState(enum RoverState newState){ // newState is just another instance for the rover state in function that is used in this function to change the state
  currentState = newState;
  stateStartTime = millis();
  lastPrintTime = millis(); // millis returns the number of milliseconds since the board began running the current program, stores it in stateStartTime so we know when state was entered/started so we can do countdowns based on that time.

  switch (currentState){
    case IDLE:
      printf("IDLE\n");
      break;
    case TELEOP:

      break;
    case AUTO:

      break;
  }
} 

void setup() {

  serial_fd = open("/dev/pts/2", O_RDWR | O_NOCTTY | O_NDELAY); // opens the serial port for reading and writing, non-blocking, and no controlling terminal
  if (serial_fd == -1){
    printf("Error: Could not open virtual serial port. \n");
  } else {
    printf("Successfully connected to virtual serial port!\n");
  }
  
}

void loop() {
  unsigned long now = millis(); //how long the program has been running minus the time when the state started which atp is 0 with stateStartTime updated every time the state changes.
  
  unsigned char buffer[6];

  int bytes_read = read(serial_fd,buffer,6);

  if (bytes_read == 6){
    if(buffer[0] == 0xA8 && (buffer[5] & 0x1F) == 0x15){
      printf("Valid Packet Received!\n");
      printf("Left Joystick Y: %d\n", buffer[2]);

      changeState(TELEOP);
    }
  }
  switch(currentState){
    case IDLE:
      if (now - lastPrintTime >= printInterval)
          lastPrintTime = now;
      break;

    case TELEOP:
      break;

    case AUTO:
      break;

  }
}

int main(void){
  setup();
  while(1){loop();}
}