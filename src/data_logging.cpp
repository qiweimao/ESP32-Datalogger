#include <FS.h>
#include <SPIFFS.h>
#include "utils.h"
#include "vm_501.h"
#include "data_logging.h"

bool loggingPaused = false;
int LOG_INTERVAL = 1000;

LogErrorCode logData() {
  
  if(loggingPaused){
    return PAUSED;
  }

  File file;
  String file_name = "/"+get_current_time(1) + ".csv";
  // Serial.println(file_name);
  if (!SD.exists(file_name)) {
      Serial.println("File does not exist");
      const char* message = "Time, SensorName, Reading\n";
      writeFile(SD, file_name.c_str(), message);
  }

  String row = get_current_time(0) +  "," + "QM_PZ-01" + "," + String(random(5000)) + "\n";
  // Serial.println(row);

  appendFile(SD, file_name.c_str(), row.c_str());
  // Serial.println("Logged successfully.");

  delay(LOG_INTERVAL);
  return LOG_SUCCESS;
}