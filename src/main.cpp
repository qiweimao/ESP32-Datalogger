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

void printFileTime(time_t t) {
  struct tm *tmstruct = localtime(&t);
  Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n", 
                tmstruct->tm_year + 1900, 
                tmstruct->tm_mon + 1, 
                tmstruct->tm_mday, 
                tmstruct->tm_hour, 
                tmstruct->tm_min, 
                tmstruct->tm_sec);
}

void listFiles(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      // No more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      listFiles(entry, numTabs + 1);
    } else {
      Serial.print("\t\t");
      printFileTime(entry.getLastWrite());
    }
    entry.close();
  }
}

void setup() {

  Serial.begin(115200);
  Serial.println("\n------------------Booting-------------------\n");

  /* Core System */
  external_rtc_init();// Initialize external RTC, MUST BE INITIALIZED BEFORE NTP
  Serial.println("*** Core System ***");
  oled_init();
  esp_error_init_sd_oled();
  pinMode(TRIGGER_PIN, INPUT_PULLUP);// Pin setting for wifi manager push button
  pinMode(LED,OUTPUT);// onboard blue LED inidcator
  spiffs_init();
  sd_init();

  listFiles(SD.open("/"), 0);

  load_system_configuration();
  load_data_collection_configuration();

  Serial.println("\n*** Connectivity ***");
  lora_init();
  wifi_setting_reset();
  wifi_init();
  start_http_server();// start Async server with api-interfaces


  ftp.addUser(FTP_USER, FTP_PASSWORD);
#if defined(ESP32)
  ftp.addFilesystem("SD", &SD);
#endif
  ftp.addFilesystem("SPIFFS", &SPIFFS);

  ftp.begin();

  Serial.println("...---'''---...---'''---...---'''---...");

  xTaskCreate(taskInitiNTP, "InitNTPTask", 4096, NULL, 1, NULL);
  Serial.println("\n------------------Boot Completed----------------\n");
}

void loop() {
  ElegantOTA.loop();
  ftp.handle();
}