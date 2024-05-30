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

String WIFI_SSID;
String WIFI_PASSWORD;
String DEVICE_NAME;
int LORA_MODE; // default set up upon flashing
int utcOffset;  // UTC offset in hours (Eastern Time Zone is -5 hours)
uint32_t PAIRING_KEY;  // UTC offset in hours (Eastern Time Zone is -5 hours)

// Function to save configuration to SD card
void save_config_to_sd(const char* filename, const String& jsonConfig) {

  // Create and open the file
  File configFile = SD.open(filename, FILE_WRITE);
  if (!configFile) {
    Serial.println("Failed to create file on SD card!");
    return;
  }

  // Write the JSON configuration to the file
  configFile.print(jsonConfig);
  configFile.close();
  Serial.println("Configuration saved to SD card.");
}

void load_system_configuration() {
  Serial.println("\n*** System Configuration ***");

  preferences.begin("credentials", false);

  if (preferences.isKey("sysconfig")) {
    String jsonConfig = preferences.getString("sysconfig");
    JsonDocument doc;
    deserializeJson(doc, jsonConfig);

    WIFI_SSID = doc["WIFI_SSID"] | "Verizon_F4ZD39";
    WIFI_PASSWORD = doc["WIFI_PASSWORD"] | "aft9-grid-knot";
    DEVICE_NAME = doc["DEVICE_NAME"] | "LOGGER_00";
    utcOffset = doc["UTC_OFFSET"] | -5;
    LORA_MODE = doc["LORA_MODE"] | LORA_GATEWAY;
    PAIRING_KEY = doc["PAIRING_KEY"] | generateRandomNumber();
    
  } else {
    Serial.println("Configuration not found. Using default values.");
    WIFI_SSID = "Verizon_F4ZD39";
    WIFI_PASSWORD = "aft9-grid-knot";
    DEVICE_NAME = "DEFAULT";
    utcOffset = -5;
    LORA_MODE = LORA_GATEWAY;
    PAIRING_KEY = generateRandomNumber();

    // Save default configuration
    JsonDocument doc;
    doc["WIFI_SSID"] = WIFI_SSID;
    doc["WIFI_PASSWORD"] = WIFI_PASSWORD;
    doc["DEVICE_NAME"] = DEVICE_NAME;
    doc["UTC_OFFSET"] = utcOffset;
    doc["LORA_MODE"] = LORA_MODE;
    doc["PAIRING_KEY"] = PAIRING_KEY;

    String jsonConfig;
    serializeJson(doc, jsonConfig);
    preferences.putString("sysconfig", jsonConfig);
    save_config_to_sd("/sys_config", jsonConfig);

  }

  Serial.printf("Device Name: %s\n", DEVICE_NAME.c_str());
  Serial.printf("WIFI_SSID: %s\n", WIFI_SSID.c_str());
  Serial.printf("WIFI_PASSWORD: %s\n", WIFI_PASSWORD.c_str());
  Serial.printf("Boot as: %s\n", LORA_MODE ? "Gateway" : "Node");
  Serial.printf("PAIRING_KEY: %ld\n", PAIRING_KEY);
  Serial.printf("utcOffset: %d\n", utcOffset);

  preferences.end();
}

bool is_valid_configuration(String newSSID, String newWiFiPassword, int newUtcOffset, int newLORA_MODE) {
  if (newSSID.length() == 0 || newWiFiPassword.length() == 0) {
    Serial.println("Error: WiFi SSID or password cannot be empty.");
    return false;
  }

  if (newLORA_MODE < 0 || newLORA_MODE > 3) {
    Serial.println("Error: ESP-NOW mode must be an integer between 0 and 3.");
    return false;
  }

  if (newUtcOffset < -12 || newUtcOffset > 14) { // UTC offsets range from -12 to +14
    Serial.println("Error: UTC offset must be between -12 and 14.");
    return false;
  }

  return true;
}

void update_system_configuration(String key, String value) {
  Serial.println("Updating system configuration...");

  preferences.begin("credentials", false);

  // Load existing configuration
  String jsonConfig = preferences.getString("sysconfig");
  JsonDocument doc;
  deserializeJson(doc, jsonConfig);

  // Update configuration based on key
  if (key.equals("WIFI_SSID")) {
    doc["WIFI_SSID"] = value;
  } else if (key.equals("WIFI_PASSWORD")) {
    doc["WIFI_PASSWORD"] = value;
  } else if (key.equals("DEVICE_NAME")) {
    if (value.length() > MAX_DEVICE_NAME_LEN) {
      Serial.println("Error: DEVICE_NAME should be shorter than 9 characters.");
      preferences.end();
      return;
    }
    doc["DEVICE_NAME"] = value;
  } else if (key.equals("UTC_OFFSET")) {
    doc["UTC_OFFSET"] = value.toInt();
  } else if (key.equals("LORA_MODE")) {
    doc["LORA_MODE"] = value.toInt();
  } else if (key.equals("PAIRING_KEY")) {
    doc["PAIRING_KEY"] = static_cast<uint32_t>(strtoul(value.c_str(), NULL, 10));
  } else {
    Serial.println("Invalid key");
  }

  // Save updated configuration
  serializeJson(doc, jsonConfig);
  preferences.putString("sysconfig", jsonConfig);
  save_config_to_sd("/sys_config", jsonConfig);

  preferences.end();
  
  load_system_configuration(); // reload configuration

}



