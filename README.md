# ESP32 Data Logger

## Internet Connection

## Time
The logger should sync with the NTP server upon power-up using `configTime ` from the `time.h` library. Use `getLocalTime(&timeinfo)` to get the current time. This function should be called within the logging function to get the exact time. However, time would not be kept if power is lost. A RTC module is needed to provide time without WiFi after powerloss.

## Flash Memory Partition
Espressif documentation on partition tables: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html
If using PlatformIO, the default partition can be found in this directory: `.platformio/packages/framework-arduinoespressif32/tools/partitions`