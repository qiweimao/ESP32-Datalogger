#include <FS.h>
#include <SPIFFS.h>
#include "utils.h"
#include "VM_501.h"
#include "datalogging.h"

bool loggingPaused = true;
int LOG_INTERVAL = 3000;

LogErrorCode logData() {
  
  if(loggingPaused){
    return PAUSED;
  }

  File file;
  if (!SD.exists(filename)) {
    Serial.println("File does not exist");
    const char* message = "Time, Frequency (Hz)\n";
    writeFile(SD, filename, message);
  }

  String data = readVM();
  DateTime now = rtc.now();

  // Format the data row
  String row = String(now.year()) + "-" + String(now.month()) + "-" + String(now.day()) + " " +
               String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + ", " +
               data + "\n";
  Serial.println(row);
  appendFile(SD, filename, row.c_str());

  delay(LOG_INTERVAL);
  return LOG_SUCCESS;
}