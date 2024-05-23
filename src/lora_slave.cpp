#include "lora_init.h"

// Set your Board and Server ID 
#define BOARD_ID 1

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
      printMAC(pairingDataNode.id);
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
    previousMillis = millis();
    pairingStatus = PAIR_REQUESTED;
    Serial.println("Pairing request sent");
    break;

    case PAIR_REQUESTED:
    // time out to allow receiving response from server
    currentMillis = millis();
    if(currentMillis - previousMillis > 250) {
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
        delay(100);
        //Set values to send
        myData.msgType = DATA;
        myData.id = BOARD_ID;
        myData.temp = readDHTTemperature();
        myData.hum = readDHTHumidity();
        myData.readingId = readingId++;
        LoRa.beginPacket();
        LoRa.write((uint8_t *) &myData, sizeof(myData));
        LoRa.endPacket();
    }
  }
}

void NodeInit() {
  NodeStart = millis();

  pairingStatus = PAIR_REQUEST;

  xTaskCreate(pairingTask, "Pairing Task", 4096, NULL, 1, NULL);
}