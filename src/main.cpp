#include <FS.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "Secrets.h"
#include "datalogging.h"
#include "utils.h"
#include "api_interface.h"
#include "VM_501.h" // delete after testing


// Include the required libraries
#include <HardwareSerial.h>
// Define the UART port to use (e.g., Serial1, Serial2, Serial3)
HardwareSerial VM(1); // UART port 1 on ESP32

/* User Input*/
const char *filename = "/log.txt";
const long gmtOffset_sec = -5 * 60 * 60;  // GMT offset in seconds (Eastern Time Zone)
const int daylightOffset_sec = 3600;  
const int LOG_INTERVAL = 3000;

/* Do not modify below */
bool loggingPaused = false;
SemaphoreHandle_t logMutex;
TaskHandle_t parsingTask; // Task handle for the parsing task
TaskHandle_t vmListeningTask; // Task handle for the VM listening task

void setup() {
  Serial.begin(115200);
  // 0x01 0x03 0x00 0x00 0x00 0x0A 0xC5 0xCD
  xTaskCreatePinnedToCore(sendCommandVM501, "ParsingTask", 4096, NULL, 1, &parsingTask, 1);
  // xTaskCreatePinnedToCore(VM501ListenTaskFunc, "ParsingTask", 4096, NULL, 1, &vmListeningTask, 1);
  VM.begin(9600, SERIAL_8N1, 16, 17); // Initialize UART port 1 with GPIO16 as RX and GPIO17 as TX

  Serial.println("-------------------------------------\nBooting...");
  logMutex = xSemaphoreCreateMutex();  // Initialize the mutex

  connectToWiFi();// Set up WiFi connection
  WiFi.onEvent(WiFiEvent);// Register the WiFi event handler

  setupSPIFFS();// Setup SPIFFS -- Flash File System
  SD_initialize();//SD card file system initialization

  startServer();// start Async server with api-interfaces

  setUpTime();// Sync With NTP Time Server
  Serial.println("-------------------------------------");
  Serial.println("Data Acquisition Started...");

}

void loop() {

  if(!loggingPaused){
    if (xSemaphoreTake(logMutex, portMAX_DELAY)) {
      LogErrorCode result = logFlashSize();
      xSemaphoreGive(logMutex);
    }
  }

  ElegantOTA.loop();

  // Read data from UART port
  // if (VM.available()) {
  //   String receivedChar = VM.readString();
  //   Serial.print("VM501 Response:\n");
  //   Serial.println(receivedChar);
  // }


  // // Read Data from PC Keyboard
  // if (Serial.available()) {
  //   sendMODBUSCommand(0x01, 0x03, 0x00, 0x00, 0x00, 0x0A, 0xC5, 0xCD);
  // }

}