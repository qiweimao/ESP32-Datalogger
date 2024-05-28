// #include "utils.h"
#include "lora_init.h"
#include <ArduinoJson.h>

//Define the pins used by the transceiver module
#define LORA_RST 27
#define DIO0 4
#define LORA_SCK 14
#define LORA_MISO 12
#define LORA_MOSI 13
#define LORA_SS 15
SPIClass loraSpi(HSPI);// Separate SPI bus for LoRa to avoid conflict with the SD Card

volatile int dataReceived = 0;// Flag to indicate data received
const int maxPacketSize = 256; // Define a maximum packet size
String message = "";
uint8_t mac_buffer[6];


void lora_gateway_init();
void lora_slave_init();

// Define the type for the callback function
typedef void (*DataRecvCallback)(const uint8_t *incomingData, int len);

void lora_init(void){

  loraSpi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setSPI(loraSpi);
  LoRa.setPins(LORA_SS, LORA_RST, DIO0);

  while (!LoRa.begin(915E6)) {  //915E6 for North America
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
  
  if (LORA_MODE == LORA_SLAVE){
    Serial.println("Initialized as Sender");
    lora_slave_init();
  }
  if (LORA_MODE == LORA_GATEWAY){
    Serial.println("Initialized as Gateway");
    lora_gateway_init();
  }

}

void LoRa_rxMode(){
  LoRa.receive();                       // set receive mode
}

void LoRa_txMode(){
  LoRa.idle();                          // set standby mode
}

void LoRa_sendMessage(String message) {
  LoRa_txMode();                        // set tx mode
  LoRa.beginPacket();                   // start packet
  LoRa.print(message);                  // add payload
  LoRa.endPacket(true);                 // finish packet and send it
}

void onReceive(int packetSize) {
  dataReceived++;
}

void onTxDone() {
  LoRa_rxMode();
}

void taskReceive(void *parameter) {

  DataRecvCallback callback = (DataRecvCallback)parameter;

  uint8_t buffer[250]; // Define a buffer to store incoming data
  int bufferIndex = 0; // Index to keep track of the buffer position
  
  while (true) {
    if (dataReceived) {
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
    vTaskDelay(10 / portTICK_PERIOD_MS); // Add a small delay to yield the CPU
  }
}