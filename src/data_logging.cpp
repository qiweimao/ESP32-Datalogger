#include <FS.h>
#include <SPIFFS.h>
#include "vibrating_wire.h"
#include "data_logging.h"
#include "configuration.h"
#include "utils.h"

// Sensor Libs
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

unsigned long lastLogTime[CHANNEL_COUNT] = {0};

float generateRandomFloat(float minVal, float maxVal) {
  uint32_t randomValue = esp_random();
  float scaledValue = (float)randomValue / (float)UINT32_MAX; // Scale to [0, 1]
  return minVal + scaledValue * (maxVal - minVal); // Scale to [minVal, maxVal]
}

void logDataFunction(int channel, SensorType type, time_t timestamp) {
  String extension;
  switch (type)
  {
    case VibratingWire:
      extension = String(".VibratingWire");
      break;

    case Barometric:
      extension = String(".Barometric");
      break;

    case GeoPhone:
      extension = String(".GeoPhone");
      break;

    case Inclinometer:
      extension = String(".Inclinometer");
      break;

    case RainGauege:
      extension = String(".RainGauege");
      break;
    
    default:
      extension = String(".data");
      break;
  }
  String filename = "/data/" + String(channel) + extension;
  if (!SD.exists(filename)) {
    File dataFile = SD.open(filename, FILE_WRITE);
    if (dataFile) {
        String header = "Time,Frequency (Hz),Temperature(Deg C)";
          dataFile.println(header);
      dataFile.close();
    } else {
      Serial.println("Failed to create file");
      return;
    }
  }

  File dataFile = SD.open(filename, FILE_APPEND);
  if (dataFile) {

    // dummy data
    float frequency = generateRandomFloat(7000, 8000);
    float temperature = generateRandomFloat(25, 30);
    
    String data = timestamp + "," + String(frequency) + "," + String(temperature);

    dataFile.println(data);
    dataFile.close();
    unsigned long endTime = millis(); // End timing
  } else {
    Serial.println("Failed to open file for writing");
  }

  // update latest data in dataconfig
  time_t now;
  time(&now);  // Get the current time as time_t (epoch time)
  dataConfig.time[channel] = now;

}

void logDataTask(void *parameter) {
  while (true) {
    unsigned long currentTime = millis() / 60000; // Convert milliseconds to minutes

    for (int i = 0; i < CHANNEL_COUNT; i++) {
      if (dataConfig.enabled[i] && (currentTime - lastLogTime[i] >= dataConfig.interval[i])) {
        logDataFunction(i, dataConfig.type[i], time(nullptr));
        lastLogTime[i] = currentTime;
      }
      vTaskDelay(100 / portTICK_PERIOD_MS); // Delay for 100 milliseconds
    }
  }
}

void log_data_init() {

  Serial.println("Initializing data logging.");
  // Prepare data folder for logging
  if (!SD.exists("/data")) {
    SD.mkdir("/data");
    Serial.println("Created /data directory on SD card.");
  }

  for (int i = 0; i < CHANNEL_COUNT; i++) {
    if (dataConfig.enabled[i]) {
      Serial.print("Channel: "); Serial.println(i);
    }
  }

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