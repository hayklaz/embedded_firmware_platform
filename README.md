# Embedded Firmware Platform

A PIC32/uC32 embedded firmware project that demonstrates register-level peripheral programming, protocol-based serial communication, and non-volatile memory control. This project was built from my ECE-121 microcontroller lab work and organized into a cleaner GitHub-ready structure.

## Overview

This project implements an embedded firmware platform using the Digilent uC32 development board with the PIC32MX340F512H microcontroller. The firmware combines UART communication, packet-based protocol handling, I2C communication, and EEPROM-based non-volatile memory operations.

The goal of this project is to show practical embedded systems skills such as low-level driver development, hardware communication protocols, interrupt-driven design, and modular firmware organization.

## Features

- Interrupt-driven UART communication
- Packet-based serial protocol layer
- I2C communication with external EEPROM
- Non-volatile memory byte read/write support
- Non-volatile memory page read/write support
- Modular `src` and `include` folder structure
- MPLAB X project support
- PIC32/uC32 hardware integration

## Hardware Used

- Digilent uC32 development board
- PIC32MX340F512H microcontroller
- Digilent Basic I/O Shield
- 24LC256-compatible I2C EEPROM
- USB serial connection for communication/debugging

## Tools Used

- MPLAB X IDE
- XC32 Compiler
- PICkit 3
- CoolTerm or another serial terminal
- Git/GitHub

## Project Structure

```text
Embedded_firmware_platform/
├── README.md
├── src/
│   ├── BOARD.c
│   ├── I2C.c
│   ├── Main.c
│   ├── NonVolatileMemory.c
│   ├── Protocol2.c
│   └── Uart.c
├── include/
│   ├── BOARD.h
│   ├── I2C.h
│   ├── MessageIDs.h
│   ├── NonVolatileMemory.h
│   ├── Protocol2.h
│   └── Uart.h
└── Embedded_firmware_platform.X/
    ├── Makefile
    ├── build/
    ├── debug/
    ├── dist/
    └── nbproject/
