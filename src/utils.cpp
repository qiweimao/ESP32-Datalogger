#include "utils.h"

const int CS = 5;
const char *ntpServers[] = {
  "pool.ntp.org",
  "time.google.com",
  "time.windows.com",
  "time.nist.gov",  // Add more NTP servers as needed
};

const int MAX_COMMANDSIZE = 6;
const int numNtpServers = sizeof(ntpServers) / sizeof(ntpServers[0]);
extern HardwareSerial VM; // UART port 1 on ESP32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RTC_DS1307 rtc;

char daysOfWeek[7][12] = {
  "Sunday",
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday"
};

void initDS1307(){

  if (! rtc.begin()) {
    Serial.println("RTC module is NOT found");
    Serial.flush();
    return;
  }

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

}

void initNTP() {

  // Attempt synchronization with each NTP server in the list
  for (int i = 0; i < numNtpServers; i++) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServers[i]);
    struct tm timeinfo;
    Serial.printf("Syncing with NTP server %s...\n", ntpServers[i]);
    if (getLocalTime(&timeinfo)) {
      Serial.printf("Time synchronized successfully with NTP server %s\n", ntpServers[i]);
      Serial.printf("The current time is: %s\n", getCurrentTime().c_str());
      return; // Exit the function if synchronization is successful
    } else {
      Serial.printf("Failed to synchronize with NTP server %s\n", ntpServers[i]);
    }
  }
  // If synchronization fails with all servers
  Serial.println("Failed to synchronize with any NTP server.");
}


String getCurrentTime() {
  struct tm timeinfo;
  
  if (getLocalTime(&timeinfo)) {
    char buffer[20]; // Adjust the buffer size based on your format
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return buffer;
  } else {
    Serial.println("Failed to get current time");
    return ""; // Return an empty string in case of failure
  }
  return ""; // Return an empty string in case of failure
}

void connectToWiFi() {
  // Serial.println("Connecting to WiFi");
  // WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    // Serial.println("Connecting to WiFi...");
  }

  // Serial.println("Connected to WiFi");
  // Serial.print("IP Address: ");
  // Serial.println(WiFi.localIP());
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("WiFi connected");
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi disconnected");
      connectToWiFi();
      break;
    default:
      break;
  }
}

String getPublicIP() {
  HTTPClient http;

  // Make a GET request to api.ipify.org to get the public IP
  http.begin("https://api.ipify.org");

  // Send the request
  int httpCode = http.GET();

  // Check for a successful response
  if (httpCode == HTTP_CODE_OK) {
    String publicIP = http.getString();
    return publicIP;
  } else {
    Serial.printf("HTTP request failed with error code %d\n", httpCode);
    return "Error";
  }

  // End the request
  http.end();
}

void setupSPIFFS(){
  // Mount SPIFFS filesystem
  Serial.printf("Mounting SPIFFS filesystem......");
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS allocation failed");
    return;
  }
  Serial.printf("Successfull.\n");
}

// ==============================================================
// File IO Functions
// ==============================================================

void SD_initialize(){
  Serial.printf("Initializing SD card...");
  if (!SD.begin(CS)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
}

void createDir(fs::FS &fs, const char * path){
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path)){
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char * path){
  Serial.printf("Removing Dir: %s\n", path);
  if(fs.rmdir(path)){
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
  // Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
      // Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char * path){
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if(file){
    len = file.size();
    size_t flen = len;
    start = millis();
    while(len){
      size_t toRead = len;
      if(toRead > 512){
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }


  file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for(i=0; i<2048; i++){
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}

void sendCommandVM501(void *parameter) {
    while (true) {
        // Check if data is available on the serial port
        if (Serial.available() > 0) {
            // Read the input command from the serial port
            String input = Serial.readStringUntil('\n');
            // Convert String to C-string
            char command[input.length() + 1];
            input.toCharArray(command, sizeof(command));

            // Parse the command
            parseCommand(command);
        }
        // Delay for a short period to avoid busy-waiting
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void parseCommand(const char* command) {
    // Variables to store parsed values
    char commandName[6];
    uint8_t address;
    uint8_t functionCode;
    uint8_t startAddressHigh;
    uint8_t startAddressLow;
    uint8_t numRegistersHigh;
    uint8_t numRegistersLow;
    uint8_t crcHigh;
    uint8_t crcLow;

    uint8_t hexArray[MAX_COMMANDSIZE] = {};

// MODBUS 0x01 0x03 0x00 0x00 0x00 0x0A
// MODBUS 0x01 0x03 0x00 0x27 0x00 0x01 GET Sensor Resistance 
  if (strncmp(command, "MODBUS", 6) == 0) {
    // Use sscanf to parse the command string
    int result = sscanf(command, "%s 0x%2hhx 0x%2hhx 0x%2hhx 0x%2hhx 0x%2hhx 0x%2hhx",
                        &commandName, 
                        &hexArray[0], &hexArray[1], &hexArray[2], &hexArray[3], 
                        &hexArray[4], &hexArray[5]);

    // Check if all values were successfully parsed
    if (result - 1 == MAX_COMMANDSIZE) {
      unsigned int crc = crc16(hexArray, sizeof(hexArray));
      uint8_t crcHighByte = (uint8_t)(crc >> 8);
      uint8_t crcLowByte = (uint8_t)(crc & 0xFF);

      VM.write(hexArray, sizeof(hexArray));
      VM.write(crcLowByte);
      VM.write(crcHighByte);

      delay(1000);
      Serial.printf("VM501:");
      while (VM.available() > 0) {
        uint8_t incomingByte = VM.read();
        Serial.printf("0x%02x ", incomingByte);
      }
      Serial.printf("\n");
    }
    else {
        Serial.println("Invalid MODBUS command");
    }
  }
  else if (strncmp(command, "$", 1) == 0){

    VM.write(command);
    VM.write("\n");

    delay(3000);

    if (VM.available() > 0) {
      String receivedChar = VM.readString();
      Serial.println(receivedChar);
    }
    else{
      Serial.println("No reponse from VM501.");
    }
  }
  else{
    Serial.println("Invalid command");
  }
}

unsigned int crc16(unsigned char *dat, unsigned int len)
{
    unsigned int crc = 0xffff;
    unsigned char i;
    while (len != 0)
    {
        crc ^= *dat;
        for (i = 0; i < 8; i++)
        {
            if ((crc & 0x0001) == 0)
                crc = crc >> 1;
            else
            {
                crc = crc >> 1;
                crc ^= 0xa001;
            }
        }
        len -= 1;
        dat++;
    }
    return crc;
}

void initializeOLED() {

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
      Serial.println(F("SSD1306 allocation failed"));
      return;
  }

  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.println("Booting...");
  display.display(); 

}