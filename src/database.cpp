#include "database.h"
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include "configuration.h"
#include "SD.h"

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(
    String(InfluxDBConfig.INFLUXDB_URL),
    String(InfluxDBConfig.INFLUXDB_ORG),
    String(InfluxDBConfig.INFLUXDB_BUCKET),
    String(InfluxDBConfig.INFLUXDB_TOKEN));
// InfluxDB client instance without preconfigured InfluxCloud certificate for insecure connection 
//InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

// Configurations
int maxHeaders =20;

// Data point
Point sensor("R");

bool pushFiletoInfluxDB(const char* filename);
void pushFilesTask(void * parameter);
time_t convertISO8601ToUnix(const String& timestampStr);

void setupInfluxDB() {

    Serial.print("Initializing InfluxDB: ");

    Serial.print("WiFi.isConnected()");
    Serial.println(WiFi.isConnected());

    client.setConnectionParams(
        String(InfluxDBConfig.INFLUXDB_URL),
        String(InfluxDBConfig.INFLUXDB_ORG),
        String(InfluxDBConfig.INFLUXDB_BUCKET),
        String(InfluxDBConfig.INFLUXDB_TOKEN));

    if (client.validateConnection()) {
        Serial.print("Connected to InfluxDB: ");
        Serial.println(client.getServerUrl());

        // Enable lines batching
        client.setWriteOptions(WriteOptions().batchSize(10));
        // Increase buffer to allow caching of failed writes
        client.setWriteOptions(WriteOptions().bufferSize(30));

        // Create the pushFilesTask
        xTaskCreate(
            pushFilesTask,   // Task function
            "PushFilesTask", // Name of the task
            4096,           // Stack size (bytes)
            NULL,            // Parameter to pass to the task
            1,               // Task priority
            NULL             // Task handle
        );

    } else {
        Serial.print("InfluxDB connection failed: ");
        Serial.println(client.getLastErrorMessage());
    }
}


// **************************************
// * Send All .dat File From Folder
// **************************************
int pushFoldertoInfluxDB(String folderPath, String extension) {
  File root = SD.open(folderPath);

  if (!root) {
    Serial.println("Failed to open directory: " + folderPath);
    return -1;
  }

  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    String fullFilePath = folderPath + "/" + fileName;
    if (!file.isDirectory() && !fileName.endsWith(".meta")) {
    
        file.close();

        if(!pushFiletoInfluxDB(fullFilePath.c_str())){
            Serial.println("Failed to push file");
        }

    }
    file = root.openNextFile();
    vTaskDelay(1 / portTICK_PERIOD_MS); // Wait for 60 seconds
  }
  root.close();
  return 0;
}

// function to scan data files for processing
// maintain a meta file to last processed position
// sensor type from folderpath
// channel no and mac from calling function
// get field information from the header of the file
// seek to processing position
// push data to InfluxDB

