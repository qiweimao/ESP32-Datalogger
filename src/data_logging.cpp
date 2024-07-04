#include <FS.h>
#include <SPIFFS.h>
#include "vibrating_wire.h"
#include "data_logging.h"
#include "configuration.h"
#include "utils.h"

// Sensor Libs
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

unsigned long lastLogTimeADC[CHANNEL_COUNT] = {0};

bool loggingPaused = false;

String createFilename(String type, int channel) {
  String filename = "/data/" + String(channel) + ".dat";
  Serial.print(filename);
  return filename;
}

void logDataFunction(int channel, String timestamp) {
  String filename = createFilename("ADC", channel);
  Serial.print(" Opened.");
  if (!SD.exists(filename)) {
    File dataFile = SD.open(filename, FILE_WRITE);
    if (dataFile) {
      dataFile.close();
    } else {
      Serial.println("Failed to create file");
      return;
    }
  }
  unsigned long startTime = millis(); // Start timing
  File dataFile = SD.open(filename, FILE_APPEND);
  if (dataFile) {
    String data = timestamp + "," + String(channel) + ",ADC data";
    dataFile.println(data);
    dataFile.close();
    unsigned long endTime = millis(); // End timing
    Serial.print("Time taken for append operation: ");
    Serial.print(endTime - startTime);
    Serial.print(" ms.");
    Serial.println(" Closed ");
  } else {
    Serial.println("Failed to open file for writing");
  }

  // update latest data in dataconfig
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  dataConfig.Value[channel] = random(0, 10000);
  dataConfig.Time[channel] = timeinfo;

}

void logDataTask(void *parameter) {
  while (true) {
    unsigned long currentTime = millis() / 60000; // Convert milliseconds to minutes

    for (int i = 0; i < CHANNEL_COUNT; i++) {
      if (dataConfig.Enabled[i] && (currentTime - lastLogTimeADC[i] >= dataConfig.Interval[i])) {
        logDataFunction(i, get_current_time(true));
        lastLogTimeADC[i] = currentTime;
      }
      vTaskDelay(100 / portTICK_PERIOD_MS); // Delay for 100 milliseconds
    }
  }
}

void log_data_init() {
  Serial.println("Initializing data logging.");

  if (!SD.exists("/data")) {
    SD.mkdir("/data");
    Serial.println("Created /data directory on SD card.");
  }

  unsigned long currentTime = millis() / 60000; // Convert milliseconds to minutes

  for (int i = 0; i < CHANNEL_COUNT; i++) {
    if (dataConfig.Enabled[i]) {
      Serial.println("ADC channel: is enabled");
      Serial.println(i);
      logDataFunction(i, get_current_time(true));
      lastLogTimeADC[i] = currentTime;
    }
  }


  Serial.println("Finished Initial Scan.");

  xTaskCreate(
    logDataTask,        // Task function
    "Log Data Task",    // Name of the task (for debugging)
    10000,              // Stack size (in words, not bytes)
    NULL,               // Task input parameter
    1,                  // Priority of the task
    NULL                // Task handle
  );
  Serial.println("Added Data Logging Task.");
}
