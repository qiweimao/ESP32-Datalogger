#include "configuration.h"
#include "lora_init.h"
#include "lora_peer.h"
#include "lora_gateway.h"
#include "lora_slave.h"

//Define the pins used by the transceiver module
#define LORA_RST 27
#define DIO0 4
#define LORA_SCK 14
#define LORA_MISO 12
#define LORA_MOSI 13
#define LORA_SS 15

bool enableCRC = true; // Default CRC setting
SPIClass loraSpi(HSPI);// Separate SPI bus for LoRa to avoid conflict with the SD Card

volatile int dataReceived = 0;// Flag to indicate data received
int ack_count = 0;// Flag to indicate ACK for data transfer received
int rej_count = 0;// Flag to indicate REJ for data transfer received
uint8_t MAC_ADDRESS_STA[MAC_ADDR_LENGTH];

SemaphoreHandle_t xMutex_DataPoll = NULL; // mutex for LoRa hardware usage

// Define the type for the callback function
typedef void (*DataRecvCallback)(const uint8_t *incomingData, int len);

void lora_init(void){

  loraSpi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setSPI(loraSpi);
  LoRa.setPins(LORA_SS, LORA_RST, DIO0);

  esp_read_mac(MAC_ADDRESS_STA, ESP_MAC_WIFI_STA);  

  while (!LoRa.begin(915E6)) {  //915E6 for North America
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing - OK");

  // Conditionally enable CRC
  if (enableCRC) {
    LoRa.enableCrc();
    Serial.println("CRC enabled.");
  } else {
    LoRa.disableCrc();
    Serial.println("CRC disabled.");
  }

  xMutex_DataPoll = xSemaphoreCreateMutex();
  
  // Callback Initialization based on Mode
  if (systemConfig.LORA_MODE == LORA_SLAVE){
    Serial.println("Lora Mode: Sender");
    lora_slave_init();
  }
  if (systemConfig.LORA_MODE == LORA_GATEWAY){
    Serial.println("Lora Mode: Gateway");
    lora_gateway_init();
  }

}

void sendLoraMessage(uint8_t* data, size_t size) {
    uint8_t type = data[0];       // first message byte is the type of message 
    // Serial.print("Lora Message type: "); Serial.println(type);
    LoRa.beginPacket();
    LoRa.write(data, size);
    LoRa.endPacket(true);
    LoRa.receive(); // set receive mode
}

void onReceive(int packetSize) {
  dataReceived++;
}

void taskReceive(void *parameter) {

  DataRecvCallback callback = (DataRecvCallback)parameter;

  uint8_t buffer[250]; // Define a buffer to store incoming data
  int bufferIndex = 0; // Index to keep track of the buffer position
  
  while (true) {
    if (dataReceived) {
      // int packetSize = LoRa.parsePacket();
      dataReceived--; // Reset the flag for the next packet
      bufferIndex = 0; // Reset the buffer index
      
      while (LoRa.available() && bufferIndex < 250) {
        buffer[bufferIndex++] = LoRa.read();
      }

      // Call the callback function with the example data
      if (callback) {
          callback(buffer, bufferIndex);
      }

    }
    vTaskDelay(1 / portTICK_PERIOD_MS); // Delay for 1 second
  }
}