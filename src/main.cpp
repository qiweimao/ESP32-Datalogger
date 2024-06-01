#include "secrets.h"
#include "data_logging.h"
#include "utils.h"
#include "api_interface.h"
#include "vibrating_wire.h"
#include "lora_init.h"
#include "configuration.h"

/* Tasks */
SemaphoreHandle_t logMutex;
TaskHandle_t parsingTask; // Task handle for the parsing task
TaskHandle_t wifimanagerTaskHandle; // Task handle for the parsing task
TaskHandle_t blinkTaskHandle; // Task handle for the parsing task

void taskInitiNTP(void *parameter) {
  ntp_sync();  // Call the initNTP function
  vTaskDelete(NULL);  // Delete the task once initialization is complete
}

void logDataTask(void *parameter) {
  while(true){
    LogErrorCode result = logData();
  }
}

void setup() {

  Serial.begin(115200);
  Serial.println("\n------------------Booting-------------------\n");

  /* Core System */
  Serial.println("*** Core System ***");
  wifi_setting_reset();
  pinMode(TRIGGER_PIN, INPUT_PULLUP);// Pin setting for wifi manager push button
  pinMode(LED,OUTPUT);// onboard blue LED inidcator
  spiffs_init();// Setup SPIFFS -- Flash File System
  external_rtc_init();// Initialize external RTC, MUST BE INITIALIZED BEFORE NTP
  sd_init();//SD card file system initialization
  load_system_configuration();
  load_data_collection_configuration();
  Serial.println("\n*** Utils ***");
  oled_init();
  lora_init();
  wifi_init();
  start_http_server();// start Async server with api-interfaces
  ftp_init();
  // xTaskCreate(logDataTask, "logDataTask", 4096, NULL, 1, NULL);
  xTaskCreate(taskInitiNTP, "InitNTPTask", 4096, NULL, 1, NULL);

  Serial.println("\n------------------Boot Completed----------------\n");
}

void loop() {
  ElegantOTA.loop();
  ftpSrv.handleFTP();        //make sure in loop you call handleFTP()!!
}