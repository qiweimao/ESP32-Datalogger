#include "utils.h"

Preferences preferences;

/*OLED*/

// Define the screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define CHAR_HEIGHT 8  // Each character row is 8 pixels high

// Define the I2C address (0x3C for most Adafruit OLEDs)
#define OLED_ADDR 0x3C

// Number of rows that fit on the screen
#define NUM_ROWS (SCREEN_HEIGHT / CHAR_HEIGHT)

// Create the display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Buffer to store the data currently on the screen
char screenBuffer[NUM_ROWS][21];  // 20 characters + null terminator

/* LoRa */
//define the pins used by the transceiver module
#define ss 15
#define rst 27
#define dio0 2
// // Define the pins used by the HSPI interface
#define LORA_SCK 14
#define LORA_MISO 12
#define LORA_MOSI 13
#define LORA_SS 15

SPIClass loraSpi(HSPI);  // loraSpi = new SPIClass(HSPI);
void lora_init(void){
  loraSpi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setSPI(loraSpi);
  // SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS); // Initialize HSPI manually
  LoRa.setPins(LORA_SS, rst, dio0);
  //replace the LoRa.begin(---E-) argument with your location's frequency 
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
  while (!LoRa.begin(915E6)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
}

/* Time */
const char *ntpServers[] = {
  "pool.ntp.org",
  "time.google.com",
  "time.windows.com",
  "time.nist.gov",  // Add more NTP servers as needed
};
const int numNtpServers = sizeof(ntpServers) / sizeof(ntpServers[0]);
long gmtOffset_sec = -5 * 60 * 60;  // GMT offset in seconds (Eastern Time Zone)
int daylightOffset_sec = 3600;
RTC_DS1307 rtc;

const int CS = 5; // SD Card chip select
const int MAX_COMMANDSIZE = 6;
HardwareSerial VM(1); // UART port 1 on ESP32

String WIFI_SSID;
String WIFI_PASSWORD;
int ESP_NOW_MODE = ESP_NOW_RESPONDER; // default set up upon flashing

char daysOfWeek[7][12] = {
  "Sunday",
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday"
};

void wifi_setting_reset(){
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); //load the flash-saved configs
  esp_wifi_init(&cfg); //initiate and allocate wifi resources (does not matter if connection fails)
  delay(2000); //wait a bit
  if(esp_wifi_restore()!=ESP_OK){
    Serial.println("WiFi is not initialized by esp_wifi_init ");
  }
  else{
    Serial.println("Clear WiFi Configurations - OK");
  }
}

