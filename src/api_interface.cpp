#include "api_interface.h"
#include "utils.h"
#include "AsyncJson.h"
#include "configuration.h"
#include "fileserver.h"
#include "LoRaLite.h"

AsyncWebServer server(80);

// React Package
void serveIndexPage(AsyncWebServerRequest *request);
void serveJS(AsyncWebServerRequest *request);
void serveCSS(AsyncWebServerRequest *request);
void serveFavicon(AsyncWebServerRequest *request);
void serveManifest(AsyncWebServerRequest *request);

// GET
void serveGateWayMetaData(AsyncWebServerRequest *request);
void serveVoltageHistory(AsyncWebServerRequest *request);
void getSysConfig(AsyncWebServerRequest *request);
void getCollectionConfig(AsyncWebServerRequest *request);
void getNodeSysConfig(AsyncWebServerRequest *request);
void getNodeCollectionConfig(AsyncWebServerRequest *request);
void serveRebootLogger(AsyncWebServerRequest *request);
void getLoRaNetworkStatus(AsyncWebServerRequest *request);

// POST
AsyncCallbackJsonWebHandler *updateSysConfig();
AsyncCallbackJsonWebHandler *updateCollectionConfig();

void start_http_server(){
  Serial.println("\n*** Starting Server ***");
  ElegantOTA.begin(&server);

// **************************************
// * GET
// **************************************
  server.on("/", HTTP_GET, serveIndexPage);
  server.on("/main.d3e2b80d.js", HTTP_GET, serveJS);
  server.on("/main.6a3097a0.css", HTTP_GET, serveCSS);
  server.on("/favicon.ico", HTTP_GET, serveFavicon);
  server.on("/manifest.json", HTTP_GET, serveManifest);

  server.on("/api/gateway-metadata", HTTP_GET, serveGateWayMetaData);
  server.on("/api/voltage-history", HTTP_GET, serveVoltageHistory);
  server.on("/api/system-configuration", HTTP_GET, getSysConfig);
  server.on("/api/collection-configuration", HTTP_GET, getCollectionConfig);
  server.on("/api/lora-network-status", HTTP_GET, getLoRaNetworkStatus);
  server.on("/reboot", HTTP_GET, serveRebootLogger);// Serve the text file

// **************************************
// * POST
// **************************************
  server.addHandler(updateSysConfig());
  server.addHandler(updateCollectionConfig());

// **************************************
// * FileServer
// **************************************
  fileserver_init();
  server.begin();  // Start server
}

/******************************************************************
 *                                                                *
 *                        Utility Functions                       *
 *                                                                *
 ******************************************************************/

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

void serveJson(AsyncWebServerRequest *request, JsonDocument doc, int responseCode, bool isGzip) {
  String jsonString;
  serializeJson(doc, jsonString);
  AsyncWebServerResponse *response = request->beginResponse(responseCode, "application/json", jsonString);
  request->send(response);
}

/******************************************************************
 *                                                                *
 *                       Serve Static Files                       *
 *                                                                *
 ******************************************************************/

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

/******************************************************************
 *                                                                *
 *                             GET                                *
 *                                                                *
 ******************************************************************/

// ***********************
// * Handle Pairing
// ***********************

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

// **************************
// * GET System Configuration
// **************************

