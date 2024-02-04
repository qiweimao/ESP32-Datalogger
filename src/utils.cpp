#include "utils.h"

const int CS = 5;
const char *ntpServer = "pool.ntp.org";

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

void connectToWiFi() {
  // Serial.println("Connecting to WiFi");
  // WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    // Serial.println("Connecting to WiFi...");
  }

  // Serial.println("Connected to WiFi");
  // Serial.print("IP Address: ");
  // Serial.println(WiFi.localIP());
}

void reconnectToWiFi() {
  Serial.println("Reconnecting to WiFi");
  
  // Implement any necessary cleanup or reconfiguration here
  
  connectToWiFi();
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("WiFi connected");
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi disconnected");
      reconnectToWiFi();
      break;
    default:
      break;
  }
}

String getPublicIP() {
  HTTPClient http;

  // Make a GET request to api.ipify.org to get the public IP
  http.begin("https://api.ipify.org");

  // Send the request
  int httpCode = http.GET();

  // Check for a successful response
  if (httpCode == HTTP_CODE_OK) {
    String publicIP = http.getString();
    return publicIP;
  } else {
    Serial.printf("HTTP request failed with error code %d\n", httpCode);
    return "Error";
  }

  // End the request
  http.end();
}

void setupSPIFFS(){
  // Mount SPIFFS filesystem
  Serial.println("Mounting SPIFFS filesystem");
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS allocation failed");
    return;
  }

  // Check if the file already exists
  Serial.println("Checking if file exists");
  if (!SPIFFS.exists(filename)) {
    Serial.println("Doesn't exist");
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

void SD_initialize(){
  Serial.println("Initializing SD card...");
  if (!SD.begin(CS)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
}


void SDWriteFile(const char * path, const char * message){
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File myFile = SD.open(path, FILE_WRITE);
  // if the file opened okay, write to it:
  if (myFile) {
    Serial.printf("Writing to %s ", path);
    myFile.println(message);
    myFile.close(); // close the file:
    Serial.println("completed.");
  } 
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening file ");
    Serial.println(path);
  }
}

void SDReadFile(const char * path){
  // open the file for reading:
  File myFile = SD.open(path);
  if (myFile) {
     Serial.printf("Reading file from %s\n", path);
     // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close(); // close the file:
  } 
  else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
}