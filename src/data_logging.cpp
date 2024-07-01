#include <FS.h>
#include <SPIFFS.h>
#include "vibrating_wire.h"
#include "data_logging.h"
#include "configuration.h"
#include "utils.h"

// Sensor Libs
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

unsigned long lastLogTimeADC[ADC_CHANNEL_COUNT] = {0};
unsigned long lastLogTimeUART[UART_CHANNEL_COUNT] = {0};
unsigned long lastLogTimeI2C[I2C_CHANNEL_COUNT] = {0};

bool loggingPaused = false;

String createFilename(String type, int channel) {
  String filename = "/data/" + type + "/" + String(channel) + ".dat";
  Serial.print(filename);
  return filename;
}

void logADCData(int channel, String timestamp) {
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
  dataConfig.adcValue[channel] = random(0, 10000);
  dataConfig.adcTime[channel] = timeinfo;

}

void logUARTData(int channel, String timestamp) {
  String filename = createFilename("UART", channel);
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
    String data = timestamp + "," + String(channel) + ",UART data";
    dataFile.println(data);
    dataFile.close();
    unsigned long endTime = millis(); // End timing
    Serial.print("Time taken for append operation: ");
    Serial.print(endTime - startTime);
    Serial.println(" ms");
  } else {
    Serial.println("Failed to open file for writing");
  }

  // update latest data in dataconfig
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  dataConfig.uartValue[channel] = random(0, 10000);
  dataConfig.uartTime[channel] = timeinfo;

}

void logI2CData(int channel, String timestamp) {
  String filename = createFilename("I2C", channel);
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
    float temp = 100;
    float pressure = 200;
    String data = timestamp + "," + String(temp) + "," + String(pressure);
    dataFile.println(data);
    dataFile.close();
    unsigned long endTime = millis(); // End timing
    Serial.print("Time taken for append operation: ");
    Serial.print(endTime - startTime);
    Serial.println(" ms");
  } else {
    Serial.println("Failed to open file for writing");
  }

  // update latest data in dataconfig
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  dataConfig.i2cValue[channel] = random(0, 10000);
  dataConfig.i2cTime[channel] = timeinfo;

}

void logDataTask(void *parameter) {
  while (true) {
    unsigned long currentTime = millis() / 60000; // Convert milliseconds to minutes

    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
      if (dataConfig.adcEnabled[i] && (currentTime - lastLogTimeADC[i] >= dataConfig.adcInterval[i])) {
        logADCData(i, get_current_time(true));
        lastLogTimeADC[i] = currentTime;
      }
      vTaskDelay(100 / portTICK_PERIOD_MS); // Delay for 100 milliseconds
    }

    for (int i = 0; i < UART_CHANNEL_COUNT; i++) {
      if (dataConfig.uartEnabled[i] && (currentTime - lastLogTimeUART[i] >= dataConfig.uartInterval[i])) {
        logUARTData(i, get_current_time(true));
        lastLogTimeUART[i] = currentTime;
      }
      vTaskDelay(100 / portTICK_PERIOD_MS); // Delay for 100 milliseconds
    }

    for (int i = 0; i < I2C_CHANNEL_COUNT; i++) {
      if (dataConfig.i2cEnabled[i] && (currentTime - lastLogTimeI2C[i] >= dataConfig.i2cInterval[i])) {
        logI2CData(i, get_current_time(true));
        lastLogTimeI2C[i] = currentTime;
      }
      vTaskDelay(100 / portTICK_PERIOD_MS); // Delay for 100 milliseconds
    }
  }
}

void log_data_init() {
  Serial.println("Initializing data logging.");

  if (!SD.exists("/data/ADC")) {
    SD.mkdir("/data/ADC");
    Serial.println("Created /ADC directory on SD card.");
  }
  if (!SD.exists("/data/UART")) {
    SD.mkdir("/data/UART");
    Serial.println("Created /UART directory on SD card.");
  }
  if (!SD.exists("/data/I2C")) {
    SD.mkdir("/data/I2C");
    Serial.println("Created /I2C directory on SD card.");
  }

  unsigned long currentTime = millis() / 60000; // Convert milliseconds to minutes

  for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
    if (dataConfig.adcEnabled[i]) {
      Serial.println("ADC channel: is enabled");
      Serial.println(i);
      logADCData(i, get_current_time(true));
      lastLogTimeADC[i] = currentTime;
    }
  }

  for (int i = 0; i < UART_CHANNEL_COUNT; i++) {
    if (dataConfig.uartEnabled[i]) {
      Serial.println("UART channel: is enabled");
      Serial.println(i);
      logUARTData(i, get_current_time(true));
      lastLogTimeUART[i] = currentTime;
    }
  }

  for (int i = 0; i < I2C_CHANNEL_COUNT; i++) {
    if (dataConfig.i2cEnabled[i]) {
      Serial.println("I2C channel: is enabled");
      Serial.println(i);
      logI2CData(i, get_current_time(true));
      lastLogTimeI2C[i] = currentTime;
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
