# Introduction
The ESP32 Data Logger is a cost efficient data acquisition system that supports vibrating wire sensors and other sensors with RS-485, TTL protocol. To set up the data logger as an end user, you can plug it into your computer, and configure the logger in a website using the browser. For advanced configurations, you can program your own code and flash the logger on PC, using PlatformIO or Arduino IDE.
- [Introduction](#introduction)
- [Development Environment](#development-environment)
  - [System Log](#system-log)
  - [ESP-Prog](#esp-prog)
  - [Settings to update in Dependencies](#settings-to-update-in-dependencies)
    - [ElegantOTA](#elegantota)
    - [FTP Server SD Card Settings](#ftp-server-sd-card-settings)
- [Architecture](#architecture)
  - [Power Supply](#power-supply)
    - [Power Consumption per Mode](#power-consumption-per-mode)
    - [Solar Panel Configuration](#solar-panel-configuration)
  - [Time](#time)
    - [NTP Server](#ntp-server)
    - [External RTC Setup](#external-rtc-setup)
  - [File System](#file-system)
    - [Flash Memory Partition](#flash-memory-partition)
    - [SD Card Setup](#sd-card-setup)
    - [Database for Timeseries Data](#database-for-timeseries-data)
  - [Internet Access](#internet-access)
    - [WiFi Reconnect Capability](#wifi-reconnect-capability)
    - [WiFi Manager](#wifi-manager)
    - [Dynamic IP Address](#dynamic-ip-address)
  - [Radio Network](#radio-network)
    - [How do devices interconnect?](#how-do-devices-interconnect)
    - [Hardware](#hardware)
  - [Data Logging Functions](#data-logging-functions)
    - [GPIO Pin Monitor](#gpio-pin-monitor)
    - [Sensor Type Supported](#sensor-type-supported)
      - [VM501](#vm501)
  - [OTA](#ota)
- [API](#api)
  - [Logger System Control](#logger-system-control)
    - [Logger Configuration](#logger-configuration)
    - [ESP-NOW Routing Map](#esp-now-routing-map)
    - [File system TODO](#file-system-todo)
  - [Data Retrieval TODO](#data-retrieval-todo)
    - [Timeseries request](#timeseries-request)
- [License](#license)
- [Useful References](#useful-references)
  
# Development Environment
## System Log
Use this guide: https://community.platformio.org/t/redirect-esp32-log-messages-to-sd-card/33734, update `esp32-hal-log.h` to define `TAG`
```
#ifdef USE_ESP_IDF_LOG
#ifndef TAG
#define TAG "myAPP"
#endif
```
Update `platformio.ini` to include:
```
build_flags= -DUSE_ESP_IDF_LOG -DCORE_DEBUG_LEVEL=5
```
Include the following libraries and definitions:
```
#include "esp_log.h"
#include "esp32-hal-log.h"

#define LOG_LEVEL ESP_LOG_WARN
#define MY_ESP_LOG_LEVEL ESP_LOG_INFO
```
## ESP-Prog
MAC OS driver issue:
https://arduino.stackexchange.com/questions/91111/how-to-install-ftdi-serial-drivers-on-mac
## Settings to update in Dependencies
### ElegantOTA
Enable async webserver in the 
```
  #define ELEGANTOTA_USE_ASYNC_WEBSERVER 1
```
### FTP Server SD Card Settings


# Architecture
## Power Supply
For powering the ESP32 development kit via USB, an 18650 battery is utilized. However, for production purposes, a custom PCB will be designed, and the module should be powered via the 3V3 or VIN pin to minimize power loss.
To harness solar power, the 18650 Shield is employed, facilitating power supply to the ESP32. [This AliExpress link](https://www.aliexpress.us/item/3256805800017684.html?spm=a2g0o.order_list.order_list_main.53.14a11802o1NdIO&gatewayAdapt=glo2usa) provides details on the shield. The input voltage range is specified as 5V to 8V, although preliminary tests suggest that a 5V solar panel is functional. Further validation will be conducted.
SD only seems to work with power from VIN pin with a buck converter in between.
- 2.6 is the minimum power
### Power Consumption per Mode
The following table outlines the current consumption of the ESP32 under various operating modes:
| Mode                                        | Current Consumption |
|---------------------------------------------|---------------------|
| WiFi TX, DSSS 1 Mbps, POUT = +19.5 dBm      | 240 mA              |
| WiFi TX, OFDM 54 Mbps, POUT = +16 dBm       | 190 mA              |
| WiFi TX, OFDM MCS7, POUT = +14 dBm          | 180 mA              |
| WiFi RX (listening)                         | (95~100) mA         |
| BT/BLE TX, POUT = 0 dBm                     | 130 mA              |
| BT/BLE RX (listening)                       | (95~100) mA         |
### Solar Panel Configuration
Currently, the setup includes two 0.3W 5V solar panels, capable of supplying a maximum of 120mA to the shield.
## Time
### NTP Server
The logger should sync with the NTP server upon power-up using `configTime ` from the `time.h` library. Use `getLocalTime(&timeinfo)` to get the current time. This function should be called within the logging function to get the exact time. However, time would not be kept if power is lost. A RTC module is needed to provide time without WiFi after powerloss.
Note: not sure if the current implementation (RTClib) will poll the ntp server periodically. However, offical implementations by Espressif does poll periodically. https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/system_time.html
### External RTC Setup
Note that both the DS1307 and the OLED screen are connected to the I2C bus, same bus but different address. The libraries are designed such that they can scan the I2C bus for common addresses.
Use this guide: https://esp32io.com/tutorials/esp32-ds1307-rtc-module
Note that the tiny RTC module does not work with 3V3, instead VIN should be supplied.
## File System
### Flash Memory Partition
Espressif documentation on partition tables: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html
If using PlatformIO, the default partition can be found in this directory: `.platformio/packages/framework-arduinoespressif32/tools/partitions`
### SD Card Setup
https://components101.com/modules/micro-sd-card-module-pinout-features-datasheet-alternatives
https://www.electronicwings.com/esp32/microsd-card-interfacing-with-esp32
### Database for Timeseries Data
Not sure, store files by date may be sufficient.
## Internet Access
### WiFi Reconnect Capability
The `WiFi.onEvent()` function is used to register a callback function, `WiFiEvent`, which will be invoked when WiFi events occur. In the WiFiEvent function, we check for the `SYSTEM_EVENT_STA_DISCONNECTED` event, indicating a WiFi disconnection. When this event occurs, we call `reconnectToWiFi()` to attempt reconnection. This way, the reconnection logic is encapsulated in the WiFiEvent callback, keeping the loop() function free of reconnection-related code.
### WiFi Manager
TODO. This function is triggered when the physical push button switch is clicked, the ESP32 will start as a WiFi access point to allow the user to connect to it via WiFi. A device configuration website will be served over WiFi.
https://dronebotworkshop.com/wifimanager/
Read this issue to learn how to erase wifi settings from ESP32, [link](https://github.com/espressif/arduino-esp32/issues/400). Otherwise the ESP32 will boot up and use the previous settings automatically, can really mess up.
### Dynamic IP Address
ESP32 should request static IP from the access point (e.g. WiFi router, LTE router); Another approach is to set static IP in router admin page for the ESP32.
The router might have dynamic IP address which might expire every few days, unless a static IP is purchased from the ISP.
TODO: esp32 API to update IP to management server.
## Radio Network
For most civil infrastructure applications where low-latency monitoring isn't critical and data rates aren't excessively high, LoRaWAN emerges as the industry standard. However, in scenarios demanding higher data rates, ESP NOW can be leveraged for shorter distance projects. For longer distance projects, an alternative approach could involve integrating additional cell modems into each station and relinquishing interconnection between the stations.
[ESP NOW – Peer to Peer ESP32 Communications](https://dronebotworkshop.com/esp-now/)

Reference: [Intro to LoRaWAN](https://www.youtube.com/watch?v=hMOwbNUpDQA) by Andreas Spiess.
| Aspect              | ESP-NOW                            | LoRaWAN                               |
|---------------------|------------------------------------|---------------------------------------|
| Range               | Short range, local area            | Long range, wide area                 |
| Power Consumption   | Low power    | Ultra-low power|
| Data Rate           | High data rates, real-time         | Low data rates, optimized for range   |
| Topology            | Peer-to-peer (P2P)                 | Star-of-stars                         |
| Scalability         | Small to medium networks           | Large-scale networks                  |
| Regulatory          | 2.4 GHz ISM band                   | Sub-gigahertz ISM bands               |
| Infrastructure      | Included in bare ESP32 module      | Gateway devices required              |

After conducting research, it appears that implementing LoRaWAN requires Gateway devices. However, opting for ESP-NOW provides an alternative solution and allows for exploration of range extension possibilities. LoRaWAN can be added when budget is available.

If a LoRa gateway is unnecessary, the firmware for all ESP32 devices can remain similar. Only the "gateway device" or main station needs adjustments to handle data communication and data table combination for HTTP requests from remote clients. Substations should still support local WiFi communication and serve webpages for users in areas without cell service.
### How do devices interconnect?
The device will be configured via the WiFi manager interface, which is essentially serving a website for users to set modes.
- The user needs to maintain a table of MAC addresses of each device, either gateway or node
- Each device will be booted up using the appropriate mode.
- Each device will be configured by the user to communicate with the gateway using the gateway's MAC address
### Hardware
ESP-32 dev boards with external antenna connections available is recommended: ESP32-WROOM-U. ESP-NOW long-range mode should be investigated in both urban and rural areas.
## Data Logging Functions
The data logging function should support different logging modes. Could be generalized based on protocol used: I2C, SPI, RS485, etc. Readings should be first saved on the device, before sending over ESP-NOW. Confirmation is needed before deleting file.
### GPIO Pin Monitor
When interfacing with new peripherals, this [GPIO Pin Monitor](https://www.youtube.com/watch?v=UxkOosaNohU) can provide remote monitoring userinterface for prototyping.
### Sensor Type Supported
TODO not tested yet vibrating wire sensors, analog sensors, SAAs.
I want to have the same capabilities: https://www.geo-instruments.com/technology/wireless-logger-networks/
#### VM501
Use ESP32 VIN out for power supply, multimeter shows a voltage of approximately 4.5V. Connect ESP32 VIN to V33 on VM501, GND to GND. Initialize UART port 1 with GPIO16 as RX and GPIO17 as TX. Run `HardwareSerial VM(1);` to configure the UART port on ESP32. Run `VM.begin(9600, SERIAL_8N1, 16, 17);` to initialize UART port 1 with GPIO16 as RX and GPIO17 as TX.
VM.Serial UART Protocol functions implemented in this project are based on the MODBUS protocol:
- Read registers from VM501
- Write registers to VM501
- CRC Algorithm
- Special Instructions
## OTA
Currently ElegantOTA free version is used without licensing for commercial applications. Documentaion: https://docs.elegantota.pro/
For commercial applications, a simple Arduino OTA wrapper library can be developed to avoid ElegantOTA.
TODO develope own version of OTA to avoid restrictions.
Importantly, remember to enable async webserver opetion in `ElegantOTA.h` in `./pio/libdeps/esp32dev/ElegantOTA`.

# API
An instance of AsyncWebServer is created on port 80. A Callback function is set up to handle incoming HTTP GET requests at the root ("/") by responding with the content of a file stored in the SPIFFS file system. Adjust the filename variable to match the desired file. After configuring the server, it is started with `server.begin()`.
## Logger System Control
### Logger Configuration
Impletemented using the Preference.h library.
```
credentials
{
    "WIFI_SSID": "*********",
    "WIFI_PASSWORD": "**********",
    "gmtOffset_sec": "************"
}
```
### ESP-NOW Routing Map
The master logger should have a master list of all substation system information.
```
[
    {
        "macAddress": "30:83:98:00:52:8C",
        "batteryVoltage": "3.7V"
    },
    {
        "macAddress": "30:83:98:00:52:8C",
        "batteryVoltage": "3.7V"
    },
]
```

### File system TODO
## Data Retrieval TODO
### Timeseries request
The logger should liten on route `/api/readings` for timeseries requests. The client can specify the `sensorId`, `start` and `end`, and `readingsOptions`. A sample request should look like the following:
```
/api/readings?sensorId=238&start=2024-02-06T13:40:00&end=2024-02-13T13:40:00&readingsOptions=0
```



# License
Created by Qiwei Mao
# Useful References
Random Nerd Tutorials: https://randomnerdtutorials.com/projects-esp32/
Dashboard: https://github.com/ayushsharma82/ESP-DASH
Github Reference https://github.com/topics/sensors-data-collection
