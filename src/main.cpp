#include <WiFi.h>
#include <time.h>
#include <FS.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

const int LOG_INTERVAL = 60000;
// WiFi
const char *ssid = "Verizon_F4ZD39";
const char *password = "aft9-grid-knot";
// Static IP configuration
IPAddress staticIP(192, 168, 1, 100);  // Change this to your desired static IP
IPAddress gateway(192, 168, 0, 1);    // Change this to your router's IP
IPAddress subnet(255, 255, 255, 0);   // Change this to match your network's subnet mask

const char *ntpServer = "pool.ntp.org";
// const char *ntpServer = "time.google.com";
const long gmtOffset_sec = -5 * 60 * 60;  // GMT offset in seconds (Eastern Time Zone)
const int daylightOffset_sec = 3600;       // Daylight saving time offset in seconds
const char *filename = "/flash_size_log.txt";

void logFlashSize();
String getCurrentTime();
void setUpTime();
void connectToWiFi();
void WiFiEvent(WiFiEvent_t event);
void reconnectToWiFi();
void serveIndexPage(AsyncWebServerRequest *request);

// Create an instance of the server
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  // Set up WiFi connection
  connectToWiFi();

  // Register the WiFi event handler
  WiFi.onEvent(WiFiEvent);
  

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

  // Serve the text file
  Serial.println("Setting Server Callback function:");
  server.on("/all", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, filename, "text/plain");
  });

  // Serve the index.html file
  server.on("/", HTTP_GET, serveIndexPage);


  // Start server
  Serial.println("Starting server...");
  server.begin();
  Serial.println("***Serve On***");

  // Sync With NTP Time Server
  setUpTime();
}

void loop() {
  // Call the function to print the current time in ETZ USA
  logFlashSize();
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
    
  file.close();
  delay(LOG_INTERVAL);
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

void connectToWiFi() {
  Serial.println("Connecting to WiFi");
  // WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
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

void reconnectToWiFi() {
  Serial.println("Reconnecting to WiFi");
  
  // Implement any necessary cleanup or reconfiguration here
  
  connectToWiFi();
}

void serveIndexPage(AsyncWebServerRequest *request) {
  // Open the file in read mode
  File file = SPIFFS.open("/index.html", "r");

  if (!file) {
    // If the file doesn't exist, send a 404 Not Found response
    request->send(404, "text/plain", "File not found");
  } else {
    // If the file exists, read its contents and send as the response
    size_t fileSize = file.size();
    String fileContent;

    // Reserve enough space in the string for the file content
    fileContent.reserve(fileSize);

    // Read the file content into the string
    while (file.available()) {
      fileContent += char(file.read());
    }

    // Send the file content as the response with the appropriate content type
    request->send(200, "text/html", fileContent);
  }

  // Close the file
  file.close();
}