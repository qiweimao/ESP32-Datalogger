#include <FS.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "Secrets.h"
#include "datalogging.h"
#include "utils.h"
#include "api_interface.h"

// Include the required libraries
#include <HardwareSerial.h>
// Define the UART port to use (e.g., Serial1, Serial2, Serial3)
HardwareSerial VM(1); // UART port 1 on ESP32

/* User Input*/
const char *AWSEC2 = "lelelumon.shop";
const char *filename = "/log.txt";
const long gmtOffset_sec = -5 * 60 * 60;  // GMT offset in seconds (Eastern Time Zone)
const int daylightOffset_sec = 3600;  
const int LOG_INTERVAL = 1000;

/* Do not modify below */
bool loggingPaused = true;
SemaphoreHandle_t logMutex;

void setup() {
  Serial.begin(115200);
  VM.begin(9600, SERIAL_8N1, 16, 17); // Initialize UART port 1 with GPIO16 as RX and GPIO17 as TX

  Serial.println("-------------------------------------\nBooting...");
  logMutex = xSemaphoreCreateMutex();  // Initialize the mutex

  connectToWiFi();// Set up WiFi connection
  WiFi.onEvent(WiFiEvent);// Register the WiFi event handler

  setupSPIFFS();// Setup SPIFFS -- Flash File System
  SD_initialize();//SD card file system initialization

  startServer();// start Async server with api-interfaces

  // setUpTime();// Sync With NTP Time Server
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
  if (VM.available()) {
    String receivedChar = VM.readString();
    Serial.print("VM501 Response:\n");
    Serial.println(receivedChar);
  }


  // Read Data from PC Keyboard
  if (Serial.available()) {
    // Read input from Serial Monitor
    String input = Serial.readStringUntil('\n');

    // Hexadecimal data to store
    uint8_t hexData[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x30, 0xC5, 0xCD};
    VM.write(hexData, sizeof(hexData));

    Serial.println("Sent to VM");
    /* VM Response below*/
    // Check if data is available to read
    uint32_t count = 0;
    while (VM.available() > 0) {
      // Read the big endian bytes
      uint8_t byte = VM.read(); // Most significant byte
      
      // Print out the received value in hex
      if(count%2 == 0){
        Serial.printf("0x");
        if(count < 0x10){
          Serial.printf("0");
        }
        Serial.printf("%X",count);
      }

      Serial.printf("0x");
      if (byte < 0x10) Serial.print("0"); // Print leading zero if necessary
      Serial.printf("%X", byte);

      if(count%2 == 1){
        Serial.println();
      }

    }

  }

}