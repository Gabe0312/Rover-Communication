# Rover Communication System

## Overview
This repository contains the communication and control software for a custom-built rover. The system acts as a multi-stage pipeline, translating joystick inputs from a laptop into physical hardware movements on the rover.

The architecture is divided into two main environments:
1. **The Brain (Raspberry Pi):** Receives Wi-Fi inputs, translates them, and manages the state machine.
2. **The Muscle (Arduino):** Receives raw byte packets via USB serial and executes hardware commands.

## Current Project Status: Phase 1 (Network Verification)
Currently, the system is in the **Network Smoke Test** phase. 
* A virtual bridge (`socat`) is being used to route data internally on the Raspberry Pi between the Go translator and the C state machine.
* The Arduino is running a bouncer script to verify alignment bytes (0xA8) and bitwise payload integrity. 
* Upon successful packet reception, the Arduino flashes the Pin 13 LED and echoes the raw data to the PlatformIO serial monitor. 
* **Next Phase:** Integrating [Insert teammate's name/Eli's] motor control math to replace the LED testing logic.

## Repository Structure
This is a monorepo containing both the Pi and Arduino codebases:

* `/SERIAL_PROJECT/` 
  * `server.go` / `main.go`: The Go server that receives JSON payloads over Wi-Fi, translates them into a byte array, and pushes them to a serial port.
  * `main.c`: The C Gatekeeper. Reads the bytes from the Go server, manages rover states (IDLE, TELEOP), and passes the validated packet to the Arduino over physical USB.
* `/serialCommFromPi/`
  * PlatformIO project containing `main.cpp`. Acts as the hardware receiver, parsing the 6-byte packet and triggering physical components.

## How to Run the Local Test Pipeline

### 1. Build the Virtual Bridge
On the Raspberry Pi, create the internal connection between the Go and C scripts:
```bash
socat -d -d pty,raw,echo=0 pty,raw,echo=0
