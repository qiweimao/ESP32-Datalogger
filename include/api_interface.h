#ifndef API_INTERFACE_H
#define API_INTERFACE_H

#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

extern SemaphoreHandle_t logMutex;
extern AsyncWebServer server;

void start_http_server();

// Frontend Handler
void serveFile(AsyncWebServerRequest *request, const char* filePath, const char* contentType, int responseCode, bool isGzip);

// Logger Control Handler
void serveRebootLogger(AsyncWebServerRequest *request);
void pauseLoggingHandler(AsyncWebServerRequest *request);
void resumeLoggingHandler(AsyncWebServerRequest *request);

#endif