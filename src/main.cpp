#include <WiFi.h>
#include <time.h>
#include <FS.h>
#include <SPIFFS.h>

const char *ssid = "Verizon_F4ZD39";
const char *password = "aft9-grid-knot";

// const char *ntpServer = "pool.ntp.org";
const char *ntpServer = "time.google.com";
const long gmtOffset_sec = -5 * 60 * 60;  // GMT offset in seconds (Eastern Time Zone)
const int daylightOffset_sec = 3600;       // Daylight saving time offset in seconds

const char *filename = "/flash_size_log.txt";

void logFlashSize();
String getCurrentTime();
void setUpTime();
void setUpWiFi();

void setup() {
  Serial.begin(115200);

  setUpWiFi();
  setUpTime();

  // Mount SPIFFS filesystem
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS allocation failed");
    return;
  }

  // Check if the file already exists
  if (!SPIFFS.exists(filename)) {
    // If the file doesn't exist, create it and write the header
    File file = SPIFFS.open(filename, "w");
    if (!file) {
      Serial.println("Failed to open file for writing");
      return;
    }
    file.println("Time, fSize, SPIFFS Used, SPIFFS Total");
    Serial.println("Time, fSize, SPIFFS Used, SPIFFS Total");
    file.close();
  }
  
}

void loop() {
  // Call the function to print the current time in ETZ USA
  logFlashSize();
  delay(100);
  // Other loop code, if any
}

void logFlashSize() {
  // Open file in append mode
  File file = SPIFFS.open(filename, "a");
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }

  String formattedTime = getCurrentTime();

  // Get FLash Space
  size_t fileSize = file.size();
  size_t spiffsTotal = SPIFFS.totalBytes();
  size_t spiffsUsed = SPIFFS.usedBytes();

  if(!file.println(String(formattedTime) + "," + String(fileSize) + "," + spiffsUsed + "," + spiffsTotal)){
    Serial.println("Error Writing file");
  }
  
  Serial.println(String(formattedTime) + "," + String(fileSize) + "," + spiffsUsed + "," + spiffsTotal);
  
  file.close();
}

String getCurrentTime() {
  struct tm timeinfo;
  
  if (getLocalTime(&timeinfo)) {
    char buffer[20]; // Adjust the buffer size based on your format
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return buffer;
  } else {
    Serial.println("Failed to get current time");
    return ""; // Return an empty string in case of failure
  }
  return ""; // Return an empty string in case of failure
}

void setUpTime(){
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
      delay(1000); // Wait for 1 second before checking again
      Serial.println("Syncing with NTP...");
  }
  Serial.println("Time configured");
}

void setUpWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}