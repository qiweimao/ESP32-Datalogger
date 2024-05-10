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
void serveVoltageHistory(AsyncWebServerRequest *request);

void startServer(){

  ElegantOTA.begin(&server);

  server.on("/", HTTP_GET, serveIndexPage);// Serve the index.html file
  server.on("/main.d3e2b80d.js", HTTP_GET, serveJS);// Serve the index.html file
  server.on("/main.6a3097a0.css", HTTP_GET, serveCSS);// Serve the index.html file
  server.on("/favicon.ico", HTTP_GET, serveFavicon);// Serve the index.html file
  server.on("/manifest.json", HTTP_GET, serveManifest);// Serve the index.html file
  server.on("/api/gateway-metadata", HTTP_GET, serveGateWayMetaData);// Serve the index.html file
  server.on("/api/voltage-history", HTTP_GET, serveVoltageHistory);// Serve the index.html file

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
  File file = SPIFFS.open(filePath, "r");
  if (!file) {
    request->send(404, "text/plain", "File not found");
    return;
  }
  AsyncWebServerResponse *response = request->beginResponse(SPIFFS, filePath, contentType, responseCode);
  if (isGzip) {
    response->addHeader("Content-Encoding", "gzip");
  }
  request->send(response);
  file.close();
}

void serveJson(AsyncWebServerRequest *request, JsonDocument doc, int responseCode, bool isGzip) {
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
  AsyncWebServerResponse *response = request->beginResponse(responseCode, "application/json", jsonString);
  request->send(response);
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
  serveFile(request, "/build/main.d3e2b80d.js.haha", "application/javascript", 200, true);
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
  JsonDocument doc;
  JsonObject obj1 = doc.add<JsonObject>();
  obj1["ip"] = WiFi.localIP().toString();
  obj1["macAddress"] = WiFi.macAddress();
  obj1["batteryVoltage"] = "3.7V";
  serveJson(request, doc, 200, false);
}

void serveVoltageHistory(AsyncWebServerRequest *request){
  JsonDocument doc;
  JsonObject obj1 = doc.add<JsonObject>();
  obj1["time"] = "2024/05/07 00:00";
  obj1["voltage"] = "3.3V";
  JsonObject obj2 = doc.add<JsonObject>();
  obj2["time"] = "2024/05/08 00:00";
  obj2["voltage"] = "3.6V";
  JsonObject obj3 = doc.add<JsonObject>();
  obj3["time"] = "2024/05/09 00:00";
  obj3["voltage"] = "3.7V";
  serveJson(request, doc, 200, false);
}


void serveRebootLogger(AsyncWebServerRequest *request) {
  Serial.println("Client requested ESP32 reboot.");
  request->send(200, "text/plain", "Rebooting ESP32...");
  delay(100);
  ESP.restart();
}

void pauseLoggingHandler(AsyncWebServerRequest *request) {
  Serial.println("Client requested to pause logging.");
  loggingPaused = true;
  request->send(200, "text/plain", "Logging paused");
}

void resumeLoggingHandler(AsyncWebServerRequest *request) {
  Serial.println("Client requested to resume logging.");
  loggingPaused = false;
  request->send(200, "text/plain", "Logging resumed");
}