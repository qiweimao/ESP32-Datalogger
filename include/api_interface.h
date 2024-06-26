#ifndef API_INTERFACE_H
#define API_INTERFACE_H

#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include "ArduinoJson.h"

extern SemaphoreHandle_t logMutex;
extern AsyncWebServer server;

void start_http_server();

// Frontend Handler
void serveFile(AsyncWebServerRequest *request, const char* filePath, const char* contentType, int responseCode, bool isGzip);
void serveJson(AsyncWebServerRequest *request, JsonDocument doc, int responseCode, bool isGzip);


#endif