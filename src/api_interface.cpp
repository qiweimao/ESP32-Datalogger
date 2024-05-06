#include "api_interface.h"
#include "utils.h"
#include "fileServer.h"

extern SemaphoreHandle_t logMutex;
extern bool loggingPaused;
AsyncWebServer server(80);

void serveIndexPage(AsyncWebServerRequest *request);
void serveJS(AsyncWebServerRequest *request);
void serveCSS(AsyncWebServerRequest *request);
void serveFavicon(AsyncWebServerRequest *request);
void serveManifest(AsyncWebServerRequest *request);
void serveGateWayMetaData(AsyncWebServerRequest *request);

void startServer(){

  ElegantOTA.begin(&server);

  server.on("/", HTTP_GET, serveIndexPage);// Serve the index.html file
  server.on("/main.8d1336c3.js", HTTP_GET, serveJS);// Serve the index.html file
  server.on("/main.6a3097a0.css", HTTP_GET, serveCSS);// Serve the index.html file
  server.on("/favicon.ico", HTTP_GET, serveFavicon);// Serve the index.html file
  server.on("/manifest.json", HTTP_GET, serveManifest);// Serve the index.html file
  server.on("/api/gateway-metadata", HTTP_GET, serveGateWayMetaData);// Serve the index.html file

  server.on("/reboot", HTTP_GET, serveRebootLogger);// Serve the text file
  server.on("/pauseLogging", HTTP_GET, pauseLoggingHandler);
  server.on("/resumeLogging", HTTP_GET, resumeLoggingHandler);
  
  startFileServer();

  server.begin();  // Start server
  Serial.printf("Server Started @ IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Public IP Address: %s\n", getPublicIP().c_str());
  Serial.printf("ESP Board MAC Address: %s\n", WiFi.macAddress().c_str());
}

void serveFile(AsyncWebServerRequest *request, const char* filePath, const char* contentType, int responseCode, bool isGzip) {
  // Open the file in read mode
  File file = SPIFFS.open(filePath, "r");

  if (!file) {
    // If the file doesn't exist, send a 404 Not Found response
    request->send(404, "text/plain", "File not found");
    return;
  }

  AsyncWebServerResponse *response = request->beginResponse(SPIFFS, filePath, contentType, responseCode);
  
  // Add Content-Encoding header if compression is required
  if (isGzip) {
    response->addHeader("Content-Encoding", "gzip");
  }

  // Send the response
  request->send(response);

  // Close the file (it will be automatically closed after the response is sent)
  file.close();
}


void serveIndexPage(AsyncWebServerRequest *request) {

  File file = SPIFFS.open("/build/index.html", "r");

  if (!file) {
    request->send(404, "text/plain", "File not found");
  } else {
    size_t fileSize = file.size();
    String fileContent;
    fileContent.reserve(fileSize);
    while (file.available()) {
      fileContent += char(file.read());
    }
    request->send(200, "text/html", fileContent);
  }
  file.close();
}

void serveJS(AsyncWebServerRequest *request) {
  serveFile(request, "/build/main.8d1336c3.js.haha", "application/javascript", 200, true);
}

void serveCSS(AsyncWebServerRequest *request) {
  serveFile(request, "/build/main.6a3097a0.css", "text/css", 200, false);
}

void serveFavicon(AsyncWebServerRequest *request) {
  serveFile(request, "/build/favicon.ico", "image/x-icon", 200, false);
}

void serveManifest(AsyncWebServerRequest *request) {
  serveFile(request, "/build/manifest.json", "application/json", 200, false);
}

void serveGateWayMetaData(AsyncWebServerRequest *request){
  request->send(200, "text/plain", WiFi.macAddress().c_str());
}

void serveRebootLogger(AsyncWebServerRequest *request) {
  Serial.println("Client requested ESP32 reboot.");

  // Send a response to the client
  request->send(200, "text/plain", "Rebooting ESP32...");

  // Delay for a short time to allow the response to be sent
  delay(100);

  // Reboot the ESP32
  ESP.restart();
}

void pauseLoggingHandler(AsyncWebServerRequest *request) {
  Serial.println("Client requested to pause logging.");
  
  // Set the flag to pause logging
  loggingPaused = true;

  // Send a response to the client
  request->send(200, "text/plain", "Logging paused");
}

void resumeLoggingHandler(AsyncWebServerRequest *request) {
  Serial.println("Client requested to resume logging.");
  
  // Set the flag to resume logging
  loggingPaused = false;

  // Send a response to the client
  request->send(200, "text/plain", "Logging resumed");
}