#include <Arduino.h>

#include <stdint.h>
#include <stdio.h>

enum RoverState {
  IDLE,
  TELEOP,
  AUTO
};

int currentState = IDLE;
unsigned long stateStartTime = 0;
unsigned long lastPrintTime = 0;

const unsigned long printInterval = 2500;

void changeState(enum RoverState newState) {
  currentState = newState;
  stateStartTime = millis();
  lastPrintTime = millis();

  switch (currentState) {
    case IDLE:
      Serial.println("IDLE");
      break;
    case TELEOP:
      Serial.println("TELEOP");
      break;
    case AUTO:
      Serial.println("AUTO");
      break;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);
}

void loop() {
  unsigned long now = millis();

  if (Serial.available() >= 6) {
    byte startByte = Serial.peek();

    if (startByte == 0xA8) {
      byte packet[6];
      size_t readCount = Serial.readBytes(packet, 6);

      if (readCount == 6 && ((packet[5] & 0x1F) == 0x15)) {
        if (currentState != TELEOP) {
          changeState(TELEOP);
        }

        Serial.println("Valid packet received from Pi!");

        digitalWrite(13, HIGH);
        delay(50);
        digitalWrite(13, LOW);

        Serial.print("Raw Data: ");
        Serial.print(packet[1]);
        Serial.print(", ");
        Serial.print(packet[2]);
        Serial.print(", ");
        Serial.print(packet[3]);
        Serial.print(", ");
        Serial.println(packet[4]);
      }
    } else {
      // Drop one byte and resync to next potential packet boundary.
      Serial.read();
    }
  }

  switch (currentState) {
    case IDLE:
      if (now - lastPrintTime >= printInterval) {
        lastPrintTime = now;
      }
      break;
    case TELEOP:
      break;
    case AUTO:
      break;
  }
}
