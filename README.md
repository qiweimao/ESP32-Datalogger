# Introduction
The ESP32 Data Logger is a cost efficient data acquisition system that supports vibrating wire sensors and other sensors with RS-485, TTL protocol. To set up the data logger as an end user, you can plug it into your computer, and configure the logger in a website using the browser. For advanced configurations, you can program your own code and flash the logger on PC, using PlatformIO or Arduino IDE.

- [Introduction](#introduction)
- [Architecture](#architecture)
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
  - [Data Logging Functions](#data-logging-functions)
    - [Sensor Type Supported](#sensor-type-supported)
      - [VM501](#vm501)
    - [Logging Configuration](#logging-configuration)
  - [OTA](#ota)
  - [Troubleshooting](#troubleshooting)
    - [Telnet](#telnet)
    - [ESP-Prog](#esp-prog)
- [API](#api)
  - [Logger System Control](#logger-system-control)
    - [System Info](#system-info)
    - [Datalogging Configuration](#datalogging-configuration)
    - [File system](#file-system-1)
  - [Data Requests](#data-requests)
    - [Timeseries request](#timeseries-request)
- [Useful References](#useful-references)
- [Issue Tracking](#issue-tracking)

# Architecture
## Time
### NTP Server
The logger should sync with the NTP server upon power-up using `configTime ` from the `time.h` library. Use `getLocalTime(&timeinfo)` to get the current time. This function should be called within the logging function to get the exact time. However, time would not be kept if power is lost. A RTC module is needed to provide time without WiFi after powerloss.
### External RTC Setup
TODO Model No.: DS1307 AT24C32 Real Time Clock Module For AVR ARM PIC NEW
## File System
### Flash Memory Partition
Espressif documentation on partition tables: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html
If using PlatformIO, the default partition can be found in this directory: `.platformio/packages/framework-arduinoespressif32/tools/partitions`
### SD Card Setup
https://components101.com/modules/micro-sd-card-module-pinout-features-datasheet-alternatives
https://www.electronicwings.com/esp32/microsd-card-interfacing-with-esp32
### Database for Timeseries Data

## Internet Access
### WiFi Reconnect Capability
The `WiFi.onEvent()` function is used to register a callback function, `WiFiEvent`, which will be invoked when WiFi events occur. In the WiFiEvent function, we check for the `SYSTEM_EVENT_STA_DISCONNECTED` event, indicating a WiFi disconnection. When this event occurs, we call `reconnectToWiFi()` to attempt reconnection. This way, the reconnection logic is encapsulated in the WiFiEvent callback, keeping the loop() function free of reconnection-related code.
### WiFi Manager
TODO. This function is triggered when the physical push button switch is clicked, the ESP32 will start as a WiFi access point to allow the user to connect to it via WiFi. A device configuration website will be served over WiFi.
### Dynamic IP Address
ESP32 should request static IP from the access point (e.g. WiFi router, LTE router); Another approach is to set static IP in router admin page for the ESP32.
The router might have dynamic IP address which might expire every few days, unless a static IP is purchased from the ISP. TODO: esp32 API to update IP to management server.
## Data Logging Functions
The data logging function should support different logging modes
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

### Logging Configuration
Logging interval, database, 
## OTA
Currently ElegantOTA free version is used without licensing for commercial applications. Documentaion: https://docs.elegantota.pro/
For commercial applications, a simple Arduino OTA wrapper library can be developed to avoid ElegantOTA.
TODO develope own version of OTA to avoid restrictions.
## Troubleshooting
### Telnet
A port for telnet is opened.
TODO security check
### ESP-Prog
MAC OS driver issue:
https://arduino.stackexchange.com/questions/91111/how-to-install-ftdi-serial-drivers-on-mac
# API
An instance of AsyncWebServer is created on port 80. A Callback function is set up to handle incoming HTTP GET requests at the root ("/") by responding with the content of a file stored in the SPIFFS file system. Adjust the filename variable to match the desired file. After configuring the server, it is started with `server.begin()`.
## Logger System Control
### System Info
TODO UI Update
### Datalogging Configuration
TODO update API
### File system
TODO UI Update
## Data Requests
### Timeseries request
The logger should liten on route `/api/readings` for timeseries requests. The client can specify the `sensorId`, `start` and `end`, and `readingsOptions`. A sample request should look like the following:
```
/api/readings?sensorId=238&start=2024-02-06T13:40:00&end=2024-02-13T13:40:00&readingsOptions=0
```
# Useful References
Random Nerd Tutorials: https://randomnerdtutorials.com/projects-esp32/
Dashboard: https://github.com/ayushsharma82/ESP-DASH

# Issue Tracking
- server on public ip may not start sometimes:
```
Server Started @ IP: 192.168.0.167
[ 16620][E][ssl_client.cpp:37] _handle_error(): [start_ssl_client():273]: (-29312) SSL - The connection indicated an EOF
[ 16620][E][WiFiClientSecure.cpp:144] connect(): start_ssl_client: -29312
HTTP request failed with error code -1
Public IP Address: Error
```