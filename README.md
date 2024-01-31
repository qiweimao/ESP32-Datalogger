# ESP32 Data Logger
intro here

## Internet Connection

## Time
The logger should sync with the NTP server upon power-up using `configTime ` from the `time.h` library. Use `getLocalTime(&timeinfo)` to get the current time. This function should be called within the logging function to get the exact time. However, time would not be kept if power is lost. A RTC module is needed to provide time without WiFi after powerloss.

## Flash Memory Partition
Espressif documentation on partition tables: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html
If using PlatformIO, the default partition can be found in this directory: `.platformio/packages/framework-arduinoespressif32/tools/partitions`

## WiFi Reconnect Capability
In the `setup()` function, the `connectToWiFi()` function is called to initiate the initial WiFi connection. The `WiFi.onEvent()` function is used to register a callback function, `WiFiEvent`, which will be invoked when WiFi events occur. In the WiFiEvent function, we check for the `SYSTEM_EVENT_STA_DISCONNECTED` event, indicating a WiFi disconnection. When this event occurs, we call `reconnectToWiFi()` to attempt reconnection. The `reconnectToWiFi()` function can be customized to include any necessary cleanup or reconfiguration steps before attempting to reconnect. In this example, it simply prints a message and calls `connectToWiFi()`. This way, the reconnection logic is encapsulated in the WiFiEvent callback, keeping the loop() function free of reconnection-related code.

## Server Setup
An instance of AsyncWebServer is created on port 80. A Callback function is set up to handle incoming HTTP GET requests at the root ("/") by responding with the content of a file stored in the SPIFFS file system. Adjust the filename variable to match the desired file. After configuring the server, it is started with `server.begin()`.

## Domain Name and IP Address
ESP32 should request static IP from the access point (e.g. WiFi router, LTE router); Another approach is to set static IP in router admin page for the ESP32.
The router might have dynamic IP address which might expire every few days, unless a static IP is purchased from the ISP.
*** TODO *** Figure out how DDNS works on Namecheap

## Useful References
Random Nerd Tutorials: https://randomnerdtutorials.com/projects-esp32/
