#include <Arduino.h>

// put function declarations here:
#include <stdint.h>
#include <stdio.h>

enum RoverState{
  IDLE,//0
  TELEOP, //1
  AUTO//2
}; // think of enum as describing ints with names EX: RED = 0

int currentState = IDLE;// the enum object creation, system will start in IDLE state in the setup() stage
unsigned long stateStartTime = 0;
unsigned long lastPrintTime = 0;

const unsigned long printInterval = 2500; // how often to print the remaining time in milliseconds, in this case every second


void changeState(enum RoverState newState){ // newState is just another instance for the rover state in function that is used in this function to change the state
  currentState = newState;
  stateStartTime = millis();
  lastPrintTime = millis(); // millis returns the number of milliseconds since the board began running the current program, stores it in stateStartTime so we know when state was entered/started so we can do countdowns based on that time.

  switch (currentState){
    case IDLE:
      Serial.println("IDLE");
      break;
    case TELEOP:

      break;
    case AUTO:

      break;
  }
} 

void setup() {

  Serial.begin(9600);

  pinMode(13, OUTPUT);

  
  
}

void loop() {
  unsigned long now = millis(); //how long the program has been running minus the time when the state started which atp is 0 with stateStartTime updated every time the state changes.
  
  if (Serial.available() > 0){

    byte peakedByte = Serial.peek();

    if(peakedByte == 0xA8){
      
      if(Serial.available() >= 6){
        
        Serial.read();
        
        byte payload[5];
        Serial.readBytes(payload, 5);


      if ((payload[4] & 0x1F) == 0x15){

        Serial.println("Valid packet received from Pi!");

        digitalWrite(13, HIGH);
        delay(50);
        digitalWrite(13, LOW);

        Serial.print("Raw Data: ");
        Serial.print(payload[0]);
        Serial.print(", ");
        Serial.print(payload[1]);
        Serial.print(", ");
        Serial.print(payload[2]);
        Serial.print(", ");
        Serial.println(payload[3]);
      }
    }
  } else {

    Serial.read();

  }

}
  switch(currentState){
    case IDLE:
      if (now - lastPrintTime >= printInterval){
          lastPrintTime = now;
      }
      break;

    case TELEOP:
      break;

    case AUTO:
      break;

  }
}
