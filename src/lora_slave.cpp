#include "lora_slave.h"
#include "lora_peer.h"

/******************************************************************
 *                                                                *
 *                            Slave                               *
 *                                                                *
 ******************************************************************/

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
    Serial.print("MAC  = ");
    printMacAddress(inData.mac);
    Serial.print("Setpoint temp = ");
    Serial.println(inData.temp);
    Serial.print("SetPoint humidity = ");
    Serial.println(inData.hum);
    Serial.print("reading Id  = ");
    Serial.println(inData.readingId);

    break;

  case PAIRING:    // we received pairing data from server
    memcpy(&pairingDataNode, incomingData, sizeof(pairingDataNode));
    if (compareMacAddress(pairingDataNode.mac, MAC_ADDRESS_STA)) {// Is this for me?
      Serial.print("Pairing done for ");
      Serial.print(" in ");
      Serial.print(millis()-NodeStart);
      Serial.println("ms");
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
    memcpy(pairingDataNode.mac, MAC_ADDRESS_STA, sizeof(MAC_ADDRESS_STA));
    /* NEED Pairing Key Implementation */   

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
        memcpy(myData.mac, MAC_ADDRESS_STA, sizeof(MAC_ADDRESS_STA));
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