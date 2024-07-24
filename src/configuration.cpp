#include <Preferences.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "configuration.h"
#include "LoRaLite.h"
#include "utils.h"

Preferences preferences;

/******************************************************************
 *                                                                *
 *                          Save to SD Card                       *
 *                                                                *
 ******************************************************************/

void saveSystemConfigToSD() {
  File file = SD.open("/sys.conf", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open systemConfig.config for writing");
    return;
  }
  file.write((uint8_t*)&systemConfig, sizeof(systemConfig));
  file.close();
  Serial.println("System configuration saved to SD card.");
}

void saveDataConfigToSD() {
  File file = SD.open("/data.conf", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open dataConfig.config for writing");
    return;
  }
  file.write((uint8_t*)&dataConfig, sizeof(dataConfig));
  file.close();
  Serial.println("Data collection configuration saved to SD card.");
}

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

  saveSystemConfigToSD();

}

void update_system_configuration(String key, String value) {
  Serial.println("Updating system configuration...");

  preferences.begin("configurations", false);

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
  saveSystemConfigToSD();
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
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    Serial.printf("Sensor %d pin %d: Enabled=%s, interval=%d, SensorType=%d\n",
                  i, dataConfig.pin[i],dataConfig.enabled[i] ? "true" : "false",
                  dataConfig.interval[i], dataConfig.type[i]);
  }

}

void loadDataConfigFromPreferences() {
  preferences.begin("configurations", false);
  if (preferences.isKey("dataconfig")) {
    preferences.getBytes("dataconfig", &dataConfig, sizeof(dataConfig));
  } else {
    Serial.println("Data collection configuration not found. Using default values.");

    int channel_count = CHANNEL_COUNT;

    // Initialize with default values
    for (int i = 0; i < CHANNEL_COUNT; i++) {
      dataConfig.pin[i] = 0;
      dataConfig.type[i] = Unknown;
      dataConfig.enabled[i] = false;
      dataConfig.interval[i] = 60;
    }

    // Save default configuration to preferences
    preferences.putBytes("dataconfig", &dataConfig, sizeof(dataConfig));
  }
  preferences.end();
  
  saveDataConfigToSD();

  printDataConfig();
}

void updateDataCollectionConfiguration(int channel, String key, int value) {
  Serial.println("Updating data collection configuration...");

  if (!( channel >= 0 && channel < 16)){
    Serial.println("Invalid channel.");
  }

  if (key.equals("enabled")) {
    dataConfig.enabled[channel] = value;
  }
  else if (key.equals("interval")) {
    dataConfig.interval[channel] = value;
  }
  else if (key.equals("pin")) {
    dataConfig.pin[channel] = value;
  }
  else if (key.equals("sensor")) {
    dataConfig.type[channel] = (SensorType) value;
  }
  else{
    Serial.println("Invalid key.");
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

  saveDataConfigToSD();
  loadDataConfigFromPreferences(); // reload into struct after update
  Serial.println("Finished updating data collection configuration.");
}