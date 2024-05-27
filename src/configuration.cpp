#include <Preferences.h>
#include "configuration.h"

Preferences preferences;

String WIFI_SSID;
String WIFI_PASSWORD;
int LORA_MODE; // default set up upon flashing
String DEVICE_NAME;
long gmtOffset_sec;  // GMT offset in seconds (Eastern Time Zone)

void load_system_configuration(){
  Serial.println("Loading configuration...");
  
  preferences.begin("credentials", false);
  
  if (preferences.isKey("WIFI_SSID")) {
    WIFI_SSID = preferences.getString("WIFI_SSID", "Verizon_F4ZD39");
  } else {
    Serial.println("WiFi SSID not found. Using default value.");
    WIFI_SSID = "Verizon_F4ZD39";
    preferences.putString("WIFI_SSID", WIFI_SSID);
  }

  if (preferences.isKey("WIFI_PASSWORD")) {
    WIFI_PASSWORD = preferences.getString("WIFI_PASSWORD", "aft9-grid-knot");
  } else {
    Serial.println("WiFi Password not found. Using default value.");
    WIFI_PASSWORD = "aft9-grid-knot";
    preferences.putString("WIFI_PASSWORD", WIFI_PASSWORD);
  }

  if (preferences.isKey("PROJECT_NAME")) {
    String projectName = preferences.getString("PROJECT_NAME", "new-project");
  } else {
    Serial.println("PROJECT_NAME not found. Using default value.");
    String projectName = "new-project";
    preferences.putString("PROJECT_NAME", projectName);
  }

  if (preferences.isKey("GMT_OFFSET_SEC")) {
    gmtOffset_sec = preferences.getLong("GMT_OFFSET_SEC", gmtOffset_sec);
  } else {
    Serial.println("GMT Offset not found. Using default value.");
    gmtOffset_sec = -5 * 60 * 60;  // GMT offset in seconds (Eastern Time Zone)
    preferences.putLong("GMT_OFFSET_SEC", gmtOffset_sec);
  }

  if (preferences.isKey("LORA_MODE")) {
    LORA_MODE = preferences.getLong("LORA_MODE", LORA_GATEWAY);
    Serial.print("Boot as gateway: ");
    Serial.println(LORA_MODE);
  } else {
    Serial.println("LORA_MODE not found. Set as Responder.");
    preferences.putLong("LORA_MODE", LORA_GATEWAY);
  }

  if (preferences.isKey("DEVICE_NAME")) {
    DEVICE_NAME = preferences.getString("DEVICE_NAME", "LOGGER_00");
    Serial.printf("Device Name: %s\n", DEVICE_NAME);
  } else {
    Serial.println("DEVICE_NAME not found. Set Name as LOGGER_00.");
    preferences.putString("DEVICE_NAME", "LOGGER_00");
  }

  preferences.end();
}

void update_system_configuration(String newSSID, String newWiFiPassword, long newgmtOffset_sec, int newLORA_MODE, String newProjectName) {

  // Check if newSSID and newWiFiPassword are not empty
  if (newSSID.length() == 0 || newWiFiPassword.length() == 0) {
    Serial.println("Error: WiFi SSID or password cannot be empty.");
    return;
  }

  // Check if newLORA_MODE is within valid range
  if (newLORA_MODE < 0 || newLORA_MODE > 3) {
    Serial.println("Error: ESP-NOW mode must be an integer between 0 and 3.");
    return;
  }

  // Check if newgmtOffset_sec is within valid range
  if (newgmtOffset_sec < -43200 || newgmtOffset_sec > 43200) {
    Serial.println("Error: GMT offset must be between -43200 and 43200.");
    return;
  }

  Serial.println("Updating configuration...");
  
  preferences.begin("credentials", false);
  
  if (preferences.isKey("WIFI_SSID")) {
    preferences.putString("WIFI_SSID", newSSID);
  }

  if (preferences.isKey("PROJECT_NAME")) {
    preferences.putString("PROJECT_NAME", newProjectName);
  }

  if (preferences.isKey("WIFI_PASSWORD")) {
    preferences.putString("WIFI_PASSWORD", newWiFiPassword);
  }

  if (preferences.isKey("gmtOffset_sec")) {
    preferences.putLong("gmtOffset_sec", newgmtOffset_sec);
  }

  if (preferences.isKey("LORA_MODE")) {
    preferences.putLong("LORA_MODE", newLORA_MODE);
  }

  preferences.end();
}