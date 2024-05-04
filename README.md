# Linkit-7697-Arduino-Irrigation-System

  

## Introduction

  

Welcome to a user-friendly Arduino Irrigation System designed with a host of components designed to automate your irrigation process. This system is a testimony of how diverse hardware components can be utilized in conjunction with software services like MQTT server, Line Notify, and a simplistic web user interface to facilitate seamless interactions.  
![WebUI](https://github.com/SAMMYBOOOOM/Linkit-7697-Arduino-Irrigate-System/blob/main/img/Simple_WebUI.png)
![MQTT](https://github.com/SAMMYBOOOOM/Linkit-7697-Arduino-Irrigate-System/blob/main/img/MQTT_server.png)
![Line notify](https://github.com/SAMMYBOOOOM/Linkit-7697-Arduino-Irrigate-System/blob/main/img/Line_Notify.png)
![OLED](https://github.com/SAMMYBOOOOM/Linkit-7697-Arduino-Irrigate-System/blob/main/img/OLED.png)

  

## Component Inventory

  

The system is comprehensively designed with the following components:

  

- A Linkit 7697 board. You can view the board [here](https://www.seeedstudio.com/LinkIt-7697-p-2818.html).
- External button: Acts as a switch for OLED and MQTT display.
- OLED display: Boasts a 128*64 display for showcasing sensor status.
- SI114X: A Sunlight sensor (Visible, IR, UV).
- DHT11: An air humidity and temperature sensor with integer accuracy(Humidity, Temperature in Celsius).
- 3v3/5v relay: Supplies ample power for the water pump.
- An electronic water pump: Facilitates water distribution.
- Soil moisture sensor: Imparts the soil moisture value.

  

## Functional Aspects

  

The system, brimming with diverse functionalities, provides:

  

- MQTT server: Facilitates the reception and transmission of diverse information to the board.
- Line Notify: Serves to inform the user. However, for two-way communication, the Line Messaging API is the go-to option.
- Simple WebUI: Exhibits exemplary functionalities of a local Web control panel.

  

## How to Begin

  

Just abide by the diagram and install all necessary libraries to compile the code (DHT.h, LWiFi.h, etc...).

  

## Usage

  

The system provides a simplified framework that allows users to customize and adapt the irrigation to suit their specific needs.

  

## Conclusion

  

A task accomplished within two days, the project is intended to illustrate the capabilities of the Linkit 7697 board in automating an irrigation system.

  

Your contribution towards improving this system or reporting issues is highly appreciated!

  

## Licensing and Acknowledgments:

  

Though not quite sure of its purpose, feel free to reach out to me for any questions you may have at B11013112@uch.edu.tw. Always ready to assist!
