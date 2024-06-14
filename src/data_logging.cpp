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

String createFilename(String type, String timestamp) {
  String filename = "/data/" + type + "/" + timestamp + ".dat";
  Serial.println(filename);
  return filename;
}

void logADCData(int channel, String timestamp) {
  String filename = createFilename("ADC", timestamp);
  File dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile) {
    // Simulate reading data from the ADC channel
    String data = timestamp + "," + String(channel) + ",ADC data";
    dataFile.println(data);
    dataFile.close();
  } else {
    Serial.println("Failed to open file for writing");
  }
}

void logUARTData(int channel, String timestamp) {
  String filename = createFilename("UART", timestamp);
  File dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile) {
    // Simulate reading data from the UART channel
    String data = timestamp + "," + String(channel) + ",UART data";
    dataFile.println(data);
    dataFile.close();
  } else {
    Serial.println("Failed to open file for writing");
  }
}

void logI2CData(int channel, String timestamp) {
  String filename = createFilename("I2C", timestamp);
  File dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile) {
    // Simulate reading data from the I2C channel
    float temp = 100;
    float pressure = 200;

    String data = timestamp + "," + String(temp) + "," + String(pressure);
    dataFile.println(data);
    dataFile.close();
  } else {
    Serial.println("Failed to open file for writing");
  }
}


void logDataTask(void *parameter) {
  while (true)
  {
    // Current time
    unsigned long currentTime = millis() / 60000; // Convert milliseconds to minutes
    
    // Check and log ADC channels
    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
      if (dataConfig.adcEnabled[i] && (currentTime - lastLogTimeADC[i] >= dataConfig.adcInterval[i])) {
        logADCData(i, get_current_time(true));
        lastLogTimeADC[i] = currentTime;
      }
    }

    // Check and log UART channels
    for (int i = 0; i < UART_CHANNEL_COUNT; i++) {
      if (dataConfig.uartEnabled[i] && (currentTime - lastLogTimeUART[i] >= dataConfig.uartInterval[i])) {
        logUARTData(i, get_current_time(true));
        lastLogTimeUART[i] = currentTime;
      }
    }

    // Check and log I2C channels
    for (int i = 0; i < I2C_CHANNEL_COUNT; i++) {
      if (dataConfig.i2cEnabled[i] && (currentTime - lastLogTimeI2C[i] >= dataConfig.i2cInterval[i])) {
        logI2CData(i, get_current_time(true));
        lastLogTimeI2C[i] = currentTime;
      }
    }

    vTaskDelay(10000 / portTICK_PERIOD_MS); // Delay for 1 second
  }
}

void log_data_init(){

  Serial.println("Initializing data logging.");

  // Check and create folders if they don't exist
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

  // Current time in minutes
  unsigned long currentTime = millis() / 60000; // Convert milliseconds to minutes
  
  // Initial log for ADC channels
  for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
    if (dataConfig.adcEnabled[i]) {
      Serial.println("ADC channel: is enabled");
      Serial.println(i);
      logADCData(i, get_current_time(true));
      lastLogTimeADC[i] = currentTime;
    }
  }

  // Initial log for UART channels
  for (int i = 0; i < UART_CHANNEL_COUNT; i++) {
    if (dataConfig.uartEnabled[i]) {
      Serial.println("UART channel: is enabled");
      Serial.println(i);
      logUARTData(i, get_current_time(true));
      lastLogTimeUART[i] = currentTime;
    }
  }

  // Initial log for I2C channels
  for (int i = 0; i < I2C_CHANNEL_COUNT; i++) {
    if (dataConfig.i2cEnabled[i]) {
      Serial.println("I2C channel: is enabled");
      Serial.println(i);
      logI2CData(i, get_current_time(true));
      lastLogTimeI2C[i] = currentTime;
    }
  }

  Serial.println("Finished Initial Scan.");

  // Create the logging task
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