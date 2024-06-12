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

  load_system_configuration();
  loadDataConfigFromPreferences();

  Serial.println("\n*** Connectivity ***");
  lora_init();
  wifi_setting_reset();
  wifi_init();
  start_http_server();// start Async server with api-interfaces
  ftp_server_init();

  xTaskCreate(taskInitiNTP, "InitNTPTask", 4096, NULL, 1, NULL);

  log_data_init();
  
  Serial.println("\n------------------Boot Completed----------------\n");
}

void loop() {
  ElegantOTA.loop();
  ftp.handle();
}