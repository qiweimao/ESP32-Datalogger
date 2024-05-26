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

/******************************************************************
 *                                                                *
 *                           Gateway                              *
 *                                                                *
 ******************************************************************/

struct_message incomingReadings;
struct_message outgoingSetpoints;
struct_pairing pairingDataGateway;

#define MAX_PEERS 10 // Define the maximum number of peers
#define PEER_ADDR_LENGTH 6 // Define the length of the peer address
uint8_t peers[MAX_PEERS];// Array to store peer addresses
int peerCount = 0;// Counter to keep track of the number of peers

// Function to add a peer gateway
bool addPeerGateway(const uint8_t peer_addr) {
  if (peerCount >= MAX_PEERS) {  // Check if we have space for more peers
    Serial.println("Max peers reached. Cannot add more.");
    return false;
  }
  peers[peerCount] = peer_addr;  // Copy the peer address to the peers array
  peerCount++;  // Increment the peer count
  Serial.println();
  return true;
}

void OnDataRecvGateway(const uint8_t *incomingData, int len) { 
  StaticJsonDocument<1000> root;
  String payload;
  uint8_t type = incomingData[0];       // first message byte is the type of message 
  switch (type) {
    case DATA :                           // the message is data type
      memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
      root["id"] = incomingReadings.id;      // create a JSON document with received data and send it by event to the web page
      root["temperature"] = incomingReadings.temp;
      root["humidity"] = incomingReadings.hum;
      root["readingId"] = String(incomingReadings.readingId);
      serializeJson(root, payload);
      oled_print(payload.c_str());
      serializeJson(root, Serial);
      Serial.println();
      break;
    
    case PAIRING:                            // the message is a pairing request 
      memcpy(&pairingDataGateway, incomingData, sizeof(pairingDataGateway));

      Serial.println(pairingDataGateway.msgType);
      Serial.print("Pairing request from: ");
      Serial.println(pairingDataGateway.id);

      /* OLED for Dev */
      oled_print(pairingDataGateway.msgType);
      oled_print(pairingDataGateway.id);
      oled_print("Pairing request from: ");

      if (pairingDataGateway.id > 0) {     // do not replay to server itself
        if (pairingDataGateway.msgType == PAIRING) { 
            pairingDataGateway.id = 0;       // 0 is server
            Serial.println("send response");
            oled_print("send response");
            Serial.println("Sending packet...");
            LoRa.beginPacket();
            LoRa.write((uint8_t *) &pairingDataGateway, sizeof(pairingDataGateway));
            LoRa.endPacket();
            addPeerGateway(pairingDataGateway.id);
            LoRa.receive();
        }  
      }  
      break; 
  }
}

void lora_gateway_init() {

  LoRa.onReceive(onReceive);
  LoRa.onTxDone(onTxDone);
  LoRa_rxMode();

  xTaskCreate(taskReceive, "Data Handler", 10000, (void*)OnDataRecvGateway, 1, NULL);

}

/******************************************************************
 *                                                                *
 *                            Slave                               *
 *                                                                *
 ******************************************************************/


// Set your Board and Server ID 
#define BOARD_ID 0x3F

PairingStatus pairingStatus = NOT_PAIRED;

//Create 2 struct_message 
struct_message myData;  // data to send
struct_message inData;  // data received
struct_pairing pairingDataNode;

// simulate temperature and humidity data
float t = 0;
float h = 0;

unsigned long currentMillis = millis();
unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 5000;        // Interval at which to publish sensor readings
unsigned long NodeStart;                // used to measure Pairing time
unsigned int readingId = 0;   

// simulate temperature reading
float readDHTTemperature() {
  t = random(0,40);
  return t;
}

// simulate humidity reading
float readDHTHumidity() {
  h = random(0,100);
  return h;
}


void OnDataRecvNode(const uint8_t *incomingData, int len) { 
  Serial.print("Packet received from: ");
  Serial.println();
  Serial.print("data size = ");
  Serial.println(sizeof(incomingData));
  uint8_t type = incomingData[0];
  switch (type) {
  case DATA :      // we received data from server
    memcpy(&inData, incomingData, sizeof(inData));
    Serial.print("ID  = ");
    Serial.println(inData.id);
    Serial.print("Setpoint temp = ");
    Serial.println(inData.temp);
    Serial.print("SetPoint humidity = ");
    Serial.println(inData.hum);
    Serial.print("reading Id  = ");
    Serial.println(inData.readingId);

    break;

  case PAIRING:    // we received pairing data from server
    memcpy(&pairingDataNode, incomingData, sizeof(pairingDataNode));
    if (pairingDataNode.id == 0) {              // the message comes from server
      Serial.print("Pairing done for ");
      Serial.print(" in ");
      Serial.print(millis()-NodeStart);
      Serial.println("ms");
      #ifdef SAVE_CHANNEL
        lastChannel = pairingDataNode.channel;
        EEPROM.write(0, pairingDataNode.channel);
        EEPROM.commit();
      #endif  
      pairingStatus = PAIR_PAIRED;             // set the pairing status
    }
    break;
  }  
}

PairingStatus autoPairing(){
  switch(pairingStatus) {
    case PAIR_REQUEST:

  // set pairing data to send to the server
    pairingDataNode.msgType = PAIRING;
    pairingDataNode.id = BOARD_ID;     

    // add peer and send request
    LoRa.beginPacket();
    LoRa.write((uint8_t *) &pairingDataNode, sizeof(pairingDataNode));
    LoRa.endPacket();
    LoRa.receive();
    previousMillis = millis();
    pairingStatus = PAIR_REQUESTED;
    Serial.println("Pairing request sent");
    break;

    case PAIR_REQUESTED:
    // time out to allow receiving response from server
    currentMillis = millis();
    if(currentMillis - previousMillis > 5000) {
      previousMillis = currentMillis;
      // time out expired,  try next channel
      pairingStatus = PAIR_REQUEST;
      Serial.println("Request again...");
    }
    break;

    case PAIR_PAIRED:
      // nothing to do here 
      Serial.println("Paired");
    break;
  }
  return pairingStatus;
}  

void pairingTask(void *pvParameters) {
  while(true){
    if (autoPairing() == PAIR_PAIRED) {
        delay(500);
        //Set values to send
        myData.msgType = DATA;
        myData.id = BOARD_ID;
        myData.temp = readDHTTemperature();
        myData.hum = readDHTHumidity();
        myData.readingId = readingId++;
        LoRa.beginPacket();
        LoRa.write((uint8_t *) &myData, sizeof(myData));
        LoRa.endPacket();
        LoRa.receive();
        Serial.printf("Sent sensor data packet...");
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // Add a small delay to yield the CPU
  }
}

void lora_slave_init() {
  NodeStart = millis();

  pairingStatus = PAIR_REQUEST;

  LoRa.onReceive(onReceive);
  LoRa.onTxDone(onTxDone);
  LoRa_rxMode();

  xTaskCreate(taskReceive, "Data Handler", 10000, (void *)OnDataRecvNode, 1, NULL);
  xTaskCreate(pairingTask, "Pairing Task", 10000, NULL, 1, NULL);

}