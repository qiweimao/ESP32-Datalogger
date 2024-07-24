#include "lora_init.h"
#include "lora_peer.h"
#include "lora_gateway.h"
#include "lora_slave.h"
#include "SD.h"

bool enableCRC = true; // Default CRC setting
SPIClass loraSpi(HSPI);// Separate SPI bus for LoRa to avoid conflict with the SD Card

volatile int dataReceived = 0;// Flag to indicate data received
int ack_count = 0;// Flag to indicate ACK for data transfer received
int rej_count = 0;// Flag to indicate REJ for data transfer received
uint8_t MAC_ADDRESS_STA[MAC_ADDR_LENGTH];
SemaphoreHandle_t xMutex_DataPoll = NULL; // mutex for LoRa hardware usage
LoRaConfig lora_config;

/******************************************************************
 *                                                                *
 *                            LORA CORE                           *
 *                                                                *
 ******************************************************************/

void lora_init(LoRaConfig *config){

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
  if (config->lora_mode == LORA_SLAVE){
    Serial.println("Lora Mode: Sender");
    lora_slave_init();
  }
  if (config->lora_mode == LORA_GATEWAY){
    Serial.println("Lora Mode: Gateway");
    lora_gateway_init();
  }

}

void sendLoraMessage(uint8_t* data, size_t size) {
    // uint8_t type = data[0];       // first message byte is the type of message 
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

/******************************************************************
 *                                                                *
 *                          AUTO PAIRING                          *
 *                                                                *
 ******************************************************************/
void handle_pairing(const uint8_t *incomingData){

  struct_pairing pairingDataGateway;
  memcpy(&pairingDataGateway, incomingData, sizeof(pairingDataGateway));

  Serial.print("\nPairing request from: ");
  printMacAddress(pairingDataGateway.mac_origin);
  Serial.println();
  Serial.println(pairingDataGateway.deviceName);

  // Check if has correct pairing key
  Serial.print("Received pairing key: ");Serial.println(pairingDataGateway.pairingKey);
  Serial.print("System pairing key: ");Serial.println(lora_config.pairingKey);
  if(pairingDataGateway.pairingKey == lora_config.pairingKey){
    Serial.println("Correct PAIRING_KEY");
    memcpy(&pairingDataGateway.mac_master, MAC_ADDRESS_STA, sizeof(MAC_ADDRESS_STA));
    sendLoraMessage((uint8_t *) &pairingDataGateway, sizeof(pairingDataGateway));
    Serial.println("Sent pairing response...");
  }
  else{
    Serial.println("Wrong PAIRING_KEY");
    return;
  }

  Serial.println("Begin adding to peer list.");

  // If first time pairing, create a dir for this node
  if(addPeerGateway(pairingDataGateway.mac_origin, pairingDataGateway.deviceName)){
    
    Serial.println("First time pairing, create a dir for this node");

    char deviceFolder[MAX_DEVICE_NAME_LEN + 1]; // 10 for the name + 1 for the null terminator
    strncpy(deviceFolder, pairingDataGateway.deviceName, 10);
    deviceFolder[MAX_DEVICE_NAME_LEN] = '\0'; // Ensure null-termination

    char folderPath[MAX_DEVICE_NAME_LEN + 6]; // 1 for '/' + 4 for 'data' + 1 for '/' + 10 for the name + 1 for the null terminator
    snprintf(folderPath, sizeof(folderPath), "/node/%s", deviceFolder);
    Serial.println(folderPath);

    if (SD.mkdir(folderPath)) {
      Serial.println("Directory created successfully.");
    } else {
      Serial.println("Failed to create directory.");
    }

    // create data folder
    char datafolderPath[MAX_DEVICE_NAME_LEN + 11]; // 10 for device name + 1 for null terminator
    snprintf(datafolderPath, sizeof(datafolderPath), "/node/%s/data", deviceFolder);
    if (SD.mkdir(datafolderPath)) {
      Serial.println("Directory /data created successfully.");
    } else {
      Serial.println("Failed to create /data directory.");
    }


    // Create subdirectories for ADC, UART, and I2C
    char subfolder[MAX_DEVICE_NAME_LEN + 20]; // 10 for device name + 1 for null terminator
    snprintf(subfolder, sizeof(subfolder), "%s/data", folderPath);

    if (SD.mkdir(subfolder)) {
      Serial.println("/data subdirectory created successfully.");
    } else {
      Serial.println("Failed to create ADC subdirectory.");
    }

    Serial.println("End of dir creation.");
  }
}

/******************************************************************
 *                                                                *
 *                      HANDLER REGISTRATION                      *
 *                                                                *
 ******************************************************************/

int addHandler(LoRaConfig *config, uint8_t messageType, LoRaMessageHandlerFunc handlerFunc_slave, LoRaMessageHandlerFunc handlerFunc_gateway) {
    if (config->handlerCount < MAX_HANDLERS) {
        config->messageHandlers[config->handlerCount].messageType = messageType;
        config->messageHandlers[config->handlerCount].slave = handlerFunc_slave;
        config->messageHandlers[config->handlerCount].gateway = handlerFunc_gateway;
        config->handlerCount++;
        return 0;
    }
    else{
      return -1;
    }
}

int addSchedule(LoRaConfig *config, LoRaPollFunc func, unsigned long interval, int isBroadcast) {
    if (config->scheduleCount < MAX_SCHEDULES) {
        config->schedules[config->scheduleCount].func = func;
        config->schedules[config->scheduleCount].isBroadcast = isBroadcast;
        config->schedules[config->scheduleCount].lastPoll = 0;
        config->schedules[config->scheduleCount].interval = interval;
        config->scheduleCount++;
        return 0;
    }
    else{
      return -1;
    }
}

LoRaMessageHandlerFunc findHandler(LoRaConfig *config, uint8_t messageType, int isSlave) {
    for (int i = 0; i < config->handlerCount; i++) {
        if (config->messageHandlers[i].messageType == messageType) {
            return isSlave ? config->messageHandlers[i].slave : config->messageHandlers[i].gateway;
        }
    }
    return NULL; // No handler found
}