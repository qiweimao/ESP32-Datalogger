// #include "utils.h"
#include "lora_init.h"
#include "lora_gateway.h"
#include "lora_slave.h"

/* LoRa */
//define the pins used by the transceiver module
#define ss 15
#define rst 27
#define dio0 2
// // Define the pins used by the HSPI interface
#define LORA_SCK 14
#define LORA_MISO 12
#define LORA_MOSI 13
#define LORA_SS 15

SPIClass loraSpi(HSPI);

// Function pointer for the callback
typedef void (*LoRaReceiveCallback)(const uint8_t* data, int size);
void onLoRaDataReceived(const uint8_t* data, int size);
void registerLoRaCallback(LoRaReceiveCallback callback);
void loraTask(void *pvParameters);

LoRaReceiveCallback loraCallback = NULL;

// Task handle
TaskHandle_t loraTaskHandle;

void lora_init(void){

  loraSpi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setSPI(loraSpi);
  LoRa.setPins(LORA_SS, rst, dio0);

  //replace the LoRa.begin(---E-) argument with your location's frequency 
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
  while (!LoRa.begin(915E6)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
  

  if (ESP_NOW_MODE == ESP_NOW_SENDER){
    Serial.println("Initialized as Sender");
    NodeInit();
    registerLoRaCallback(onLoRaDataReceived);
  }
  if (ESP_NOW_MODE == ESP_NOW_RESPONDER){
    Serial.println("Initialized as Gateway");
    GatewayInit();
    registerLoRaCallback(onLoRaDataReceived);
  }
  // Create the LoRa packet checking task
  xTaskCreate(loraTask, "LoRa Task", 2048, NULL, 1, &loraTaskHandle);
}

void printMAC(const uint8_t mac_addr){
  Serial.print(mac_addr);
  oled_print(mac_addr);
}


// Function to register the callback
void registerLoRaCallback(LoRaReceiveCallback callback) {
  loraCallback = callback;
}

// LoRa packet checking task
void loraTask(void *pvParameters) {
  while (1) {
    // Check if a packet is received
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      uint8_t receivedData[packetSize];
      int index = 0;
      while (LoRa.available()) {
        receivedData[index++] = LoRa.read();
      }

      // Call the callback if it is registered
      if (loraCallback != NULL) {
        loraCallback(receivedData, packetSize);
      }
    }

    // Delay to avoid task hogging
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// Example callback function
void onLoRaDataReceived(const uint8_t* data, int size) {
  Serial.print("Received packet: ");
  for (int i = 0; i < size; i++) {
    Serial.print(data[i], HEX);
    if (i < size - 1) {
      Serial.print(":");
    }
  }
  Serial.println();
}