void load_system_configuration(){
  Serial.println("Loading configuration...");
  
  preferences.begin("credentials", false);
  
  if (preferences.isKey("WIFI_SSID")) {
    WIFI_SSID = preferences.getString("WIFI_SSID", "Verizon_F4ZD39");
    // Serial.print("WiFi SSID: ");
    // Serial.println(WIFI_SSID);
  } else {
    Serial.println("WiFi SSID not found. Using default value.");
    WIFI_SSID = "Verizon_F4ZD39";
    preferences.putString("WIFI_SSID", WIFI_SSID);
  }

  if (preferences.isKey("WIFI_PASSWORD")) {
    WIFI_PASSWORD = preferences.getString("WIFI_PASSWORD", "aft9-grid-knot");
    // Serial.print("WiFi Password: ");
    // Serial.println(WIFI_PASSWORD);
  } else {
    Serial.println("WiFi Password not found. Using default value.");
    WIFI_PASSWORD = "aft9-grid-knot";
    preferences.putString("WIFI_PASSWORD", WIFI_PASSWORD);
  }

  if (preferences.isKey("PROJECT_NAME")) {
    String projectName = preferences.getString("PROJECT_NAME", "new-project");
    Serial.print("PROJECT_NAME: ");
    Serial.println(projectName);
  } else {
    Serial.println("PROJECT_NAME not found. Using default value.");
    String projectName = "new-project";
    preferences.putString("PROJECT_NAME", projectName);
  }

  if (preferences.isKey("gmtOffset_sec")) {
    gmtOffset_sec = preferences.getLong("gmtOffset_sec", gmtOffset_sec);
    // Serial.print("GMT Offset (seconds): ");
    // Serial.println(gmtOffset_sec);
  } else {
    Serial.println("GMT Offset not found. Using default value.");
    gmtOffset_sec = -5 * 60 * 60;  // GMT offset in seconds (Eastern Time Zone)
    preferences.putLong("gmtOffset_sec", gmtOffset_sec);
  }

  if (preferences.isKey("ESP_NOW_MODE")) {
    ESP_NOW_MODE = preferences.getLong("ESP_NOW_MODE", ESP_NOW_RESPONDER);
    Serial.print("Boot as gateway: ");
    Serial.println(ESP_NOW_MODE);
  } else {
    Serial.println("ESP_NOW_MODE not found. Set as Responder.");
    preferences.putLong("ESP_NOW_MODE", ESP_NOW_RESPONDER);
  }

  preferences.end();
}
void update_system_configuration(String newSSID, String newWiFiPassword, long newgmtOffset_sec, int newESP_NOW_MODE, String newProjectName) {

  // Check if newSSID and newWiFiPassword are not empty
  if (newSSID.length() == 0 || newWiFiPassword.length() == 0) {
    Serial.println("Error: WiFi SSID or password cannot be empty.");
    return;
  }

  // Check if newESP_NOW_MODE is within valid range
  if (newESP_NOW_MODE < 0 || newESP_NOW_MODE > 3) {
    Serial.println("Error: ESP-NOW mode must be an integer between 0 and 3.");
    return;
  }

  // Check if newgmtOffset_sec is within valid range
  if (newgmtOffset_sec < -43200 || newgmtOffset_sec > 43200) {
    Serial.println("Error: GMT offset must be between -43200 and 43200.");
    return;
  }

  Serial.println("Updating configuration...");
  
  preferences.begin("credentials", false);
  
  if (preferences.isKey("WIFI_SSID")) {
    preferences.putString("WIFI_SSID", newSSID);
  }

  if (preferences.isKey("PROJECT_NAME")) {
    preferences.putString("PROJECT_NAME", newProjectName);
  }

  if (preferences.isKey("WIFI_PASSWORD")) {
    preferences.putString("WIFI_PASSWORD", newWiFiPassword);
  }

  if (preferences.isKey("gmtOffset_sec")) {
    preferences.putLong("gmtOffset_sec", newgmtOffset_sec);
  }

  if (preferences.isKey("ESP_NOW_MODE")) {
    preferences.putLong("ESP_NOW_MODE", newESP_NOW_MODE);
  }

  preferences.end();
}

void vm501_init() {
  VM.begin(9600, SERIAL_8N1, 16, 17); // Initialize UART port 1 with GPIO16 as RX and GPIO17 as TX
}

DateTime tmToDateTime(struct tm timeinfo) {
  return DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

void external_rtc_init(){

  if (! rtc.begin()) {
    Serial.println("RTC module is NOT found");
    Serial.flush();
    return;
  }

  DateTime now = rtc.now();
  Serial.print("RTC time: ");
  print_datetime(now);
}

void external_rtc_sync_ntp(){
  // Synchronize DS1307 RTC with NTP time
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    DateTime ntpTime = tmToDateTime(timeinfo);
    rtc.adjust(ntpTime);
    Serial.println("DS1307 RTC synchronized with NTP time.");
    DateTime now = rtc.now();
    Serial.print("RTC time: ");
    print_datetime(now);
    return; // Exit the function if synchronization is successful
  } else {
    Serial.printf("Failed to get NTP Time for DS1307\n");
    return;
  }
}

void print_datetime(DateTime dt) {
  // Print date and time components
  Serial.print(dt.year(), DEC);
  Serial.print('/');
  Serial.print(dt.month(), DEC);
  Serial.print('/');
  Serial.print(dt.day(), DEC);
  Serial.print(" ");
  Serial.print(dt.hour(), DEC);
  Serial.print(':');
  Serial.print(dt.minute(), DEC);
  Serial.print(':');
  Serial.print(dt.second(), DEC);
  Serial.println();
}

