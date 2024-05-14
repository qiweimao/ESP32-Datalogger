#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HardwareSerial.h>
#include "Secrets.h"
#include "datalogging.h"
#include "utils.h"
#include "api_interface.h"
#include "VM_501.h"
#include "espInit.h"

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

void wifimanagerTask(void *parameter) {
  while(true){
    delay(3000);
    if ( !wifimanagerrunning && digitalRead(TRIGGER_PIN) == LOW) {
      WiFi.mode(WIFI_AP);
      const char* apSSID = "ESP32-AP";
      const char* apPassword = "12345678";
      WiFi.softAP(apSSID, apPassword);

      if(ESP_NOW_MODE == ESP_NOW_SENDER){
        startServer();// start Async server with api-interfaces
      }

      // Get the IP address of the access point
      IPAddress apIP = WiFi.softAPIP();
      Serial.print("Access Point IP address: ");
      Serial.println(apIP);
      wifimanagerrunning = true;
    }
    else if (wifimanagerrunning && digitalRead(TRIGGER_PIN) == LOW){
        wifimanagerrunning = false;
        delay(4000);
        Serial.println("Reboot...");
        ESP.restart(); // Reboot the ESP32
    }
  }
}

void setup() {
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  pinMode(LED,OUTPUT);

  /* Essentials for Remote Access */
  Serial.begin(115200);
  Serial.println("-------------------------------------\nBooting...");

  setupSPIFFS();// Setup SPIFFS -- Flash File System
  SD_initialize();//SD card file system initialization
  // connectToWiFi();// Set up WiFi connection
  // WiFi.onEvent(WiFiEvent);// Register the WiFi event handler

  /* Logging Capabilities */
  // logMutex = xSemaphoreCreateMutex();  // Mutex for current logging file
  // void initVM501();
  // initDS1307();// Initialize external RTC, MUST BE INITIALIZED BEFORE NTP
  // initializeOLED();

  loadSysConfig();

  if (ESP_NOW_MODE == ESP_NOW_SENDER){
    Serial.println("Initialized as Sender");
    espSenderInit();
  }
  if (ESP_NOW_MODE == ESP_NOW_RESPONDER){
    Serial.println("Initialized as Responder");
    espResponderInit();
    startServer();// start Async server with api-interfaces
  }

  xTaskCreatePinnedToCore(wifimanagerTask, "wifimanagerTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(blinkTask, "blinkTask", 4096, NULL, 1, &blinkTaskHandle, 1);
  // xTaskCreate(logDataTask, "logDataTask", 4096, NULL, 1, NULL);
  // xTaskCreate(taskInitiNTP, "InitNTPTask", 4096, NULL, 1, NULL);
  Serial.println("-------------------------------------");
  Serial.println("Data Acquisition Started...");

}

void loop() {
  
  // if (ESP_NOW_MODE == ESP_NOW_SENDER){
  //   espSendData();
  // }

  ElegantOTA.loop();

}