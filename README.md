# Linkit-7697-Arduino-Irrigation-System

  

## Introduction

  

Welcome to a user-friendly Arduino Irrigation System designed with a host of components designed to automate your irrigation process. This system is a testimony of how diverse hardware components can be utilized in conjunction with software services like MQTT server, Line Notify, and a simplistic web user interface to facilitate seamless interactions.  
![WebUI](https://github.com/SAMMYBOOOOM/Linkit-7697-Arduino-Irrigate-System/blob/main/img/Simple_WebUI.png)
![MQTT](https://github.com/SAMMYBOOOOM/Linkit-7697-Arduino-Irrigate-System/blob/main/img/MQTT_server.png)
![Line notify](https://github.com/SAMMYBOOOOM/Linkit-7697-Arduino-Irrigate-System/blob/main/img/Line_Notify.png)
![OLED](https://github.com/SAMMYBOOOOM/Linkit-7697-Arduino-Irrigate-System/blob/main/img/OLED.png)

  
## How to use
1. **Environment Preparation:** Before diving into the project, ensure you have the right tools and software. In case of any issues with setting up the environment, I highly recommend downloading and extracting the ![Arduino hourly build.zip](https://github.com/SAMMYBOOOOM/Linkit-7697-Arduino-Irrigate-System/raw/main/ArduinoAndLibrary/Arduino%20hourly%20build.zip?download=) and ![Arduino15.zip](https://github.com/SAMMYBOOOOM/Linkit-7697-Arduino-Irrigate-System/raw/main/ArduinoAndLibrary/Arduino15.zip?download=). These contain essential files to jumpstart your project.

Unzip the **Arduino15.zip** then paste it at **C:\Users\username\AppData\Local\Arduino15**

Instruction video:
https://youtu.be/stdLgHaFlJg

2. **Library Installation:** For the system to function correctly, add the necessary libraries to your Arduino IDE. You can import them directly via **Sketch > Include Library > Add .ZIP Library…**. Here are the libraries your project needs:
- [DHT sensor](https://github.com/adafruit/DHT-sensor-library) for air humidity and temperature measurements.
- [SI114X sunlight sensor](https://github.com/Seeed-Studio/Grove_Sunlight_Sensor) to monitor visible, IR, and UV light.
- [NTP client](https://github.com/arduino-libraries/NTPClient) for accurate timekeeping.
- [Pubsubclient 2.6](https://www.arduino.cc/reference/en/libraries/pubsubclient/) to handle message queuing telemetry transport (MQTT) communication.
Component Assembly: Follow the comprehensive guide provided here to connect all the hardware components correctly. This step is crucial for the smooth operation of your irrigation system.


## Component Inventory

  

The system is comprehensively designed with the following components:

  

- A Linkit 7697 board. You can view the board [here](https://www.seeedstudio.com/LinkIt-7697-p-2818.html).
1. External button: Acts as a switch for OLED and MQTT display.
2. OLED display: Boasts a 128*64 display for showcasing sensor status.
3. SI114X: A Sunlight sensor (Visible, IR, UV).
4. DHT11: An air humidity and temperature sensor with integer accuracy(Humidity, Temperature in Celsius).
5. Soil moisture sensor: Imparts the soil moisture value.
6. Relay: Control the water pump.
7. 3v3/5v Power supply: Supplies ample power for the water pump.
8. An electronic water pump: Facilitates water distribution.
   
Visit [here](https://github.com/SAMMYBOOOOM/Linkit-7697-Arduino-Irrigate-System/tree/main/Components_link) for more detail on how to link each components


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