void getSysConfig(AsyncWebServerRequest *request){
  Serial.println("Received request for System Configuration.");
  SystemConfig config;

  if (!request->hasParam("device")) {  // Check if parameter device is received
    request->send(400, "application/json", "{\"error\":\"Device query parameter is missing\"}");
  }

  String deviceName = request->getParam("device")->value();
  Serial.print("DeviceName ="); Serial.println(deviceName);
  if (deviceName == "gateway") {
    config = systemConfig; // load from systemConfig
  }
  else if(isDeviceNameValid(deviceName)){
    String filepath = "/node/" + deviceName + "/sys.conf";
    Serial.print("Substation config file path ="); Serial.println(filepath);
    File file = SD.open(filepath, FILE_READ);
    if (file) {
      file.read((uint8_t*) &config, sizeof(config)); // load from SD card
      file.close();
    }
    else{
      request->send(400, "application/json", "{\"error\":\"Configuration File not found.\"}");
    }
  } 
  else {
    // Handle case where the query parameter is missing
    request->send(400, "application/json", "{\"error\":\"Invalid Device Name\"}");
  }

  // Prepare JSON document
  JsonDocument doc;
  JsonObject obj1 = doc.to<JsonObject>();
  obj1["WIFI_SSID"] = config.WIFI_SSID;
  obj1["WIFI_PASSWORD"] = config.WIFI_PASSWORD;
  obj1["DEVICE_NAME"] = config.DEVICE_NAME;
  obj1["LORA_MODE"] = config.LORA_MODE;
  obj1["utcOffset"] = config.utcOffset;
  obj1["PAIRING_KEY"] = config.PAIRING_KEY;
  serveJson(request, doc, 200, false);

}

// ***********************************
// * GET Data Collection Configuration
// ***********************************

void getCollectionConfig(AsyncWebServerRequest *request) {
  
  Serial.println("Received request for data collection configuring, ");
  
  DataCollectionConfig config;

  if (!request->hasParam("device")){
    request->send(400, "application/json", "{\"error\":\"Device query parameter is missing\"}");
  }
    String deviceName = request->getParam("device")->value();
    
  if (deviceName == "gateway") {
    config = dataConfig;
  }
  else if(isDeviceNameValid(deviceName)){
    String filepath = "/node/" + deviceName + "/data.conf";
    File file = SD.open(filepath, FILE_READ);
    if (file) {
      file.read((uint8_t*) &config, sizeof(config));
      file.close();
    }
    else{
      request->send(400, "application/json", "{\"error\":\"Configuration File not found.\"}");
    }
  }
  else {
    // Handle case where the query parameter is missing
    request->send(400, "application/json", "{\"error\":\"Invalid Device Name\"}");
  }

  JsonDocument doc;

  // Adding ADC configurations
  JsonArray adcArray = doc.to<JsonArray>();
  for (int i = 0; i < config.channel_count; i++) {
    JsonObject adcObj = adcArray.add<JsonObject>();
    adcObj["channel"] = i;
    adcObj["pin"] = config.pin[i];
    adcObj["sensor"] = config.type[i];
    adcObj["enabled"] = config.enabled[i];
    adcObj["interval"] = config.interval[i];
    adcObj["time"] = convertTMtoString(config.time[i]);
  }

  // Serve the JSON document
  serveJson(request, doc, 200, false);

}

// ***********************************
// * Reboot Logger
// ***********************************

void serveRebootLogger(AsyncWebServerRequest *request) {
  Serial.println("Client requested ESP32 reboot.");
  request->send(200, "text/plain", "Rebooting ESP32...");
  delay(100);
  ESP.restart();
}

// ***********************************
// * LoRa Network Status
// ***********************************