/******************************************************************
 *                                                                *
 *                    Data Collection Config                      *
 *                                                                *
 ******************************************************************/

DataCollectionConfig dataConfig;

void load_data_collection_configuration() {
  Serial.println("Loading data collection configuration...");

  preferences.begin("datacollection", false);

  if (preferences.isKey("dataconfig")) {
    String jsonConfig = preferences.getString("dataconfig");
    JsonDocument doc;
    deserializeJson(doc, jsonConfig);

    for (int i = 0; i < 16; i++) {
      dataConfig.adcSensorType[i] = doc["ADC"][i]["sensorType"] | "Unknown";
      dataConfig.adcEnabled[i] = doc["ADC"][i]["enabled"] | false;
      dataConfig.adcInterval[i] = doc["ADC"][i]["interval"] | 60;
    }

    for (int i = 0; i < 2; i++) {
      dataConfig.uartSensorType[i] = doc["UART"][i]["sensorType"] | "VW";
      dataConfig.uartEnabled[i] = doc["UART"][i]["enabled"] | false;
      dataConfig.uartInterval[i] = doc["UART"][i]["interval"] | 60;
    }

    for (int i = 0; i < 5; i++) {
      dataConfig.i2cSensorType[i] = doc["I2C"][i]["sensorType"] | "Barometric";
      dataConfig.i2cEnabled[i] = doc["I2C"][i]["enabled"] | false;
      dataConfig.i2cInterval[i] = doc["I2C"][i]["interval"] | 60;
    }

    save_config_to_sd("/collection_config", jsonConfig);

  } else {
    Serial.println("Data collection configuration not found. Using default values.");

    for (int i = 0; i < 16; i++) {
      dataConfig.adcSensorType[i] = "Unknown";
      dataConfig.adcEnabled[i] = false;
      dataConfig.adcInterval[i] = 60;
    }

    for (int i = 0; i < 2; i++) {
      dataConfig.uartSensorType[i] = "VW";
      dataConfig.uartEnabled[i] = false;
      dataConfig.uartInterval[i] = 60;
    }

    for (int i = 0; i < 5; i++) {
      dataConfig.i2cSensorType[i] = "Barometric";
      dataConfig.i2cEnabled[i] = false;
      dataConfig.i2cInterval[i] = 60;
    }

    // Save default configuration
    JsonDocument doc;
    for (int i = 0; i < 16; i++) {
      doc["ADC"][i]["sensorType"] = dataConfig.adcSensorType[i];
      doc["ADC"][i]["enabled"] = dataConfig.adcEnabled[i];
      doc["ADC"][i]["interval"] = dataConfig.adcInterval[i];
    }
    for (int i = 0; i < 2; i++) {
      doc["UART"][i]["sensorType"] = dataConfig.uartSensorType[i];
      doc["UART"][i]["enabled"] = dataConfig.uartEnabled[i];
      doc["UART"][i]["interval"] = dataConfig.uartInterval[i];
    }
    for (int i = 0; i < 5; i++) {
      doc["I2C"][i]["sensorType"] = dataConfig.i2cSensorType[i];
      doc["I2C"][i]["enabled"] = dataConfig.i2cEnabled[i];
      doc["I2C"][i]["interval"] = dataConfig.i2cInterval[i];
    }

    String jsonConfig;
    serializeJson(doc, jsonConfig);
    preferences.putString("dataconfig", jsonConfig);
    save_config_to_sd("/collection_config", jsonConfig);
  }

  preferences.end();
}

void update_data_collection_configuration(String type, int index, String key, String value) {
  Serial.println("Updating data collection configuration...");

  preferences.begin("datacollection", false);

  // Load existing configuration
  String jsonConfig = preferences.getString("dataconfig");
  JsonDocument doc;
  deserializeJson(doc, jsonConfig);

  if (type.equals("ADC") && index >= 0 && index < 16) {
    if (key.equals("enabled")) {
      doc["ADC"][index]["enabled"] = (value.equals("true"));
    } else if (key.equals("interval")) {
      doc["ADC"][index]["interval"] = value.toInt();
    } else if (key.equals("sensorType")) {
      doc["ADC"][index]["sensorType"] = value;
    }
  } else if (type.equals("UART") && index >= 0 && index < 2) {
    if (key.equals("enabled")) {
      doc["UART"][index]["enabled"] = (value.equals("true"));
    } else if (key.equals("interval")) {
      doc["UART"][index]["interval"] = value.toInt();
    } else if (key.equals("sensorType")) {
      doc["UART"][index]["sensorType"] = value;
    }
  } else if (type.equals("I2C") && index >= 0 && index < 5) {
    if (key.equals("enabled")) {
      doc["I2C"][index]["enabled"] = (value.equals("true"));
    } else if (key.equals("interval")) {
      doc["I2C"][index]["interval"] = value.toInt();
    } else if (key.equals("sensorType")) {
      doc["I2C"][index]["sensorType"] = value;
    }
  } else {
    Serial.println("Invalid type or index");
  }

  // Save updated configuration
  serializeJson(doc, jsonConfig);
  preferences.putString("dataconfig", jsonConfig);
  save_config_to_sd("/collection_config", jsonConfig);

  preferences.end();
  Serial.println("Finished updating data collection configuration.");
}