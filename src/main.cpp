#include "data_logging.h"
#include "utils.h"
#include "api_interface.h"
#include "vibrating_wire.h"
#include "lora_network.h"
#include "configuration.h"
#include "database.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>

// #include "FreeRTOS.h"

/* Tasks */
SemaphoreHandle_t logMutex;
TaskHandle_t parsingTask; // Task handle for the parsing task
TaskHandle_t wifimanagerTaskHandle; // Task handle for the parsing task
TaskHandle_t blinkTaskHandle; // Task handle for the parsing task

void taskInitiNTP(void *parameter) {
  ntp_sync();  // Call the initNTP function
  vTaskDelete(NULL);  // Delete the task once initialization is complete
}



void logRAMInfo() {
    // Get total heap size
    size_t totalHeapSize = heap_caps_get_total_size(MALLOC_CAP_8BIT);

    // Get free heap size
    size_t freeHeapSize = heap_caps_get_free_size(MALLOC_CAP_8BIT);

    // Get minimum ever free heap size (i.e., the lowest recorded free heap size)
    size_t minEverFreeHeapSize = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);

    // Calculate used heap size
    size_t usedHeapSize = totalHeapSize - freeHeapSize;

    // Calculate used heap percentage
    float usedHeapPercentage = ((float)usedHeapSize / totalHeapSize) * 100.0;

    // Log RAM information
    // Serial.printf("Total Heap Size: %u bytes\n", totalHeapSize);
    // Serial.printf("Free Heap Size: %u bytes\n", freeHeapSize);
    // Serial.printf("Used Heap Size: %u bytes\n", usedHeapSize);
    Serial.printf("Used Heap Percentage: %.2f%%\n", usedHeapPercentage);
    // Serial.printf("Minimum Ever Free Heap Size: %u bytes\n", minEverFreeHeapSize);
}


// void logTaskInfo() {
//     // Get the number of tasks
//     UBaseType_t numTasks = uxTaskGetNumberOfTasks();

//     // Allocate memory for task status array
//     TaskStatus_t *taskStatusArray = (TaskStatus_t *)pvPortMalloc(numTasks * sizeof(TaskStatus_t));

//     if (taskStatusArray != NULL) {
//         // Get task status information
//         numTasks = uxTaskGetSystemState(taskStatusArray, numTasks, NULL);

//         // Log number of tasks
//         Serial.printf("Number of Tasks: %u\n", numTasks);

//         // Log RAM usage for each task
//         for (UBaseType_t i = 0; i < numTasks; i++) {
//             Serial.printf("Task: %s, RAM Used: %u bytes\n", taskStatusArray[i].pcTaskName, taskStatusArray[i].usStackHighWaterMark);
//         }

//         // Free the memory allocated for the task status array
//         vPortFree(taskStatusArray);
//     } else {
//         Serial.println("Failed to allocate memory for task status array");
//     }
// }

void monitorTask(void *pvParameters) {
    (void) pvParameters;

    while (1) {
        logRAMInfo();
        // logTaskInfo();

        // Wait for 5 seconds
        vTaskDelay(pdMS_TO_TICKS(1000));
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
  load_data_collection_configuration();
  load_influx_db_configuration();


  Serial.println("\n*** Connectivity ***");

  wifi_init();
  xTaskCreate(taskInitiNTP, "InitNTPTask", 4096, NULL, 1, NULL);
  start_http_server();// start Async server with api-interfaces
  ftp_server_init();
  // setupInfluxDB();
  lora_initialize();
  log_data_init();
  xTaskCreate(monitorTask, "MonitorTask", 4096, NULL, 1, NULL);

  Serial.println("\n------------------Boot Completed----------------\n");
}

void loop() {
  ElegantOTA.loop();
  ftp.handle();
}