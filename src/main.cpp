#include <FS.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "Secrets.h"
#include "aws_mqtt.h"
#include "datalogging.h"
#include "utils.h"
#include "api_interface.h"

/* User Input*/
const char *AWSEC2 = "lelelumon.shop";
const char *filename = "/log.txt";
const long gmtOffset_sec = -5 * 60 * 60;  // GMT offset in seconds (Eastern Time Zone)
const int daylightOffset_sec = 3600;  
const int LOG_INTERVAL = 1000;

/* Do not modify below */
bool loggingPaused = false;
extern AsyncWebServer server;
extern WiFiClientSecure net;
extern PubSubClient client;

SemaphoreHandle_t logMutex;

void setup() {
  Serial.begin(115200);
  delay(5000);
  Serial.println("-------------------------------------\nBooting...");
  
  logMutex = xSemaphoreCreateMutex();  // Initialize the mutex

  connectToWiFi();// Set up WiFi connection
  WiFi.onEvent(WiFiEvent);// Register the WiFi event handler

  setupSPIFFS();// Setup SPIFFS -- Flash File System
  SD_initialize();//SD card file system initialization

  startServer();// start Async server with api-interfaces

  setUpTime();// Sync With NTP Time Server
  Serial.println("-------------------------------------");
}

void loop() {

if(!loggingPaused){
  if (xSemaphoreTake(logMutex, portMAX_DELAY)) {
    LogErrorCode result = logFlashSize();
    xSemaphoreGive(logMutex);
  }
}

}
