#include "api_interface.h"
#include "utils.h"
#include "fileServer.h"

extern SemaphoreHandle_t logMutex;
extern bool loggingPaused;
AsyncWebServer server(80);

void startServer(){

  TelnetStream.begin(); /* DANGEROUS */

  ElegantOTA.begin(&server);

  server.on("/", HTTP_GET, serveIndexPage);// Serve the index.html file
  server.on("/script.js", HTTP_GET, serveJS);// Serve the index.html file
  server.on("/styles.css", HTTP_GET, serveCSS);// Serve the index.html file
  server.on("/favicon.svg", HTTP_GET, servefavicon);// Serve the index.html file

  server.on("/reboot", HTTP_GET, serveRebootLogger);// Serve the text file
  server.on("/pauseLogging", HTTP_GET, pauseLoggingHandler);
  server.on("/resumeLogging", HTTP_GET, resumeLoggingHandler);
  
  startFileServer();

  server.begin();  // Start server
  Serial.printf("Server Started @ IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Public IP Address: %s\n", getPublicIP().c_str());

}

void serveIndexPage(AsyncWebServerRequest *request) {

  // Open the file in read mode
  File file = SPIFFS.open("/static/index.html", "r");

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

void serveJS(AsyncWebServerRequest *request) {

  // Open the file in read mode
  File file = SPIFFS.open("/static/script.js", "r");

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
    request->send(200, "application/javascript", fileContent);
  }

  // Close the file
  file.close();
}

void serveCSS(AsyncWebServerRequest *request) {

  // Open the file in read mode
  File file = SPIFFS.open("/static/styles.css", "r");

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
    request->send(200, "text/css", fileContent);
  }

  // Close the file
  file.close();
}

void servefavicon(AsyncWebServerRequest *request) {

  // Open the file in read mode
  File file = SPIFFS.open("/static/favicon.svg", "r");

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
    request->send(200, "image/svg+xml", fileContent);
  }

  // Close the file
  file.close();
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