#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <SD.h>

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

int mqtt_buffer_size = 3 * 4096;

void mqtt_initialize() {
  client.setServer(mqtt_server, 1883);
  client.setBufferSize(mqtt_buffer_size);
}

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


bool mqtt_process_file(const char* filename) {

    // mqtt client prepare
    if (!client.connected()) {
        mqtt_reconnect();
    }
    client.loop();

    // load meta file
    size_t lastSentPosition = 0;
    String filenameStr = String(filename);
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
    dataFile.seek(lastSentPosition);// Seek to the last sent position in the data file


    // Initialize performance measurement
    unsigned long startTime = millis();
    size_t totalBytesSent = 0;

    // Read the file line by line
    while (dataFile.available()) {
        String line = dataFile.readStringUntil('\n');  // Read until the newline character

        // Skip the first line (header)
        if (line.startsWith("Time,")) {
            continue;
        }

        // Publish the line to the MQTT topic
        if (client.publish(filename, line.c_str())) {
            // Add the size of the line to the total bytes sent
            totalBytesSent += line.length();
        }
    }

    // Close the file
    lastSentPosition = dataFile.position(); // Update the current position
    dataFile.close();

    // Update the meta file with the last sent position
    metaFile = SD.open(metaFilename.c_str(), FILE_WRITE);
    if (!metaFile) {
        Serial.println("Failed to open meta file for updating!");
        return false;
    }
    metaFile.seek(0);
    metaFile.println(lastSentPosition); // Write the current position to the meta file
    metaFile.close();

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
bool mqtt_process_folder(String folderPath, String extension){
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
      Serial.print("Process file: "); Serial.println(fullFilePath);
      if (!mqtt_process_file(fullFilePath.c_str())) {// append mode
        Serial.println("sync_folder: send file failed, abort.");
        file.close();
        return -2;
      }
      file.close();
    }
    file = root.openNextFile();
  }
  root.close();
  return 0;
}

// void mqtt_process_file_in_batches() {
//     if (!client.connected()) {
//         mqtt_reconnect();
//     }
//     client.loop();

//     // Specify the filename you want to read
//     String filename = "/node/STA-01/data/0.dat";

//     // Open the file for reading
//     File dataFile = SD.open(filename);
    
//     if (!dataFile) {
//         Serial.println("Failed to open file for reading");
//         return;
//     }

//     // Initialize performance measurement
//     unsigned long startTime = millis();
//     size_t totalBytesSent = 0;
//     String batchPayload = "";
//     const size_t maxBatchSize = mqtt_buffer_size - 10;  // Adjust to a bit less than max to account for MQTT overhead
//     const int maxRetries = 3;  // Maximum number of retry attempts

//     // Read the file line by line
//     while (dataFile.available()) {
//         String line = dataFile.readStringUntil('\n');  // Read until the newline character

//         // Skip the first line (header)
//         if (line.startsWith("Time,")) {
//             continue;
//         }

//         // Check if the current line can be added to the batch
//         if (batchPayload.length() + line.length() < maxBatchSize) {
//             batchPayload += line + "\n";
//         } else {
//             // Send the current batch with retry logic
//             bool sent = false;
//             for (int attempt = 0; attempt < maxRetries; attempt++) {
//                 if (client.publish("vwpz_reading", batchPayload.c_str())) {
//                     totalBytesSent += batchPayload.length();
//                     sent = true;
//                     break;
//                 } else {
//                     Serial.print("Failed to send batch. Attempt ");
//                     Serial.print(attempt + 1);
//                     Serial.println(" of 3.");
//                 }
//             }

//             // If sending failed after retries, print an error
//             if (!sent) {
//                 Serial.println("Failed to send batch after maximum retries.");
//             }

//             // Start a new batch
//             batchPayload = line + "\n";
//         }
//     }

//     // Publish any remaining lines in the batchPayload with retry logic
//     if (batchPayload.length() > 0) {
//         bool sent = false;
//         for (int attempt = 0; attempt < maxRetries; attempt++) {
//             if (client.publish("vwpz_reading", batchPayload.c_str())) {
//                 totalBytesSent += batchPayload.length();
//                 sent = true;
//                 break;
//             } else {
//                 Serial.print("Failed to send final batch. Attempt ");
//                 Serial.print(attempt + 1);
//                 Serial.println(" of 3.");
//             }
//         }

//         // If sending failed after retries, print an error
//         if (!sent) {
//             Serial.println("Failed to send final batch after maximum retries.");
//         }
//     }

//     // Close the file
//     dataFile.close();

//     // Calculate time taken and transfer rate
//     unsigned long endTime = millis();
//     unsigned long duration = endTime - startTime;  // Duration in milliseconds
//     float transferRateKBps = (totalBytesSent / 1024.0) / (duration / 1000.0);  // KB/s

//     // Output performance metrics
//     Serial.print("File processing complete. Time taken: ");
//     Serial.print(duration / 1000.0);  // Convert to seconds
//     Serial.print(" seconds, Transfer rate: ");
//     Serial.print(transferRateKBps);
//     Serial.println(" KB/s");
// }
