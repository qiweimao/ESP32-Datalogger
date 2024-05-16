#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HardwareSerial.h>

#include "secrets.h"
#include "data_logging.h"
#include "utils.h"
#include "api_interface.h"
#include "vm_501.h"
#include "esp_now_init.h"

/* Tasks */
SemaphoreHandle_t logMutex;
TaskHandle_t parsingTask; // Task handle for the parsing task
TaskHandle_t wifimanagerTaskHandle; // Task handle for the parsing task
TaskHandle_t blinkTaskHandle; // Task handle for the parsing task

void taskInitiNTP(void *parameter) {
  initNTP();  // Call the initNTP function
  Serial.println("Deleted NTP task");
  vTaskDelete(NULL);  // Delete the task once initialization is complete
}

void logDataTask(void *parameter) {
  while(true){
    LogErrorCode result = logData();
  }
}

void blinkTask(void *parameter) {
  while (true){
    delay(100);
    if(wifimanagerrunning){
      delay(500);
      digitalWrite(LED,HIGH);
      delay(500);
      digitalWrite(LED,LOW);
    }
  }
}

void setup() {

  Serial.begin(115200);
  Serial.println("\n------------------Booting-------------------\n");

  /* Essential System */
  Serial.println("*** Core System ***");
  clearWiFiConfiguration();
  pinMode(TRIGGER_PIN, INPUT_PULLUP);// Pin setting for wifi manager push button
  pinMode(LED,OUTPUT);// onboard blue LED inidcator
  setupSPIFFS();// Setup SPIFFS -- Flash File System
  SD_initialize();//SD card file system initialization
  loadSysConfig();
  xTaskCreatePinnedToCore(blinkTask, "blinkTask", 4096, NULL, 1, &blinkTaskHandle, 1);

  initESP_NOW();
  startServer();// start Async server with api-interfaces

  /* Logging Capabilities */
  // logMutex = xSemaphoreCreateMutex();  // Mutex for current logging file
  // void initVM501();
  // initDS1307();// Initialize external RTC, MUST BE INITIALIZED BEFORE NTP
  // initializeOLED();
  // xTaskCreate(logDataTask, "logDataTask", 4096, NULL, 1, NULL);
  // xTaskCreate(taskInitiNTP, "InitNTPTask", 4096, NULL, 1, NULL);

  Serial.println("\n------------------Boot Completed----------------\n");
  Serial.println("Entering Loop");
}

void loop() {
  
  if (ESP_NOW_MODE == ESP_NOW_SENDER){
    espSendData();
  }

  ElegantOTA.loop();

}