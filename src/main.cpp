#include "secrets.h"
#include "data_logging.h"
#include "utils.h"
#include "api_interface.h"
#include "vibrating_wire.h"
#include "lora_init.h"
#include "configuration.h"
#include "ESP-FTP-Server-Lib.h"
#include "FTPFilesystem.h"

#define FTP_USER     "esp32"
#define FTP_PASSWORD "esp32"

FTPServer ftp;

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

  sd_init();//SD card file system initialization
  load_system_configuration();
  load_data_collection_configuration();
  
  Serial.println("\n*** Utils ***");
  external_rtc_init();// Initialize external RTC, MUST BE INITIALIZED BEFORE NTP
  oled_init();
  lora_init();

  wifi_init();
  start_http_server();// start Async server with api-interfaces

  // xTaskCreate(logDataTask, "logDataTask", 4096, NULL, 1, NULL);

  ftp.addUser(FTP_USER, FTP_PASSWORD);
  ftp.addFilesystem("SD", &SD);
  ftp.addFilesystem("SPIFFS", &SPIFFS);
  ftp.begin();

  xTaskCreate(taskInitiNTP, "InitNTPTask", 4096, NULL, 1, NULL);
  /* Logging Capabilities */
  Serial.println("\n------------------Boot Completed----------------\n");
}

void loop() {
  ftp.handle();
  ElegantOTA.loop();
}