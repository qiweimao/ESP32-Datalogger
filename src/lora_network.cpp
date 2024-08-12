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
      Serial.println(); Serial.println("=== TIME_SYNC === ");
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

void handle_collection_config_update(const uint8_t *incomingData){
  Serial.println(); Serial.println("=== UPDATE_DATA_CONFIG === ");
  collection_config_message msg;
  memcpy(&msg, incomingData, sizeof(msg));

  update_data_collection_configuration(msg.channel, "pin", msg.pin);
  update_data_collection_configuration(msg.channel, "sensor", msg.sensor);
  update_data_collection_configuration(msg.channel, "enabled", msg.enabled);
  update_data_collection_configuration(msg.channel, "interval", msg.interval);
}

void handle_system_config_update(const uint8_t *incomingData){
  Serial.println(); Serial.println("=== UPDATE_DATA_CONFIG === ");
  sysconfig_message msg;
  memcpy(&msg, incomingData, sizeof(msg));
  String key = msg.key;
  String value = msg.value;
  update_system_configuration(key, value);
}

int poll_config_flag =0 ;

void handle_config_poll(const uint8_t *incomingData){
  poll_config_flag = 1;
  Serial.println(); Serial.println("=== GET_CONFIG === ");
}

void task_handle_config_poll(void *parameter){
  while (true)
  {
    if (poll_config_flag)
    {
      Serial.println("=== data configuration ===");
      if(sendLoRaData((uint8_t *) &dataConfig, sizeof(dataConfig), "/data.conf")){
        Serial.println("Sent data collection configuration to gateway.");
      }
      Serial.println("=== sys configuration ===");
      if(sendLoRaData((uint8_t *) &systemConfig, sizeof(systemConfig), "/sys.conf")){
        Serial.println("Sent sys collection configuration to gateway.");
      }

      // send end of sync signal
      signal_message poll_complete_msg;
      memcpy(&poll_complete_msg.mac, MAC_ADDRESS_STA, MAC_ADDR_LENGTH);
      poll_complete_msg.msgType = POLL_COMPLETE;
      sendLoraMessage((uint8_t *)&poll_complete_msg, sizeof(poll_complete_msg));

      poll_config_flag = 0; // reset flag
    }
    
  }
    vTaskDelay(1 / portTICK_PERIOD_MS); // Delay for 1 second
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
  String extension = ".dat";
  memcpy(&msg.path, path.c_str(), sizeof(msg.path));
  memcpy(&msg.extension, extension.c_str(), sizeof(msg.extension));
  sendLoraMessage((uint8_t *) &msg, sizeof(msg));
  Serial.printf("Sent SYNC_FOLDER to: ");
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
  Serial.printf("Sent GET_CONFIG to: ");
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
    lora_config.deviceName = systemConfig.DEVICE_NAME;

    addHandler(&lora_config, TIME_SYNC, (LoRaMessageHandlerFunc)handle_time_sync, NULL);
    addHandler(&lora_config, GET_CONFIG, (LoRaMessageHandlerFunc)handle_config_poll, NULL);
    addHandler(&lora_config, UPDATE_DATA_CONFIG, (LoRaMessageHandlerFunc)handle_collection_config_update, NULL);
    addHandler(&lora_config, UPDATE_SYS_CONFIG, (LoRaMessageHandlerFunc)handle_system_config_update, NULL);

    if(addSchedule(&lora_config, sync_folder_request, 20000, 0) == 0){
      Serial.println("Added sync folder request handler.");
    }

    if(addSchedule(&lora_config, poll_config, 10000, 0) == 0){
      Serial.println("Added poll config handler.");
    }
    // Create the task for the control loop
    xTaskCreate(
      task_handle_config_poll,    // Task function
      "Hanle Poll Config Task",     // Name of the task
      10000,              // Stack size in words
      NULL,               // Task input parameter
      1,                  // Priority of the task
      NULL                // Task handle
    );
    Serial.println("Added Data send handler");

    if(addSchedule(&lora_config, send_time_sync_message, 10000, 1) == 0){
      Serial.println("Added send time sync handler.");
    }
    Serial.print("Schedule count: ");Serial.println(lora_config.scheduleCount);
    lora_init(&lora_config);
    return 0;
}