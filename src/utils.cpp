#include "utils.h"
#include "configuration.h"

const int CS = 5; // SD Card chip select
HardwareSerial VM(1); // UART port 1 on ESP32

void spiffs_init(){
  // Mount SPIFFS filesystem
  Serial.printf("Mounting SPIFFS filesystem - ");
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS allocation failed");
    return;
  }
  Serial.println("OK");
}

/******************************************************************
 *                                                                *
 *                             WiFi                               *
 *                                                                *
 ******************************************************************/

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

// Function to connect to WiFi
void wifi_init(){

    WiFi.mode(WIFI_AP_STA);

    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Wait for connection
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
      i++;
        delay(1000);
        Serial.print(".");
        if(i > 10){
          break;
        }
    }

    // Print local IP address
    if(WiFi.status() != WL_CONNECTED){
      Serial.println();
      Serial.println("Connected to WiFi. IP address: ");
      Serial.println(WiFi.localIP());
    }
    else{
      Serial.println("NOT Connected to WiFi. Will keep trying.");
    }

    // Set up Access Point (AP)
    Serial.print("Setting up AP...");
    bool ap_started = WiFi.softAP(DEVICE_NAME, "SenseLynk101");
    if(ap_started){
        Serial.println("AP started");
        Serial.printf("AP SSID: %s\n", DEVICE_NAME);
        Serial.printf("AP Password: SenseLynk101\n");
        Serial.print("AP IP address: ");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.println("Failed to start AP");
    }

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



/******************************************************************
 *                                                                *
 *                             Time                               *
 *                                                                *
 ******************************************************************/

int rtc_mounted = 0;

/* Time */
const char *ntpServers[] = {
  "pool.ntp.org",
  "time.google.com",
  "time.windows.com",
  "time.nist.gov",  // Add more NTP servers as needed
};

const int numNtpServers = sizeof(ntpServers) / sizeof(ntpServers[0]);
int daylightOffset_sec = 3600;
RTC_DS1307 rtc;

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

  rtc_mounted = 1;

  DateTime now = rtc.now();
  Serial.print("RTC time: ");
  Serial.println(get_current_time());

  // Set the ESP32 system time to the RTC time
  struct tm timeinfo;
  timeinfo.tm_year = now.year() - 1900; // tm_year is year since 1900
  timeinfo.tm_mon = now.month() - 1;    // tm_mon is 0-based
  timeinfo.tm_mday = now.day();
  timeinfo.tm_hour = now.hour();
  timeinfo.tm_min = now.minute();
  timeinfo.tm_sec = now.second();
  time_t t = mktime(&timeinfo);
  struct timeval now_tv = { .tv_sec = t };
  settimeofday(&now_tv, NULL);
}

void external_rtc_sync_ntp(){

  if(!rtc_mounted){
    Serial.println("Skip RTC sync, external RTC not mounted.");
    return;
  }

  // Synchronize DS1307 RTC with NTP time
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    DateTime ntpTime = tmToDateTime(timeinfo);
    rtc.adjust(ntpTime);
    Serial.println("\nDS1307 RTC synchronized with NTP time.");
    DateTime now = rtc.now();
    Serial.print("RTC time: ");
    Serial.println(get_current_time());    return; // Exit the function if synchronization is successful
  } else {
    Serial.printf("Failed to get NTP Time for DS1307\n");
    return;
  }
}

void ntp_sync() {

  long gmtOffset_sec = utcOffset * 60 * 60;

  // Attempt synchronization with each NTP server in the list
  for (int i = 0; i < numNtpServers; i++) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServers[i]);
    struct tm timeinfo;
    // Serial.printf("Syncing with NTP server %s...\n", ntpServers[i]);
    if (getLocalTime(&timeinfo)) {
      // Serial.printf("Time synchronized successfully with NTP server %s\n", ntpServers[i]);
      // Serial.printf("NTP Time: %s\n", get_current_time().c_str());
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

  if(!rtc_mounted){
    Serial.println("External RTC not mounted. Cannot get current time.");
    return "error.";
  }

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

/******************************************************************
 *                                                                *
 *                            SD Card                             *
 *                                                                *
 ******************************************************************/
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

  // Ensure the "data" directory exists
  if (!SD.exists("/data")) {
    Serial.print("Creating 'data' directory - ");
    if (SD.mkdir("/data")) {
      Serial.println("OK");
    } else {
      Serial.println("Failed to create 'data' directory");
    }
  }

}

