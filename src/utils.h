#ifndef UTILS_h
#define UTILS_h

#include <time.h>
#include <RTClib.h>
#include <FS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <Arduino.h>
#include <SD.h>
#include <AsyncTCP.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SimpleFTPServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "esp_wifi.h"


#include "secrets.h"
#include "data_logging.h"
#define ESP_NOW_SENDER 0
#define ESP_NOW_RESPONDER 1
#define ESP_NOW_DUAL 2

#define TRIGGER_PIN 4
#define LED 2

extern int ESP_NOW_MODE;
extern bool wifimanagerrunning; // Flag to indicate if WiFi configuration is done


extern RTC_DS1307 rtc;
extern char daysOfWeek[7][12];

extern String WIFI_SSID;
extern String WIFI_PASSWORD;


void clearWiFiConfiguration();

void loadSysConfig();
void updateSysConfig(String newSSID, String newWiFiPassword, long newgmtOffset_sec, int newESP_NOW_MODE);

String getCurrentTime();
void initDS1307();
void printDateTime(DateTime dt);
void initNTP();
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

void initVM501();
void sendCommandVM501(void *parameter);
void parseCommand(const char* command);
void VM501ListenTaskFunc(void *parameter);
unsigned int crc16(unsigned char *dat, unsigned int len);

void initializeOLED();

#endif