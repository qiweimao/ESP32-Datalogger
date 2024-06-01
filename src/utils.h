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
#include <SimpleFTPServer.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define TRIGGER_PIN 4
#define LED 2

extern int LORA_MODE;
extern RTC_DS1307 rtc;
extern char daysOfWeek[7][12];
extern String WIFI_SSID;
extern String WIFI_PASSWORD;
extern int utcOffset;  // UTC offset in hours (Eastern Time Zone is -5 hours)
extern FtpServer ftpSrv;   //set #define FTP_DEBUG in ESP8266FtpServer.h to see ftp verbose on serial

void wifi_setting_reset();
void wifi_init();
String get_current_time(bool getFilename = false);
void external_rtc_init();
void ntp_sync();
String get_public_ip();
void spiffs_init();
void oled_init();
void oled_print(const char* text);
void oled_print(uint8_t value);

void lora_init(void);

/* SD Card */
void sd_init();
uint32_t generateRandomNumber();
void ftp_init();

#endif