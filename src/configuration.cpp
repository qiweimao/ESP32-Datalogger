#include <Preferences.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "configuration.h"
#include "lora_init.h"

Preferences preferences;

/******************************************************************
 *                                                                *
 *                        System Config                           *
 *                                                                *
 ******************************************************************/

SystemConfig systemConfig;

void load_system_configuration() {
  Serial.println("\n*** System Configuration ***");

  preferences.begin("configurations", false);

  if (preferences.isKey("sysconfig")) {
    preferences.getBytes("sysconfig", &systemConfig, sizeof(systemConfig));
  } else {
    Serial.println("Configuration not found. Using default values.");

    strncpy(systemConfig.WIFI_SSID, "Verizon_F4ZD39", sizeof(systemConfig.WIFI_SSID) - 1);
    strncpy(systemConfig.WIFI_PASSWORD, "aft9-grid-knot", sizeof(systemConfig.WIFI_PASSWORD) - 1);
    strncpy(systemConfig.DEVICE_NAME, "DEFAULT", sizeof(systemConfig.DEVICE_NAME) - 1);

    systemConfig.utcOffset = -5;
    systemConfig.LORA_MODE = LORA_GATEWAY;
    systemConfig.PAIRING_KEY = generateRandomNumber();

    // Save default configuration
    preferences.putBytes("sysconfig", &systemConfig, sizeof(systemConfig));
  }

  preferences.end();

  Serial.println("-- Loaded System Configuration --");
  Serial.printf("Device Name: %s\n", systemConfig.DEVICE_NAME);
  Serial.printf("WIFI_SSID: %s\n", systemConfig.WIFI_SSID);
  Serial.printf("WIFI_PASSWORD: %s\n", systemConfig.WIFI_PASSWORD);
  Serial.printf("Boot as: %s\n", systemConfig.LORA_MODE ? "Gateway" : "Node");
  Serial.printf("PAIRING_KEY: %lu\n", systemConfig.PAIRING_KEY);
  Serial.printf("utcOffset: %d\n", systemConfig.utcOffset);
}

void update_system_configuration(String key, String value) {
  Serial.println("Updating system configuration...");

  preferences.begin("credentials", false);

  // Load existing configuration
  if (preferences.isKey("sysconfig")) {
    preferences.getBytes("sysconfig", &systemConfig, sizeof(systemConfig));
  } else {
    Serial.println("No existing configuration found.");
  }

  // Update configuration based on key
  if (key.equals("WIFI_SSID")) {
    strncpy(systemConfig.WIFI_SSID, value.c_str(), sizeof(systemConfig.WIFI_SSID) - 1);
    systemConfig.WIFI_SSID[sizeof(systemConfig.WIFI_SSID) - 1] = '\0';
  } else if (key.equals("WIFI_PASSWORD")) {
    strncpy(systemConfig.WIFI_PASSWORD, value.c_str(), sizeof(systemConfig.WIFI_PASSWORD) - 1);
    systemConfig.WIFI_PASSWORD[sizeof(systemConfig.WIFI_PASSWORD) - 1] = '\0';
  } else if (key.equals("DEVICE_NAME")) {
    if (value.length() > sizeof(systemConfig.DEVICE_NAME) - 1) {
      Serial.println("Error: DEVICE_NAME should be shorter than 16 characters.");
      preferences.end();
      return;
    }
    strncpy(systemConfig.DEVICE_NAME, value.c_str(), sizeof(systemConfig.DEVICE_NAME) - 1);
    systemConfig.DEVICE_NAME[sizeof(systemConfig.DEVICE_NAME) - 1] = '\0';
  } else if (key.equals("UTC_OFFSET")) {
    systemConfig.utcOffset = value.toInt();
  } else if (key.equals("LORA_MODE")) {
    systemConfig.LORA_MODE = value.toInt();
  } else if (key.equals("PAIRING_KEY")) {
    systemConfig.PAIRING_KEY = static_cast<uint32_t>(strtoul(value.c_str(), NULL, 10));
  } else {
    Serial.println("Invalid key");
  }

  // Save updated configuration
  preferences.putBytes("sysconfig", &systemConfig, sizeof(systemConfig));

  preferences.end();

  load_system_configuration(); // reload configuration
}


/******************************************************************
 *                                                                *
 *                    Data Collection Config                      *
 *                                                                *
 ******************************************************************/

DataCollectionConfig dataConfig;