void ntp_sync() {

  // Attempt synchronization with each NTP server in the list
  for (int i = 0; i < numNtpServers; i++) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServers[i]);
    struct tm timeinfo;
    Serial.printf("Syncing with NTP server %s...\n", ntpServers[i]);
    if (getLocalTime(&timeinfo)) {
      Serial.printf("Time synchronized successfully with NTP server %s\n", ntpServers[i]);
      Serial.printf("NTP Time: %s\n", get_current_time().c_str());
      external_rtc_sync_ntp();
      return; // Exit the function if synchronization is successful
    } else {
      Serial.printf("Failed to synchronize with NTP server %s\n", ntpServers[i]);
    }
  }
  // If synchronization fails with all servers
  Serial.println("Failed to synchronize with any NTP server.");
}

String get_current_time(bool getFilename) {
  struct tm timeinfo;
  DateTime now = rtc.now();
  
    char buffer[20]; // Adjust the buffer size based on your format
    if(!getFilename){
      snprintf(buffer, sizeof(buffer), "%04d/%02d/%02d %02d:%02d:%02d", 
              now.year(), now.month(), now.day(), now.hour(), now.minute(),now.second());
    }
    else{
      // Round minutes to the nearest 15-minute interval
      int roundedMinutes = (now.minute() / 15) * 15;
      snprintf(buffer, sizeof(buffer), "%04d_%02d_%02d_%02d_%02d", 
              now.year(), now.month(), now.day(), now.hour(), roundedMinutes);
    }
    return buffer;
  return ""; // Return an empty string in case of failure
}

String get_public_ip() {

  HTTPClient http;
  http.begin("https://api.ipify.org");  // Make a GET request to api.ipify.org to get the public IP
  int httpCode = http.GET();  // Send the request

  if (httpCode == HTTP_CODE_OK) {  // Check for a successful response
    String publicIP = http.getString();
    return publicIP;
  } else {
    Serial.printf("HTTP request failed with error code %d\n", httpCode);
    return "Error";
  }

  http.end();  // End the request
}

void spiffs_init(){
  // Mount SPIFFS filesystem
  Serial.printf("Mounting SPIFFS filesystem - ");
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS allocation failed");
    return;
  }
  Serial.println("OK");
}

// ==============================================================
// File IO Functions
// ==============================================================
SPIClass *sdSpi = NULL; // SPI object for SD card

void sd_init(){
  Serial.printf("Initializing SD card - ");

  // Initialize SD card SPI bus
  sdSpi = new SPIClass(VSPI);
  sdSpi->begin(18, 19, 23, 5); // VSPI pins for SD card: miso, mosi, sck, ss

  // SPI.begin(18, 19, 23, 5); //SCK, MISO, MOSI,SS
  // if (!SD.begin(CS, SPI)) {
  if (!SD.begin(CS, *sdSpi)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("OK");
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

void oled_init() {

  // Initialize the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Initialize the screen buffer with empty strings
  for (int i = 0; i < NUM_ROWS; i++) {
    screenBuffer[i][0] = '\0';
  }

}

void oled_print(const char* text) {
  static int readingIndex = 0; // Keep track of the current reading index

  // Scroll up if the screen is full
  if (readingIndex >= NUM_ROWS) {
    // Shift all rows up by copying the content from the next row
    for (int i = 0; i < NUM_ROWS - 1; i++) {
      strcpy(screenBuffer[i], screenBuffer[i + 1]);
    }
    readingIndex = NUM_ROWS - 1;
  }

  // Add the new reading to the buffer
  strncpy(screenBuffer[readingIndex], text, sizeof(screenBuffer[readingIndex]));
  screenBuffer[readingIndex][sizeof(screenBuffer[readingIndex]) - 1] = '\0';  // Ensure null termination

  // Clear the display
  display.clearDisplay();

  // Redraw all the buffer content
  for (int i = 0; i <= readingIndex; i++) {
    display.setCursor(0, i * CHAR_HEIGHT);
    display.print(screenBuffer[i]);
  }
  display.display();

  // Update the reading index
  readingIndex++;
}

// Overloaded function for uint8_t data type
void oled_print(uint8_t value) {
  char buffer[8];
  snprintf(buffer, 8, "%u", value);
  oled_print(buffer);
}