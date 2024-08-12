#include "data_logging.h"
#include "utils.h"
#include "api_interface.h"
#include "vibrating_wire.h"
#include "lora_network.h"
#include "configuration.h"
#include "mqtt.h"


/* Tasks */
SemaphoreHandle_t logMutex;
TaskHandle_t parsingTask; // Task handle for the parsing task
TaskHandle_t wifimanagerTaskHandle; // Task handle for the parsing task
TaskHandle_t blinkTaskHandle; // Task handle for the parsing task

void taskInitiNTP(void *parameter) {
  ntp_sync();  // Call the initNTP function
  vTaskDelete(NULL);  // Delete the task once initialization is complete
}

void processFileTask(void * parameter) {

  while (true)
  {
    mqtt_process_folder("/data", ".dat");
    vTaskDelay(10000/portTICK_PERIOD_MS);
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

  load_system_configuration();
  loadDataConfigFromPreferences();

  Serial.println("\n*** Connectivity ***");
  // wifi_setting_reset();
  wifi_init();
  xTaskCreate(taskInitiNTP, "InitNTPTask", 4096, NULL, 1, NULL);
  start_http_server();// start Async server with api-interfaces
  ftp_server_init();
  lora_initialize();
  log_data_init();

  mqtt_initialize();
  // Create the task to process the file
  xTaskCreate(
    processFileTask,      // Task function
    "ProcessFileTask",    // Name of the task (for debugging)
    4 * 4096,                 // Stack size in words
    NULL,                 // Task input parameter
    1,                    // Priority of the task
    NULL                  // Task handle (can be NULL if not needed)
  );

  Serial.println("\n------------------Boot Completed----------------\n");
}

void loop() {
  ElegantOTA.loop();
  ftp.handle();
}