bool pushFiletoInfluxDB(const char* filename) {
    Serial.printf("pushFiletoInfluxDBLoRaFile %s\n", filename);

    const unsigned long maxRunTime = 500; // 5 seconds
    unsigned long startTime = millis();

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

    File file = SD.open(filename);
    if (!file) {
        Serial.println("Failed to open file!");
        return false;
    }

    // Read the first line (header) of the file
    String headerLine = file.readStringUntil('\n');
    Serial.println("Header: " + headerLine);

    // Extract headers into an array of Strings
    String headers[maxHeaders];
    int headerCount = 0;

    int startIdx = 0;
    int endIdx = headerLine.indexOf(',');
    while (endIdx != -1 && headerCount < maxHeaders) {
        // Extract only the first 4 characters of the header
        headers[headerCount++] = headerLine.substring(startIdx, startIdx + 4);
        startIdx = endIdx + 1;
        endIdx = headerLine.indexOf(',', startIdx);
    }

    // Add the last header, ensuring only the first 4 characters are used
    if (headerCount < maxHeaders) {
        headers[headerCount++] = headerLine.substring(startIdx, startIdx + 4);
    }

    // Validate headers
    if (headerCount < 2) {
        Serial.println("Invalid header. Exiting.");
        file.close();
        return false;
    }

    // Seek to the last sent position in the data file
    file.seek(lastSentPosition);

    uint8_t MAC_ADDRESS_STA[6];
    esp_read_mac(MAC_ADDRESS_STA, ESP_MAC_WIFI_STA);  

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X",
            MAC_ADDRESS_STA[0], MAC_ADDRESS_STA[1], MAC_ADDRESS_STA[2],
            MAC_ADDRESS_STA[3], MAC_ADDRESS_STA[4], MAC_ADDRESS_STA[5]);
    sensor.clearTags();
    // sensor.addTag("mac", macStr);
    // sensor.addTag("n", filename);

    while (file.available()) {

        // Check if the maximum runtime has been exceeded
        if (millis() - startTime >= maxRunTime) {
            Serial.println("Exceeded maximum runtime. Exiting.");
            file.close();
            break;
        }
        
        String dataLine = file.readStringUntil('\n');

        if (dataLine.length() > 0) {
            int commaCount = 0;
            for (int i = 0; i < dataLine.length(); i++) {
                if (dataLine[i] == ',') {
                    commaCount++;
                }
            }

            // Validate the data row
            if (commaCount != (headerCount - 1)) {
                Serial.println("Corrupted data row. Skipping.");
                continue;
            }


            sensor.clearFields();

            startIdx = 0;
            endIdx = dataLine.indexOf(',', startIdx);
            int fieldIdx = 0;
            while (endIdx != -1) {
                String value = dataLine.substring(startIdx, endIdx);
                if (fieldIdx < headerCount) {
                    sensor.addField(headers[fieldIdx], value.toFloat());
                }
                startIdx = endIdx + 1;
                endIdx = dataLine.indexOf(',', startIdx);
                fieldIdx++;
            }
            String lastValue = dataLine.substring(startIdx);
            if (fieldIdx < headerCount) {
                // sensor.addField(headers[fieldIdx].c_str(), lastValue.toFloat());
                sensor.addField("DegC", lastValue.toFloat());
            }

            // Print what we are writing
            // Serial.println("= = = = = = = = Added all Fields = = = = = = = = ");
            // Serial.println("= = Print Point = = ");
            vTaskDelay(1 / portTICK_PERIOD_MS); // Wait for 60 seconds
            Serial.println(client.pointToLineProtocol(sensor));
            // Serial.print("Line length: "); Serial.println(client.pointToLineProtocol(sensor).length());

            // Write point
            if (!client.writePoint(sensor)) {
                Serial.print("InfluxDB write failed: ");
                Serial.println(client.getLastErrorMessage());
                file.close();
                return false;
            }

            vTaskDelay(1 / portTICK_PERIOD_MS); // Wait for 60 seconds
            // Update the last sent position
            lastSentPosition = file.position();
        }
        
        vTaskDelay(1 / portTICK_PERIOD_MS); // Wait for 60 seconds
    }

    file.close();

    // Update the meta file with the last sent position
    metaFile = SD.open(metaFilename.c_str(), FILE_WRITE);
    if (!metaFile) {
        Serial.println("Failed to open meta file for updating!");
        return false;
    }
    metaFile.seek(0);
    metaFile.println(lastSentPosition); // Write the current position to the meta file
    metaFile.close();

    return true;
}

// Task to periodically push files to InfluxDB
void pushFilesTask(void * parameter) {
    while (true) {
        vTaskDelay(60000 / portTICK_PERIOD_MS); // Wait for 60 seconds
        Serial.println("Task: Push files to InfluxDB, Started.");
        pushFoldertoInfluxDB("/data", ".csv");
        Serial.println("Task: Pushed files to InfluxDB, waiting for next iteration.");
    }
}

// Function to convert ISO 8601 time string to Unix time
time_t convertISO8601ToUnix(const String& timestampStr) {
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    
    // Parse the date-time part
    if (strptime(timestampStr.c_str(), "%Y-%m-%dT%H:%M:%S", &tm) == NULL) {
        return 0;
    }

    // Adjust for timezone offset if provided
    const char* tzStart = strchr(timestampStr.c_str(), '-');
    if (!tzStart) {
        tzStart = strchr(timestampStr.c_str(), '+');
    }

    int tzHours = 0;
    int tzMinutes = 0;
    if (tzStart) {
        sscanf(tzStart, "%03d:%02d", &tzHours, &tzMinutes);
        tm.tm_hour -= tzHours;
        tm.tm_min -= tzMinutes;
    }

    // Convert to Unix time
    time_t t = mktime(&tm);
    return t;
}