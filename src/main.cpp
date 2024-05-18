#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HardwareSerial.h>

#include "secrets.h"
#include "data_logging.h"
#include "utils.h"
#include "api_interface.h"
#include "vm_501.h"
#include "esp_now_init.h"

#if defined(ESP32)
#include <SD.h>
#include <SPIFFS.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FS.h>
#endif

#include "ESP-FTP-Server-Lib.h"
#include "FTPFilesystem.h"

#define FTP_USER     "esp32"
#define FTP_PASSWORD "esp32"

// #ifndef UNIT_TEST

FTPServer ftp;


/* Tasks */
SemaphoreHandle_t logMutex;
TaskHandle_t parsingTask; // Task handle for the parsing task
TaskHandle_t wifimanagerTaskHandle; // Task handle for the parsing task
TaskHandle_t blinkTaskHandle; // Task handle for the parsing task

void taskInitiNTP(void *parameter) {
  ntp_sync();  // Call the initNTP function
  Serial.println("Deleted NTP task");
  vTaskDelete(NULL);  // Delete the task once initialization is complete
}

void logDataTask(void *parameter) {
  while(true){
    LogErrorCode result = logData();
  }
}

void setup() {

  Serial.begin(115200);
  Serial.println("\n------------------Booting-------------------\n");

  /* Core System */
  Serial.println("*** Core System ***");
  wifi_setting_reset();
  pinMode(TRIGGER_PIN, INPUT_PULLUP);// Pin setting for wifi manager push button
  pinMode(LED,OUTPUT);// onboard blue LED inidcator
  spiffs_init();// Setup SPIFFS -- Flash File System
  sd_init();//SD card file system initialization
  load_system_configuration();
  esp_now_setup();
  start_http_server();// start Async server with api-interfaces
  external_rtc_init();// Initialize external RTC, MUST BE INITIALIZED BEFORE NTP
  oled_init();
  xTaskCreate(taskInitiNTP, "InitNTPTask", 4096, NULL, 1, NULL);

  ftp.addUser(FTP_USER, FTP_PASSWORD);
  #if defined(ESP32)
    ftp.addFilesystem("SD", &SD);
  #endif
  ftp.addFilesystem("SPIFFS", &SPIFFS);
  ftp.begin();

  /* Logging Capabilities */
  Serial.println("\n------------------Boot Completed----------------\n");
  Serial.println("Entering Loop");
}

void loop() {
  ftp.handle();
  ElegantOTA.loop();
}