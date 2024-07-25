// This file is used to set up the LoRaLite library and register user defined functions

#include "LoRaLite.h"
#include "utils.h"
#include "configuration.h"

/******************************************************************
 *                                                                *
 *                        Slave Handlers                          *
 *                                                                *
 ******************************************************************/

void handle_time_sync(const uint8_t *incomingData){
      Serial.print("\nTime Sync Received. ");
      time_sync_message msg;
      memcpy(&msg, incomingData, sizeof(msg));

      // Prepare a struct tm object to hold the received time
      struct tm timeinfo;
      timeinfo.tm_year = msg.year - 1900; // years since 1900
      timeinfo.tm_mon = msg.month - 1;    // months since January (0-11)
      timeinfo.tm_mday = msg.day;         // day of the month (1-31)
      timeinfo.tm_hour = msg.hour;        // hours since midnight (0-23)
      timeinfo.tm_min = msg.minute;       // minutes after the hour (0-59)
      timeinfo.tm_sec = msg.second;       // seconds after the minute (0-61, allows for leap seconds)

      // Convert struct tm to time_t
      time_t epoch = mktime(&timeinfo);

      // Update system time using settimeofday
      struct timeval tv;
      tv.tv_sec = epoch;
      tv.tv_usec = 0;
      if (settimeofday(&tv, nullptr) != 0) {
        Serial.println("Failed to set system time.");
      } else {
        Serial.println("System time updated successfully.");
      }
      
      // update RTC Time
      external_rtc_sync_ntp();

}

void handle_config_poll(const uint8_t *incomingData){
  Serial.println("=== data configuration ===");
  if(sendLoRaData((uint8_t *) &dataConfig, sizeof(dataConfig), "/data.conf")){
    Serial.println("Sent data collection configuration to gateway.");
  }
  Serial.println("=== sys configuration ===");
  if(sendLoRaData((uint8_t *) &systemConfig, sizeof(systemConfig), "/sys.conf")){
    Serial.println("Sent sys collection configuration to gateway.");
  }
}

/******************************************************************
 *                                                                *
 *                        Gateway Schedules                       *
 *                                                                *
 ******************************************************************/


// ***********************
// * Poll Data
// ***********************

// sync file with certain extension in a dir
void sync_folder_request(int index){
  
  uint8_t * mac = peers[index].mac;
  signal_message msg;
  msg.msgType = SYNC_FOLDER;
  memcpy(&msg.mac, mac, MAC_ADDR_LENGTH);
  String path = "/data";
  memcpy(&msg.path, path.c_str(), sizeof(msg.path));
  sendLoraMessage((uint8_t *) &msg, sizeof(msg));
  Serial.printf("Sent data poll message to:");
  printMacAddress(mac);Serial.println();Serial.println();

}

// ***********************
// * Poll Config
// ***********************

void poll_config(int index){

  uint8_t * mac = peers[index].mac;
  signal_message msg;
  msg.msgType = GET_CONFIG;
  memcpy(&msg.mac, mac, MAC_ADDR_LENGTH);
  sendLoraMessage((uint8_t *) &msg, sizeof(msg));
  Serial.printf("Sent config poll message to:");
  printMacAddress(mac);Serial.println();Serial.println();

}

void poll_config(uint8_t * mac){
  
  signal_message msg;
  msg.msgType = GET_CONFIG;
  memcpy(&msg.mac, mac, MAC_ADDR_LENGTH);
  sendLoraMessage((uint8_t *) &msg, sizeof(msg));
  Serial.printf("Sent config poll message to:");
  printMacAddress(mac);Serial.println();Serial.println();

}

// ***********************
// * Time Sync
// ***********************

time_sync_message get_current_time_struct() {
  time_sync_message msg;

  if (!rtc_mounted) {
    Serial.println("External RTC not mounted. Falling back to system time.");

    // Attempt to get the system time
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain system time.");
      // Return a message with an invalid type or handle this error as needed
      msg.msgType = 0xFF; // Invalid type for error indication
      return msg;
    }

    msg.msgType = TIME_SYNC;
    msg.pairingKey = systemConfig.PAIRING_KEY; // key for network
    msg.year = timeinfo.tm_year + 1900;
    msg.month = timeinfo.tm_mon + 1;
    msg.day = timeinfo.tm_mday;
    msg.hour = timeinfo.tm_hour;
    msg.minute = timeinfo.tm_min;
    msg.second = timeinfo.tm_sec;

    return msg;
  }

  DateTime now = rtc.now();

  if (now.year() < 2000) {
    Serial.println("RTC read error.");
    // Handle RTC read failure, return appropriate error message
    msg.msgType = 0xFF; // Invalid type for error indication
    return msg;
  }

  msg.msgType = TIME_SYNC; // Define TIME_SYNC
  msg.pairingKey = systemConfig.PAIRING_KEY; // key for network
  msg.year = now.year();
  msg.month = now.month();
  msg.day = now.day();
  msg.hour = now.hour();
  msg.minute = now.minute();
  msg.second = now.second();

  return msg;
}

void send_time_sync_message(int index) {
  time_sync_message msg = get_current_time_struct();

  if (msg.msgType == 0xFF) {
    // Handle error if time retrieval failed
    return;
  }

  uint8_t buffer[sizeof(time_sync_message)];
  memcpy(buffer, &msg, sizeof(time_sync_message));

  // only execute if not in data transfer mode
  sendLoraMessage(buffer, sizeof(time_sync_message));

}

/******************************************************************
 *                                                                *
 *                         INITIALIZATION                         *
 *                                                                *
 ******************************************************************/


int lora_initialize(){
    lora_config.lora_mode = systemConfig.LORA_MODE;
    lora_config.pairingKey = systemConfig.PAIRING_KEY;
    addHandler(&lora_config, TIME_SYNC, (LoRaMessageHandlerFunc)handle_time_sync, NULL);
    addHandler(&lora_config, GET_CONFIG, (LoRaMessageHandlerFunc)handle_config_poll, NULL);
    if(addSchedule(&lora_config, sync_folder_request, 60000, 0) == 0){
      Serial.println("Added sync folder request handler.");
    }
    if(addSchedule(&lora_config, poll_config, 60000, 0) == 0){
      Serial.println("Added poll config handler.");
    }
    if(addSchedule(&lora_config, send_time_sync_message, 60000, 0) == 0){
      Serial.println("Added send time sync handler.");
    }
    Serial.print("Schedule count: ");Serial.println(lora_config.scheduleCount);
    lora_init(&lora_config);
    return 0;
}