/******************************************************************
 *                                                                *
 *                            OLED                                *
 *                                                                *
 ******************************************************************/

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define CHAR_HEIGHT 8  // Each character row is 8 pixels high
#define OLED_ADDR 0x3C // Define the I2C address (0x3C for most Adafruit OLEDs)
#define NUM_ROWS (SCREEN_HEIGHT / CHAR_HEIGHT) // Number of rows that fit on the screen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);// Create the display object
char screenBuffer[NUM_ROWS][21];  // 20 characters + null terminator

void oled_init() {

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  Serial.println("SSD1306 allocation done.");

  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Initialize the screen buffer with empty strings
  for (int i = 0; i < NUM_ROWS; i++) {
    screenBuffer[i][0] = '\0';
  }

  display.setCursor(0, 0);  // Set cursor to top-left corner
  display.println(F("booted"));
  display.display();

}

// Overloaded function to print text to OLED display assuming null-terminated string
void oled_print(const char* text) {
    oled_print(text, strlen(text));
}

void oled_print(const char* text, size_t size) {
    static int readingIndex = 0; // Keep track of the current reading index
    const int maxLineLength = sizeof(screenBuffer[0]) - 1; // Maximum length of a line in the buffer

    const char* ptr = text;
    const char* end = text + size;

    while (ptr < end) {
        // Scroll up if the screen is full
        if (readingIndex >= NUM_ROWS) {
            // Shift all rows up by copying the content from the next row
            for (int i = 0; i < NUM_ROWS - 1; i++) {
                strcpy(screenBuffer[i], screenBuffer[i + 1]);
            }
            readingIndex = NUM_ROWS - 1;
        }

        // Copy a portion of the text to the current buffer line
        size_t copyLength = maxLineLength;
        if (ptr + copyLength > end) {
            copyLength = end - ptr;
        }
        strncpy(screenBuffer[readingIndex], ptr, copyLength);
        screenBuffer[readingIndex][copyLength] = '\0';  // Ensure null termination

        // Move the pointer forward by the length of the copied text
        ptr += copyLength;

        // Clear the display
        display.clearDisplay();

        // Redraw all the buffer content
        for (int i = 0; i < NUM_ROWS; i++) {
            display.setCursor(0, i * CHAR_HEIGHT);
            display.print(screenBuffer[i]);
        }
        display.display();

        // Update the reading index
        readingIndex++;
    }
}


// Overloaded function for uint8_t data type
void oled_print(uint8_t value) {
  char buffer[8];
  snprintf(buffer, 8, "%u", value);
  oled_print(buffer, sizeof(buffer));
}

// Function to generate a random number using ESP32's hardware RNG
uint32_t generateRandomNumber() {
  // Seed the random number generator with a value from the hardware RNG
  uint32_t seed = esp_random();
  randomSeed(seed);
  
  // Generate a random number between 0 and 4294967294
  uint32_t randomNumber = random(0, 4294967295);
  
  return randomNumber;
}

/******************************************************************
 *                                                                *
 *                            FTP                                 *
 *                                                                *
 ******************************************************************/

FtpServer ftpSrv;   //set #define FTP_DEBUG in ESP8266FtpServer.h to see ftp verbose on serial

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

void ftp_init(){
  ftpSrv.setCallback(_callback);
  // ftpSrv.setTransferCallback(_transferCallback);

  ftpSrv.begin("esp32","esp32");    //username, password for ftp.   (default 21, 50009 for PASV)
}

/******************************************************************
 *                                                                *
 *                       Error Logging                            *
 *                                                                *
 ******************************************************************/

int sdCardLogOutput(const char *format, va_list args)
{
	// Serial.println("Callback running");
  char buf[128] = {0}; // Initialize buffer with zeros
	int ret = vsnprintf(buf, sizeof(buf), format, args);
  Serial.println(buf);
  oled_print(buf, ret);
	return ret;
}

void esp_error_init_sd_oled(){

  Serial.println("Setting log levels and callback");
  esp_log_level_set("TEST", LOG_LEVEL);
  esp_log_level_set(TAG, MY_ESP_LOG_LEVEL);
  esp_log_set_vprintf(sdCardLogOutput);
  
  // ESP_LOGE(TAG, "Error message");
  // ESP_LOGW(TAG, "Warning message");
  // // ESP_LOGI("TEST", "Info message");
  // ESP_LOGD(TAG, "Verbose message"); 
}