#include "lora_init.h"
#include "lora_gateway.h"
#include "lora_file_transfer.h"
#include "lora_peer.h"
#include "SD.h"

unsigned long lastPollTime = 0;
unsigned long lastTimeSyncTime = 0;
unsigned long lastConfigSyncTime = 0;
const unsigned long pollInterval = 60000; // 1 minute
const unsigned long timeSyncInterval = 60000;
const unsigned long configSyncInterval = 60000;

bool poll_success = false;
int rssi = 0;

/******************************************************************
 *                                                                *
 *                        Receive Control                         *
 *                                                                *
 ******************************************************************/

// *************************************
// * OnReceive Handlers Registration
// *************************************
void OnDataRecvGateway(const uint8_t *incomingData, int len) { 
  
  uint8_t type = incomingData[0];       // first message byte is the type of message 

  switch (type) {
    case PAIRING:                            // the message is a pairing request 
      handle_pairing(incomingData);
      break;
    case FILE_BODY:
      // Serial.println("Received FILE_BODY");
        handle_file_body(incomingData);
      break;
    case FILE_ENTIRE:
      // Serial.println("Received FILE_ENTIRE");
      handle_file_entire(incomingData);
      break;
    case POLL_COMPLETE:
      // Serial.println("Received POLL_COMPLETE");
      rssi = LoRa.packetRssi();
      poll_success = true;
      break;
    default:
      Serial.println("Unkown message type.");
  }
}

/******************************************************************
 *                                                                *
 *                         Send Control                           *
 *                                                                *
 ******************************************************************/


int waitForPollAck() {
  unsigned long startTime = millis();
  while (millis() - startTime < POLL_TIMEOUT) {
    if(poll_success){
      poll_success = false;
      return true;
    }
    vTaskDelay(1 / portTICK_PERIOD_MS); // Delay for 1 second
  }
  return false;
}

/******************************************************************
 *                         Control Tasks                          *
 ******************************************************************/
void scheduled_poll(void *parameter){

  while(true){

    unsigned long currentTime = millis();

    if(peerCount == 0){
      vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second
      continue;
    }

    for (int i = 0; i < lora_config.scheduleCount; i++){
      LoRaPollSchedule schedule = lora_config.schedules[i];
      LoRaPollFunc func = schedule.func;
      unsigned long lastPoll = schedule.lastPoll;
      unsigned long interval = schedule.interval;
      int isBroadcast = schedule.isBroadcast;

      if ((currentTime - lastPoll) >= interval) {
        
        Serial.println();Serial.println("================================");
        Serial.print("Beging Schedule: "); Serial.print(i);
        Serial.print(", lastPoll: "); Serial.print(lastPoll);
        Serial.print(", interval: "); Serial.print(interval);
        Serial.print(", currentTime: "); Serial.println(currentTime);

        lora_config.schedules[i].lastPoll = currentTime;
        for(int j = 0; j < peerCount; j++){
          
          rej_switch = 0; // turn on file transfer

          if (xSemaphoreTake(xMutex_DataPoll, portMAX_DELAY) == pdTRUE) {
            poll_success = false;
            func(j);
            xSemaphoreGive(xMutex_DataPoll);
          }

          if(isBroadcast){
            Serial.println("Is broadcast, no need to wait for confirmation.");
            continue;
          }

          if(waitForPollAck()){ // check for ack before proceeding to next one
            struct tm timeinfo;
            getLocalTime(&timeinfo);
            peers[j].lastCommTime = timeinfo;
            peers[j].status = ONLINE;
            peers[j].SignalStrength = rssi;
            Serial.println("Received poll success.");
            Serial.println("Updated LoRa communication stats.");
          }
          else{ // still in transmission or slave is non responsive

            // signal_message ackMessage_gateway;
            // ackMessage_gateway.msgType = REJ;
            // memcpy(&ackMessage_gateway.mac, peers[j].mac, sizeof(ackMessage_gateway.mac));
            // printMacAddress(peers[j].mac);
            
            // if (xSemaphoreTake(xMutex_LoRaHardware, portMAX_DELAY) == pdTRUE) {
            //   sendLoraMessage((uint8_t *) &ackMessage_gateway, sizeof(ackMessage_gateway));
            //   xSemaphoreGive(xMutex_LoRaHardware);
            //   Serial.println("Schedule Time out. Sent STOP flag to slave.");
            // }
            // else{
            //   Serial.println("Failed to Sent STOP flag to slave.");
            // }
            rej_switch = true;
            Serial.println("rej_switch = true");
            delay(1000);

          }
        }
        Serial.print("Completed Schedule: "); Serial.println(i);
      }
      
    }

    // Sleep for a short interval before next check (if needed)
    vTaskDelay(100 / portTICK_PERIOD_MS); // Delay for 1 second
  }
}

/******************************************************************
 *                                                                *
 *                        Initialization                          *
 *                                                                *
 ******************************************************************/

void lora_gateway_init() {

  LoRa.onReceive(onReceive);
  LoRa.receive();

  loadPeersFromSD();

  // Ensure the "data" directory exists
  if (!SD.exists("/node")) {
    Serial.print("Creating 'node' directory - ");
    if (SD.mkdir("/node")) {
      Serial.println("OK");
    } else {
      Serial.println("Failed to create 'node' directory");
    }
  }
  
  Serial.println("Finished checking node dir");

  // Create the task for the receive loop
  xTaskCreate(
    taskReceive,
    "Data Receive Handler",
    10000,
    (void*)OnDataRecvGateway,
    3,
    NULL
  );

  Serial.println("Added Data receieve handler");

  // Create the task for the control loop
  xTaskCreate(
    scheduled_poll,    // Task function
    "Control Task",     // Name of the task
    10000,              // Stack size in words
    NULL,               // Task input parameter
    1,                  // Priority of the task
    NULL                // Task handle
  );
  Serial.println("Added Data send handler");

  // sync_folder_request(peers[0].mac);

  
}