# Automatic Pet Feeder Project

## Overview

This project involves creating an automatic pet feeder using Arduino. The feeder dispenses designated portions of food at specified times of the day. Users can configure portion sizes and feeding schedules using buttons, and all relevant data is displayed on an LCD screen. Additionally, the feeder can be remotely controlled via a WiFi module, such as from a smartphone.

## Project Components

- **Arduino Uno**: The core of the system, equipped with an AVR ATmega328 microcontroller, powered by a 12V jack power supply.

- **Micro Servo SG-90**: Responsible for opening and closing the feeder.

- **Power Supply (12V)**: Powers the Arduino Uno.

- **LCD 2x16 Display**: Displays information such as portion quantities and feeding times.

- **Tactile Switches (6x6mm)**: Buttons for setting portion sizes and feeding times.

- **Real-Time Clock Module (RTC DS1302)**: Provides precise time readings.

- **WiFi Module (ESP-12E ESP8266)**: Enables remote communication with the Arduino.

- **Voltage Regulator (LDO 3.3V)**: Stabilizes voltage for the WiFi module.

- **Potentiometer (10kΩ)**: Adjusts the brightness of the LCD display.

## Current Code

The Arduino code is structured into several classes, including `FeedTime` for representing feeding times, `Display` for managing display functionality, `MainDisplay` for the main clock display, and `FeedingDisplay` for handling feeding-related displays. The code also utilizes the `LiquidCrystal` library for LCD interfacing and the `RtcDS1302` library for real-time clock functionality.

The main functionalities include setting up buttons, LCD, real-time clock, and feeding times during the setup phase. The `loop` function continuously handles button inputs and updates the display accordingly. The project supports multiple feeding times, and users can edit these times through the interface.

## Instructions

1. **Setting Up Arduino**: Connect the components as specified in the code.
2. **Uploading Code**: Upload the provided Arduino code to the Arduino Uno.
3. **Configuring Feeding Times**: Use the buttons to set feeding times and portion sizes.
4. **Remote Control (Optional)**: Connect the Arduino to a WiFi network using the ESP8266 module for remote control.

Feel free to customize the code according to your specific requirements and expand the functionalities of the automatic pet feeder. Happy coding!
