#ifndef UTILS_h
#define UTILS_h

#include <time.h>
#include <FS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <SD.h>
#include "Secrets.h"


extern const char *filename;
extern const char *ntpServer;
extern const long gmtOffset_sec;  // GMT offset in seconds (Eastern Time Zone)
extern const int daylightOffset_sec;       // Daylight saving time offset in seconds

String getCurrentTime();
void setUpTime();
void connectToWiFi();
void reconnectToWiFi();
void WiFiEvent(WiFiEvent_t event);
void setupSPIFFS();
void SD_initialize();
void SDWriteFile(const char * path, const char * message);
void SDReadFile(const char * path);

#endif