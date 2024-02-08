#ifndef API_INTERFACE_H
#define API_INTERFACE_H

#include <ESPAsyncWebServer.h>

extern SemaphoreHandle_t logMutex;
extern AsyncWebServer server;

void startServer();

void serveIndexPage(AsyncWebServerRequest *request);
void serveCompleteFile(AsyncWebServerRequest *request);
void serveLogList(AsyncWebServerRequest *request);
void serveRebootLogger(AsyncWebServerRequest *request);
void pauseLoggingHandler(AsyncWebServerRequest *request);
void resumeLoggingHandler(AsyncWebServerRequest *request);

#endif