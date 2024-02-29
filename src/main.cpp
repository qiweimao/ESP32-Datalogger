#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HardwareSerial.h>
#include "Secrets.h"
#include "datalogging.h"
#include "utils.h"
#include "api_interface.h"
#include "VM_501.h"

/* Do not modify below */
SemaphoreHandle_t logMutex;
TaskHandle_t parsingTask; // Task handle for the parsing task
FtpServer ftpSrv;   //set #define FTP_DEBUG in ESP8266FtpServer.h to see ftp verbose on serial

/* Tasks */
void initNTPTask(void *parameter) {
  initNTP();  // Call the initNTP function
  vTaskDelete(NULL);  // Delete the task once initialization is complete
}

void logDataTask(void * parameter) {
  for (;;) {  // Infinite loop to continuously run the task
    LogErrorCode result = logData();
    // You can add error handling or other logic here if needed
  }
}

void _callback(FtpOperation ftpOperation, unsigned int freeSpace, unsigned int totalSpace){
	Serial.print(">>>>>>>>>>>>>>> _callback " );
	Serial.print(ftpOperation);
	/* FTP_CONNECT,
	 * FTP_DISCONNECT,
	 * FTP_FREE_SPACE_CHANGE
	 */
	Serial.print(" ");
	Serial.print(freeSpace);
	Serial.print(" ");
	Serial.println(totalSpace);

	// freeSpace : totalSpace = x : 360

	if (ftpOperation == FTP_CONNECT) Serial.println(F("CONNECTED"));
	if (ftpOperation == FTP_DISCONNECT) Serial.println(F("DISCONNECTED"));
};
void _transferCallback(FtpTransferOperation ftpOperation, const char* name, unsigned int transferredSize){
	Serial.print(">>>>>>>>>>>>>>> _transferCallback " );
	Serial.print(ftpOperation);
	/* FTP_UPLOAD_START = 0,
	 * FTP_UPLOAD = 1,
	 *
	 * FTP_DOWNLOAD_START = 2,
	 * FTP_DOWNLOAD = 3,
	 *
	 * FTP_TRANSFER_STOP = 4,
	 * FTP_DOWNLOAD_STOP = 4,
	 * FTP_UPLOAD_STOP = 4,
	 *
	 * FTP_TRANSFER_ERROR = 5,
	 * FTP_DOWNLOAD_ERROR = 5,
	 * FTP_UPLOAD_ERROR = 5
	 */
	Serial.print(" ");
	Serial.print(name);
	Serial.print(" ");
	Serial.println(transferredSize);
};

void setup() {
  /* Essentials for Remote Access */
  Serial.begin(115200);
  Serial.println("-------------------------------------\nBooting...");
  setupSPIFFS();// Setup SPIFFS -- Flash File System
  SD_initialize();//SD card file system initialization

  connectToWiFi();// Set up WiFi connection
  WiFi.onEvent(WiFiEvent);// Register the WiFi event handler
  startServer();// start Async server with api-interfaces

  ftpSrv.setCallback(_callback);
  ftpSrv.setTransferCallback(_transferCallback);

  ftpSrv.begin("esp8266","esp8266");    //username, password for ftp.   (default 21, 50009 for PASV)

  /* Logging Capabilities */
  logMutex = xSemaphoreCreateMutex();  // Mutex for current logging file
  void initVM501();
  initDS1307();// Initialize external RTC, MUST BE INITIALIZED BEFORE NTP
  initializeOLED();

  loadConfiguration();

  xTaskCreatePinnedToCore(sendCommandVM501, "ParsingTask", 4096, NULL, 1, &parsingTask, 1);
  xTaskCreatePinnedToCore(logDataTask, "logDataTask", 4096, NULL, 1, &parsingTask, 0);
  xTaskCreate(initNTPTask, "InitNTPTask", 4096, NULL, 1, NULL);
  Serial.println("-------------------------------------");
  Serial.println("Data Acquisition Started...");
}

void loop() {
  ftpSrv.handleFTP();        //make sure in loop you call handleFTP()!!

  // LogErrorCode result = logData();

  ElegantOTA.loop();

}