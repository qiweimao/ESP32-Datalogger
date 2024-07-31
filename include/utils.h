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
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp32-hal-log.h"

#include "ESP-FTP-Server-Lib.h"
#include "FTPFilesystem.h"

#define FTP_USER     "esp32"
#define FTP_PASSWORD "esp32"

#define LOG_LEVEL ESP_LOG_ERROR
#define MY_ESP_LOG_LEVEL ESP_LOG_ERROR

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define TRIGGER_PIN 4
#define LED 2

extern int LORA_MODE;
extern RTC_DS1307 rtc;
extern bool rtc_mounted;
extern char daysOfWeek[7][12];
extern String WIFI_SSID;
extern String WIFI_PASSWORD;
extern int utcOffset;  // UTC offset in hours (Eastern Time Zone is -5 hours)

void wifi_setting_reset();
void wifi_init();
String get_current_time(bool getFilename = false);
String get_external_rtc_current_time();
String convertTMtoString(time_t now);
void external_rtc_init();
void external_rtc_sync_ntp();
void ntp_sync();
String get_public_ip();
void spiffs_init();
void oled_init();
void oled_print(const char* text);
void oled_print(uint8_t value);
void oled_print(const char* text, size_t size);

/* SD Card */
void sd_init();

void renameFolder(const char* oldFolderName, const char* newFolderName);

uint32_t generateRandomNumber();

int sdCardLogOutput(const char *format, va_list args);
void esp_error_init_sd_oled();

extern FTPServer ftp;
void ftp_server_init();


#endif