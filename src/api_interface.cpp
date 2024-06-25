#include "api_interface.h"
#include "utils.h"
#include "AsyncJson.h"
#include "configuration.h"

extern SemaphoreHandle_t logMutex;
extern bool loggingPaused;
AsyncWebServer server(80);

// React Package
void serveIndexPage(AsyncWebServerRequest *request);
void serveJS(AsyncWebServerRequest *request);
void serveCSS(AsyncWebServerRequest *request);
void serveFavicon(AsyncWebServerRequest *request);
void serveManifest(AsyncWebServerRequest *request);

// API
AsyncCallbackJsonWebHandler *updateSysConfig();
// AsyncCallbackJsonWebHandler *sendSysConfig();
AsyncCallbackJsonWebHandler *updateCollectionConfig();
// AsyncCallbackJsonWebHandler *sendCollectionConfig();

void serveGateWayMetaData(AsyncWebServerRequest *request);
void serveVoltageHistory(AsyncWebServerRequest *request);
void getSysConfig(AsyncWebServerRequest *request);
void getCollectionConfig(AsyncWebServerRequest *request);
void getNodeSysConfig(AsyncWebServerRequest *request);
void getNodeCollectionConfig(AsyncWebServerRequest *request);

void start_http_server(){
  Serial.println("\n*** Starting Server ***");
  ElegantOTA.begin(&server);

  // GET
  server.on("/", HTTP_GET, serveIndexPage);
  server.on("/main.d3e2b80d.js", HTTP_GET, serveJS);
  server.on("/main.6a3097a0.css", HTTP_GET, serveCSS);
  server.on("/favicon.ico", HTTP_GET, serveFavicon);
  server.on("/manifest.json", HTTP_GET, serveManifest);

  server.on("/api/gateway-metadata", HTTP_GET, serveGateWayMetaData);
  server.on("/api/voltage-history", HTTP_GET, serveVoltageHistory);

  // Self Configuration
  server.on("/api/system-configuration", HTTP_GET, getSysConfig);
  server.addHandler(updateSysConfig());
  server.on("/api/collection-configuration", HTTP_GET, getCollectionConfig);
  server.addHandler(updateCollectionConfig());

  // Node Configuration
  // server.on("/api/node/system-configuration", HTTP_GET, getNodeSysConfig);
  // server.addHandler(sendSysConfig());
  // server.on("/api/node/collection-configuration", HTTP_GET, getNodeCollectionConfig);
  // server.addHandler(sendCollectionConfig());

  server.on("/reboot", HTTP_GET, serveRebootLogger);// Serve the text file
  server.on("/pauseLogging", HTTP_GET, pauseLoggingHandler);
  server.on("/resumeLogging", HTTP_GET, resumeLoggingHandler);

  server.begin();  // Start server
}

// Function to handle the system configuration update
AsyncCallbackJsonWebHandler* updateSysConfig() {
  return new AsyncCallbackJsonWebHandler("/api/system-configuration/update", [](AsyncWebServerRequest *request, JsonVariant &json) {
      // Check if the json is a JsonObject
      if (!json.is<JsonObject>()) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }

      JsonObject jsonObj = json.as<JsonObject>();

      // Iterate through the key-value pairs in the JSON object
      for (JsonPair kv : jsonObj) {
        String key = kv.key().c_str();
        String value = kv.value().as<String>();

        // Call the update function with the extracted key and value
        update_system_configuration(key, value);
      }

      request->send(200, "application/json", "{\"success\":true}");
    });
}

AsyncCallbackJsonWebHandler *updateCollectionConfig (){

  return new AsyncCallbackJsonWebHandler("/api/collection-configuration/update", [](AsyncWebServerRequest *request, JsonVariant &json) {

      String type = json["type"].as<String>();
      Serial.printf("Type: %s\n", type.c_str());

      int index = json["index"].as<int>();
      Serial.printf("Index: %d\n", index);

      String key = json["key"].as<String>();
      Serial.printf("Key: %s\n", key.c_str());

      String value = json["value"].as<String>();
      Serial.printf("Value: %s\n", value.c_str());

      // Error checking inside the function below
      updateDataCollectionConfiguration(type, index, key, value);

      request->send(200); // Send an empty response with HTTP status code 200
    });
}


void serveFile(AsyncWebServerRequest *request, const char* filePath, const char* contentType, int responseCode, bool isGzip) {
  File file = SPIFFS.open(filePath, "r");
  if (!file) {
    request->send(404, "text/plain", "File not found");
    return;
  }
  AsyncWebServerResponse *response = request->beginResponse(SPIFFS, filePath, "", responseCode);
  response->addHeader("Content-Type", contentType); // Set the content type
  if (isGzip) {
    response->addHeader("Content-Encoding", "gzip");
  }
  request->send(response);
  file.close();
}

// Serves json doc as a list
void serveJson(AsyncWebServerRequest *request, JsonDocument doc, int responseCode, bool isGzip) {
  String jsonString;
  serializeJson(doc, jsonString);
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

void getSysConfig(AsyncWebServerRequest *request){
  JsonDocument doc;
  JsonObject obj1 = doc.add<JsonObject>();
  obj1["WIFI_SSID"] = systemConfig.WIFI_SSID;
  obj1["WIFI_PASSWORD"] = systemConfig.WIFI_PASSWORD;
  obj1["DEVICE_NAME"] = systemConfig.DEVICE_NAME;
  obj1["LORA_MODE"] = systemConfig.LORA_MODE;
  obj1["utcOffset"] = systemConfig.utcOffset;
  obj1["PAIRING_KEY"] = systemConfig.PAIRING_KEY;
  serveJson(request, doc, 200, false);
}

void getCollectionConfig(AsyncWebServerRequest *request) {
  JsonDocument doc;
  Serial.println("Received request for data collection configuring, ");

  // Adding ADC configurations
  JsonArray adcArray = doc["ADC"].to<JsonArray>();
  for (int i = 0; i < dataConfig.adc_channel_count; i++) {
    JsonObject adcObj = adcArray.add<JsonObject>();
    adcObj["enabled"] = dataConfig.adcEnabled[i];
    adcObj["interval"] = dataConfig.adcInterval[i];
  }

  // Adding UART configurations
  JsonArray uartArray = doc["UART"].to<JsonArray>();
  for (int i = 0; i < dataConfig.uart_channel_count; i++) {
    JsonObject uartObj = uartArray.add<JsonObject>();
    uartObj["sensorType"] = dataConfig.uartSensorType[i];
    uartObj["enabled"] = dataConfig.uartEnabled[i];
    uartObj["interval"] = dataConfig.uartInterval[i];
  }

  // Adding I2C configurations
  JsonArray i2cArray = doc["I2C"].to<JsonArray>();
  for (int i = 0; i < dataConfig.i2c_channel_count; i++) {
    JsonObject i2cObj = i2cArray.add<JsonObject>();
    i2cObj["sensorType"] = dataConfig.i2cSensorType[i];
    i2cObj["enabled"] = dataConfig.i2cEnabled[i];
    i2cObj["interval"] = dataConfig.i2cInterval[i];
  }

  // Serve the JSON document
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