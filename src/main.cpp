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

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Set up time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
      delay(1000); // Wait for 1 second before checking again
      Serial.println("Syncing with NTP...");
  }
  Serial.println("Time configured");

  // Mount SPIFFS filesystem
  // Allocate space for SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS allocation failed");
    return;
  }

  // Serial.println("SPIFFS Mounted");
  // size_t spiffsTotal = SPIFFS.totalBytes();
  // size_t spiffsTotalMB = spiffsTotal / (1024 * 1024); // Convert bytes to megabytes

  // Serial.print("SPIFFS total size: ");
  // Serial.print(spiffsTotalMB);
  // Serial.println(" MB");
}

void loop() {
  // Call the function to print the current time in ETZ USA
  logFlashSize();
  delay(1000);
  // Other loop code, if any
}


  // struct tm timeinfo;
  // if (getLocalTime(&timeinfo)) {
  //   Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n", 
  //                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
  //                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);


void logFlashSize() {
  // Open file in append mode
  File file = SPIFFS.open(filename, "a");
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }

  // Get Current Time
  struct tm timeinfo;
  char formattedTime[25];
  if (getLocalTime(&timeinfo)) {
    strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", &timeinfo);
  }
  else{
    Serial.printf("Failed to get current time");
  }

  size_t freeSpace = ESP.getFreeSketchSpace();
  for (size_t i = 0; i < 100; i++)
  {
    file.println(String(formattedTime) + " - File Size: " + String(file.size()) + " bytes" + " - Total flash size: " + ESP.getFlashChipSize());
  }
  
  Serial.println(String(formattedTime) + " - File Size: " + String(file.size()) + " bytes" + " - Total flash size: " + ESP.getFlashChipSize());

  file.close();
}