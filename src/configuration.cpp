#include <Preferences.h>
#include "configuration.h"

Preferences preferences;

/* Lists of Configuration Parameters */
String WIFI_SSID;
String WIFI_PASSWORD;
String DEVICE_NAME;
int LORA_MODE; // default set up upon flashing
int utcOffset;  // UTC offset in hours (Eastern Time Zone is -5 hours)

void load_preference(const char* key, String& value, const char* defaultValue) {
  if (preferences.isKey(key)) {
    value = preferences.getString(key, defaultValue);
  } else {
    Serial.printf("%s not found. Using default value.\n", key);
    value = defaultValue;
    preferences.putString(key, value);
  }
}

void load_preference(const char* key, int& value, int defaultValue) {
  if (preferences.isKey(key)) {
    value = preferences.getInt(key, defaultValue);
  } else {
    Serial.printf("%s not found. Using default value.\n", key);
    value = defaultValue;
    preferences.putInt(key, value);
  }
}

void load_system_configuration() {
  Serial.println("Loading configuration...");

  preferences.begin("credentials", false);

  load_preference("WIFI_SSID", WIFI_SSID, "Verizon_F4ZD39");
  load_preference("WIFI_PASSWORD", WIFI_PASSWORD, "aft9-grid-knot");
  load_preference("DEVICE_NAME", DEVICE_NAME, "LOGGER_00");
  load_preference("UTC_OFFSET", utcOffset, -5);
  load_preference("LORA_MODE", LORA_MODE, LORA_GATEWAY);

  Serial.printf("Boot as: %s\n", LORA_MODE ? "Gateway": "Node");
  Serial.printf("Device Name: %s\n", DEVICE_NAME);

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

void update_preference(const char* key, const String& value) {
  preferences.putString(key, value);
}

void update_preference(const char* key, int value) {
  preferences.putInt(key, value);
}

void update_system_configuration(String newSSID, String newWiFiPassword, int newUtcOffset, int newLORA_MODE, String newProjectName) {
  if (!is_valid_configuration(newSSID, newWiFiPassword, newUtcOffset, newLORA_MODE)) {
    return;
  }

  Serial.println("Updating configuration...");

  preferences.begin("credentials", false);

  update_preference("WIFI_SSID", newSSID);
  update_preference("WIFI_PASSWORD", newWiFiPassword);
  update_preference("UTC_OFFSET", newUtcOffset);
  update_preference("LORA_MODE", newLORA_MODE);
  update_preference("PROJECT_NAME", newProjectName);

  preferences.end();
}