void getLoRaNetworkStatus(AsyncWebServerRequest *request) {

  JsonDocument doc;

  Serial.println("Received request for lora status");

  for(int i = 0; i < peerCount; i++){
    JsonObject obj = doc.add<JsonObject>();

    char buffer[30];
    struct tm timeinfo =peers[i].lastCommTime;
    snprintf(buffer, sizeof(buffer), "%04d/%02d/%02d %02d:%02d:%02d", 
              timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
              timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    obj["name"] = peers[i].deviceName;
    obj["lastCommsTime"] = buffer;
    obj["status"] = peers[i].status;
    obj["rssi"] = peers[i].SignalStrength;
  }
  

  // Serve the JSON document
  serveJson(request, doc, 200, false);
  return;
}



/******************************************************************
 *                                                                *
 *                             POST                               *
 *                                                                *
 ******************************************************************/

// ************************************
// * Update Data Collection Settings
// ************************************

AsyncCallbackJsonWebHandler* updateCollectionConfig() {
  return new AsyncCallbackJsonWebHandler("/api/collection-configuration/update", [](AsyncWebServerRequest *request, JsonVariant &json) {

    if (!request->hasParam("device")){
      request->send(400, "application/json", "{\"error\":\"Device query parameter is missing\"}");
      return;
    }

    String deviceName = request->getParam("device")->value();
    Serial.printf("Device: %s\n", deviceName.c_str());

    int channel = json["channel"].as<int>();
    int pin = json["pin"].as<int>();
    int sensor= json["sensor"].as<int>();
    bool enabled = json["enabled"].as<int>();
    uint16_t interval = json["interval"].as<uint16_t>();
    
    Serial.printf("channel: %d\n", channel);
    Serial.printf("pin: %d\n", pin);
    Serial.printf("sensor: %d\n", sensor);
    Serial.printf("enabled: %d\n", enabled);
    Serial.printf("interval: %d\n", interval);

    // Handle different device types
    if (deviceName == "gateway") {

      // Error checking inside the function below
      updateDataCollectionConfiguration(channel, "pin", pin);
      updateDataCollectionConfiguration(channel, "sensor", sensor);
      updateDataCollectionConfiguration(channel, "enabled", enabled);
      updateDataCollectionConfiguration(channel, "interval", interval);
      request->send(200); // Send an empty response with HTTP status code 200

    }
    else if(isDeviceNameValid(deviceName)){
      // Implementation to send the configuration update to remote stations
      collection_config_message msg;
      msg.msgType = UPDATE_DATA_CONFIG;                                          // msgType
      getMacByDeviceName(deviceName, msg.mac);                            // MAC
      msg.channel = channel;
      msg.pin = pin;
      msg.sensor = (SensorType)sensor;
      msg.enabled = enabled;
      msg.interval = interval;
      sendLoraMessage((uint8_t *) &msg, sizeof(msg));
      request->send(200); // Send an empty response with HTTP status code 200
      poll_config(msg.mac);
      return;
    }
    else {
      // Handle other device types or invalid device
      request->send(400, "application/json", "{\"error\":\"Invalid device parameter\"}");
      return;
    }
  });
}


// ************************************
// * Update System Configuration
// ************************************

AsyncCallbackJsonWebHandler* updateSysConfig() {
  return new AsyncCallbackJsonWebHandler("/api/system-configuration/update", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // Check if the json is a JsonObject
    if (!json.is<JsonObject>()) {
      request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    
    String deviceName = request->getParam("device")->value();
    Serial.printf("Device: %s\n", deviceName.c_str());
    JsonObject jsonObj = json.as<JsonObject>();

    // Iterate through the key-value pairs in the JSON object
    String key, value;
    for (JsonPair kv : jsonObj) {

      key = kv.key().c_str();
      value = kv.value().as<String>();

      if (deviceName == "gateway") {
        // Call the update function with the extracted key and value
        update_system_configuration(key, value);
      }
      else if(isDeviceNameValid(deviceName)){
        // Implementation to send the configuration update to remote stations
        sysconfig_message msg;
        msg.msgType = UPDATE_SYS_CONFIG;                                          // msgType
        getMacByDeviceName(deviceName, msg.mac);                            // MAC
        strncpy(msg.key, key.c_str(), MAX_JSON_LEN_1 - 1);                    // key
        msg.key[MAX_JSON_LEN_1 - 1] = '\0'; // Null-terminate the string
        strncpy(msg.value, value.c_str(), MAX_JSON_LEN_1 - 1);                //value
        msg.value[MAX_JSON_LEN_1 - 1] = '\0'; // Null-terminate the string
        sendLoraMessage((uint8_t *) &msg, sizeof(msg));
      }
      else{
        // Handle other device types or invalid device
        request->send(400, "application/json", "{\"error\":\"Invalid device parameter\"}");
      }
    }
    
    request->send(200); // Send an empty response with HTTP status code 200

  });
}