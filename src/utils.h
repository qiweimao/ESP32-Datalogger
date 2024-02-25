#ifndef UTILS_h
#define UTILS_h

#include <time.h>
#include <FS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <Arduino.h>
#include <SD.h>
#include <TelnetStream.h>
#include <AsyncTCP.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Secrets.h"

extern const char *filename;
extern const char *ntpServer;
extern const long gmtOffset_sec;  // GMT offset in seconds (Eastern Time Zone)
extern const int daylightOffset_sec;       // Daylight saving time offset in seconds

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// extern Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

String getCurrentTime();
void setUpTime();
void connectToWiFi();
void WiFiEvent(WiFiEvent_t event);
String getPublicIP();
void setupSPIFFS();

void SD_initialize();
String listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void createDir(fs::FS &fs, const char * path);
void removeDir(fs::FS &fs, const char * path);
void readFile(fs::FS &fs, const char * path);
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
void renameFile(fs::FS &fs, const char * path1, const char * path2);
void deleteFile(fs::FS &fs, const char * path);
void testFileIO(fs::FS &fs, const char * path);

void sendCommandVM501(void *parameter);
void parseCommand(const char* command);
void VM501ListenTaskFunc(void *parameter);
unsigned int crc16(unsigned char *dat, unsigned int len);

void initializeOLED();

#endif