void printDataConfig() {
  Serial.println("\n*** Data Collection Configuration ***");

  // Print ADC configuration
  for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
    Serial.printf("ADC Channel %d: Enabled=%s, Interval=%d, SensorType=%d\n",
                  i, dataConfig.adcEnabled[i] ? "true" : "false",
                  dataConfig.adcInterval[i], dataConfig.adcSensorType[i]);
  }

  // Print UART configuration
  for (int i = 0; i < UART_CHANNEL_COUNT; i++) {
    Serial.printf("UART Channel %d: Enabled=%s, Interval=%d, SensorType=%d\n",
                  i, dataConfig.uartEnabled[i] ? "true" : "false",
                  dataConfig.uartInterval[i], dataConfig.uartSensorType[i]);
  }

  // Print I2C configuration
  for (int i = 0; i < I2C_CHANNEL_COUNT; i++) {
    Serial.printf("I2C Channel %d: Enabled=%s, Interval=%d, SensorType=%d\n",
                  i, dataConfig.i2cEnabled[i] ? "true" : "false",
                  dataConfig.i2cInterval[i], dataConfig.i2cSensorType[i]);
  }
}

void loadDataConfigFromPreferences() {
  preferences.begin("configurations", false);
  if (preferences.isKey("dataconfig")) {
    preferences.getBytes("dataconfig", &dataConfig, sizeof(dataConfig));
  } else {
    Serial.println("Data collection configuration not found. Using default values.");

    // Initialize with default values
    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
      dataConfig.adcSensorType[i] = Unknown;
      dataConfig.adcEnabled[i] = false;
      dataConfig.adcInterval[i] = 60;
    }

    for (int i = 0; i < UART_CHANNEL_COUNT; i++) {
      dataConfig.uartSensorType[i] = VibratingWire;
      dataConfig.uartEnabled[i] = false;
      dataConfig.uartInterval[i] = 60;
    }

    for (int i = 0; i < I2C_CHANNEL_COUNT; i++) {
      dataConfig.i2cSensorType[i] = Barometric;
      dataConfig.i2cEnabled[i] = false;
      dataConfig.i2cInterval[i] = 60;
    }

    // Save default configuration to preferences
    preferences.putBytes("dataconfig", &dataConfig, sizeof(dataConfig));
  }
  preferences.end();

  printDataConfig();
}

void updateDataCollectionConfiguration(String type, int index, String key, String value) {
  Serial.println("Updating data collection configuration...");

  if (type.equals("ADC") && index >= 0 && index < 16) {
    if (key.equals("enabled")) {
      dataConfig.adcEnabled[index] = (value.equals("true"));
      Serial.println(value.equals("true"));
      Serial.println("Updated adc to true");
    } else if (key.equals("interval")) {
      dataConfig.adcInterval[index] = value.toInt();
    } else if (key.equals("sensorType")) {
      if (value.equals("Unknown")) {
        dataConfig.adcSensorType[index] = Unknown;
      } else if (value.equals("VibratingWire")) {
        dataConfig.adcSensorType[index] = VibratingWire;
      } else if (value.equals("Barometric")) {
        dataConfig.adcSensorType[index] = Barometric;
      }
    }
  } else if (type.equals("UART") && index >= 0 && index < 2) {
    if (key.equals("enabled")) {
      dataConfig.uartEnabled[index] = (value.equals("true"));
    } else if (key.equals("interval")) {
      dataConfig.uartInterval[index] = value.toInt();
    } else if (key.equals("sensorType")) {
      if (value.equals("Unknown")) {
        dataConfig.uartSensorType[index] = Unknown;
      } else if (value.equals("VibratingWire")) {
        dataConfig.uartSensorType[index] = VibratingWire;
      } else if (value.equals("Barometric")) {
        dataConfig.uartSensorType[index] = Barometric;
      }
    }
  } else if (type.equals("I2C") && index >= 0 && index < 5) {
    if (key.equals("enabled")) {
      dataConfig.i2cEnabled[index] = (value.equals("true"));
    } else if (key.equals("interval")) {
      dataConfig.i2cInterval[index] = value.toInt();
    } else if (key.equals("sensorType")) {
      if (value.equals("Unknown")) {
        dataConfig.i2cSensorType[index] = Unknown;
      } else if (value.equals("VibratingWire")) {
        dataConfig.i2cSensorType[index] = VibratingWire;
      } else if (value.equals("Barometric")) {
        dataConfig.i2cSensorType[index] = Barometric;
      }
    }
  } else {
    Serial.println("Invalid type or index");
  }

  // Save updated configuration
  preferences.begin("configurations", false);
  if (preferences.isKey("dataconfig")) {
    preferences.putBytes("dataconfig", &dataConfig, sizeof(dataConfig));
  } else {
    Serial.println("Data collection configuration not found. Update Failed.");
  }
  preferences.end();
  printDataConfig();
  Serial.println("Finished updating data collection configuration.");
}
