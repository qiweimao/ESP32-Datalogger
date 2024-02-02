# ESP32 Data Logger
intro here

## Internet Connection

## Time
The logger should sync with the NTP server upon power-up using `configTime ` from the `time.h` library. Use `getLocalTime(&timeinfo)` to get the current time. This function should be called within the logging function to get the exact time. However, time would not be kept if power is lost. A RTC module is needed to provide time without WiFi after powerloss.

## Flash Memory Partition
Espressif documentation on partition tables: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html
If using PlatformIO, the default partition can be found in this directory: `.platformio/packages/framework-arduinoespressif32/tools/partitions`

## WiFi Reconnect Capability
The `WiFi.onEvent()` function is used to register a callback function, `WiFiEvent`, which will be invoked when WiFi events occur. In the WiFiEvent function, we check for the `SYSTEM_EVENT_STA_DISCONNECTED` event, indicating a WiFi disconnection. When this event occurs, we call `reconnectToWiFi()` to attempt reconnection. This way, the reconnection logic is encapsulated in the WiFiEvent callback, keeping the loop() function free of reconnection-related code.

## Server Setup
An instance of AsyncWebServer is created on port 80. A Callback function is set up to handle incoming HTTP GET requests at the root ("/") by responding with the content of a file stored in the SPIFFS file system. Adjust the filename variable to match the desired file. After configuring the server, it is started with `server.begin()`.


## OTA
Placeholder

## ESP-Prog
MAC OS driver issue:
https://arduino.stackexchange.com/questions/91111/how-to-install-ftdi-serial-drivers-on-mac
## Domain Name and IP Address
ESP32 should request static IP from the access point (e.g. WiFi router, LTE router); Another approach is to set static IP in router admin page for the ESP32.
The router might have dynamic IP address which might expire every few days, unless a static IP is purchased from the ISP.

## API
### Send IP Address
ESP32 send the IP address to AWS, AWS machine receives the IP address of the ESP32 and creates a redirect link to the ESP32.

### Remote Debugging and Configuration:
Consider implementing a secure way to enable and disable debugging remotely to ensure the security of your system.
Create APIs that allow changing configurations, such as sensor sampling intervals, thresholds, or other parameters. Ensure that these changes persist across reboots.

### Client Data Request:
Develop APIs to retrieve specific data or a range of data from the logs. This could involve specifying a time range, specific sensors, or other relevant parameters.

Consider security measures, such as authentication and authorization, to ensure that only authorized clients can access the data.
Implement a mechanism to handle different data formats (e.g., JSON, CSV) based on client preferences.
Security:

Use secure communication protocols, such as HTTPS, to protect the data exchange between the ESP32 and clients.
Implement proper authentication mechanisms to ensure that only authorized clients can access and modify the device's settings.
Consider encrypting sensitive data to protect it from unauthorized access.

### Error Handling:
Implement robust error-handling mechanisms for both API requests and data logging. Provide meaningful error messages that can assist in debugging.
Include mechanisms for logging errors or issues to facilitate remote diagnostics.
Documentation:

Document your APIs thoroughly, including the expected input parameters, output formats, and any potential error codes. This documentation will be valuable for both yourself and potential users of your system.

### Scalability:
Design your APIs to be scalable, considering the potential growth in the number of sensors or clients. Ensure that your system can handle increased data traffic and requests.

## Useful References
Random Nerd Tutorials: https://randomnerdtutorials.com/projects-esp32/
