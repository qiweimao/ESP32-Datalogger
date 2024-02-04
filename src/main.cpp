// #include <WiFi.h>
// #include <time.h>
#include <FS.h>
// #include <SPIFFS.h>
// #include <SPI.h>
// #include <SD.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "Secrets.h"
#include "aws_mqtt.h"
#include "datalogging.h"
#include "utils.h"

const char *AWSEC2 = "lelelumon.shop";
const char *filename = "/flash_size_log.txt";
const long gmtOffset_sec = -5 * 60 * 60;  // GMT offset in seconds (Eastern Time Zone)
const int daylightOffset_sec = 3600;  
const int LOG_INTERVAL = 1000;

void serveIndexPage(AsyncWebServerRequest *request);
void serveCompleteFile(AsyncWebServerRequest *request);

// Create an instance of the server
AsyncWebServer server(80);

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

void setup() {
  Serial.begin(115200);
  connectToWiFi();// Set up WiFi connection
  WiFi.onEvent(WiFiEvent);// Register the WiFi event handler
  setupSPIFFS();// Setup SPIFFS

  SD_initialize();
  SDWriteFile("/test.txt", "ElectronicWings.com");
  SDReadFile("/test.txt");

  server.on("/all", HTTP_GET, serveCompleteFile);// Serve the text file
  server.on("/", HTTP_GET, serveIndexPage);// Serve the index.html file
  server.begin();  // Start server
  Serial.printf("Server Started @ IP: %s\n", WiFi.localIP().toString().c_str());

  // connectAWS();

  setUpTime();// Sync With NTP Time Server

  /* Tasks */
}

void loop() {
  // Call the function to print the current time in ETZ USA
  logFlashSize();
  // publishMessage();
  // Other loop code, if any
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

void serveCompleteFile(AsyncWebServerRequest *request){
  Serial.println("Client requested complete file.");
  request->send(SPIFFS, filename, "text/plain");
}
