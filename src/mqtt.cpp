#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <SD.h>
#include "configuration.h"
#include "LoRaLite.h"

// Add your MQTT Broker IP address, example:
//const char* mqtt_server = "192.168.1.144";
const char* mqtt_server = "192.168.0.250";
const char* mqtt_user = "senselynk";
const char* mqtt_password = "senselynk";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
int mqtt_buffer_size = 4096;


void mqtt_reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
    }
  }
}


bool mqtt_process_file(String filename, String DeviceName) {

    // mqtt client prepare
    if (!client.connected()) {
        mqtt_reconnect();
    }
    client.loop();

    // Prepare topic
    int lastSlashIndex = filename.lastIndexOf('/');
    String extractedFilename = filename.substring(lastSlashIndex + 1);
    String topic = DeviceName + "-" + extractedFilename;

    // load meta file
    size_t lastSentPosition = 0;
    String filenameStr = filename;
    int dotIndex = filenameStr.lastIndexOf('.');
    if (dotIndex > 0) {
        filenameStr = filenameStr.substring(0, dotIndex);
    }
    String metaFilename = filenameStr + ".meta";
    File metaFile = SD.open(metaFilename.c_str(), FILE_READ);
    if (!metaFile) {
      Serial.println("Meta file does not exist. Creating new meta file.");
      metaFile = SD.open(metaFilename.c_str(), FILE_WRITE);
      if (!metaFile) {
        Serial.println("Failed to create meta file!");
        return false;
      }
      metaFile.println(0); // Write initial position 0 to the meta file
    } else {
      // Read the last sent position from the .meta file
      lastSentPosition = metaFile.parseInt();
    }
    metaFile.close();
    Serial.println("Meta file loaded.");

    // Open the file for reading
    File dataFile = SD.open(filename);
    if (!dataFile) {
        Serial.println("Failed to open file for reading");
        return false;
    }
    Serial.println("Processing data file.");
    if (dataFile.seek(lastSentPosition)) {
        Serial.println("Seek successful.");
        // Continue processing the file from this position
    } else {
        Serial.println("Seek failed.");
        // Handle the error, possibly by resetting the position or aborting the operation
    }


    // Initialize performance measurement
    unsigned long startTime = millis();
    size_t totalBytesSent = 0;

    Serial.println("Enter publish loop");
    // Read the file line by line
    while (dataFile.available()) {
        String line = dataFile.readStringUntil('\n');  // Read until the newline character

        // Skip the first line (header)
        if (line.startsWith("Time,")) {
            continue;
        }

        // Publish the line to the MQTT topic
        if (client.publish(topic.c_str(), line.c_str())) {
            // Add the size of the line to the total bytes sent
            totalBytesSent += line.length();
        }
    }

    // Close the file
    lastSentPosition = dataFile.position(); // Update the current position
    dataFile.close();
    Serial.println("Closed data file.");

    // Update the meta file with the last sent position
    metaFile = SD.open(metaFilename.c_str(), FILE_WRITE);
    if (!metaFile) {
        Serial.println("Failed to open meta file for updating!");
        return false;
    }
    metaFile.seek(0);
    metaFile.println(lastSentPosition); // Write the current position to the meta file
    metaFile.close();
    Serial.println("Updated meta file.");

    // Calculate time taken and transfer rate
    unsigned long endTime = millis();
    unsigned long duration = endTime - startTime;  // Duration in milliseconds
    float transferRateKBps = (totalBytesSent / 1024.0) / (duration / 1000.0);  // KB/s

    // Output performance metrics
    Serial.print("File processing complete. Time taken: ");
    Serial.print(duration / 1000.0);  // Convert to seconds
    Serial.print(" seconds, Transfer rate: ");
    Serial.print(transferRateKBps);
    Serial.println(" KB/s");
    return true;
}


// **************************************
// * Send All .dat File From Folder
// **************************************
bool mqtt_process_folder(String folderPath, String extension, String DeviceName){
  File root = SD.open(folderPath);

  if (!root) {
    Serial.println("Failed to open directory: " + folderPath);
    return -1;
  }

  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    if (!file.isDirectory() && fileName.endsWith(extension)) {
      String fullFilePath = folderPath + "/" + fileName;
      Serial.print("== Process file: "); Serial.println(fullFilePath);
      if (!mqtt_process_file(fullFilePath, DeviceName)) {// append mode
        Serial.println("sync_folder: send file failed, abort.");
        file.close();
        return -2;
      }
      file.close();
      Serial.print("Closed file:"); Serial.println(fullFilePath);
    }
    file = root.openNextFile();
  }
  root.close();
  return 0;
}

void publish_system_status() {
  if (!client.connected()) {
    mqtt_reconnect();
  }
  client.loop();

  // Gather system status
  int cpuFreq = getCpuFrequencyMhz();
  uint32_t freeHeap = esp_get_free_heap_size();
  uint32_t minFreeHeap = esp_get_minimum_free_heap_size();

  // Create the message payload
  String payload = "CPU Frequency: " + String(cpuFreq) + " MHz\n";
  payload += "Free Heap: " + String(freeHeap) + " bytes\n";
  payload += "Minimum Free Heap: " + String(minFreeHeap) + " bytes";

  // Publish the system status to a specific topic
  if (client.publish("esp32/status", payload.c_str())) {
    Serial.println("System status published successfully.");
  } else {
    Serial.println("Failed to publish system status.");
  }
}

void processFileTask(void * parameter) {

  while (true)
  {
    vTaskDelay(10000/portTICK_PERIOD_MS);
    // Process gateway data
    Serial.print("Processing mqtt for:"); Serial.println(systemConfig.DEVICE_NAME);
    mqtt_process_folder("/data", ".dat", systemConfig.DEVICE_NAME);

    // Process slave data
    for(int i = 0; i < peerCount; i++){
      String folerPath = "/node/" + String(peers[i].deviceName) + "/data";
      Serial.print("Processing mqtt for:"); Serial.println(String(peers[i].deviceName));
      mqtt_process_folder(folerPath, ".dat", String(peers[i].deviceName));
    }

  }
  
}

void systemInfoTask(void * parameter) {

  while (true)
  {
    publish_system_status();
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
  
}


void mqtt_initialize() {
  client.setServer(mqtt_server, 1883);
  client.setBufferSize(mqtt_buffer_size);


  // Create the task to process the file
  xTaskCreate(
    processFileTask,      // Task function
    "ProcessFileTask",    // Name of the task (for debugging)
    4 * 4096,                 // Stack size in words
    NULL,                 // Task input parameter
    1,                    // Priority of the task
    NULL                  // Task handle (can be NULL if not needed)
  );

  // Create the task to process the file
  xTaskCreate(
    systemInfoTask,      // Task function
    "systemInfoTask",    // Name of the task (for debugging)
    4096,                 // Stack size in words
    NULL,                 // Task input parameter
    1,                    // Priority of the task
    NULL                  // Task handle (can be NULL if not needed)
  );
}