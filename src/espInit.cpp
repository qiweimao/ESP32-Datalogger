/*
  ESP-NOW Demo - Transmit
  esp-now-demo-xmit.ino
  Sends data to Responder
  
  DroneBot Workshop 2022
  https://dronebotworkshop.com
*/

// Include Libraries
#include <esp_now.h>
#include <WiFi.h>
#include <espInit.h>
#include <esp_wifi.h>

#include "Secrets.h"

// Variables for test data
int int_value;
float float_value;
bool bool_value = true;

// MAC Address of responder - edit as required
uint8_t broadcastAddress[] = {0x30, 0x83, 0x98, 0x00, 0x52, 0x8C};

// Define a data structure
typedef struct struct_message {
  char a[32];
  int b;
  float c;
  bool d;
} struct_message;

// Create a structured object
struct_message myData;

// Peer info
esp_now_peer_info_t peerInfo;

unsigned int readingId = 0;

int32_t channel = 1;

// Insert your SSID
// constexpr char WIFI_SSID[] = "REPLACE_WITH_YOUR_SSID";

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

// Callback function called when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status != ESP_NOW_SEND_SUCCESS){
    if (channel <= 5){
      channel += 1;
    }
    else{
      channel = 1;
    }
    testChannel(channel);
  }
}

// Callback function executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Data received: ");
  Serial.println(len);
  Serial.print("Character Value: ");
  Serial.println(myData.a);
  Serial.print("Integer Value: ");
  Serial.println(myData.b);
  Serial.print("Float Value: ");
  Serial.println(myData.c);
  Serial.print("Boolean Value: ");
  Serial.println(myData.d);
  Serial.println();
}

void testChannel(int32_t channel){

  // Set device as a Wi-Fi Station and set channel
  WiFi.mode(WIFI_STA);
  esp_wifi_set_protocol( WIFI_IF_STA , WIFI_PROTOCOL_LR);

  // int32_t channel = getWiFiChannel(WIFI_SSID);

  WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); // Uncomment to verify channel change after

  // Initilize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the send callback
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void espSenderInit() {

  for (size_t i = 0; i < 11; i++)
  {
    testChannel(i+1);
    esp_err_t result = espSendData();
    if (result == ESP_OK) {
      Serial.println("Sending confirmed");
      break;
    }
    else {
      Serial.println("Sending error, try next channel");
    }
  }
}

void espResponderInit() {

  // Set ESP32 as a Wi-Fi Station
  WiFi.mode(WIFI_AP_STA);
  // esp_wifi_set_protocol( WIFI_IF_STA , WIFI_PROTOCOL_LR);

  // Set device as a Wi-Fi Station
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
 
  // Initilize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register callback function
  esp_now_register_recv_cb(OnDataRecv);
}

esp_err_t espSendData() {

  // Create test data

  // Generate a random integer
  int_value = random(1,20);

  // Use integer to make a new float
  float_value = 1.3 * int_value;

  // Invert the boolean value
  bool_value = !bool_value;
  
  // Format structured data
  strcpy(myData.a, "Welcome to the Workshop!");

  myData.b = int_value;
  myData.c = float_value;
  myData.d = bool_value;

  Serial.printf("%s\n", myData.a);
  Serial.printf("%d\n", myData.b);
  Serial.printf("%f\n", myData.c);
  Serial.printf("%d\n", myData.d);
  
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result == ESP_OK) {
    Serial.println("Sending confirmed");
  }
  else {
    Serial.println("Sending error");
  }
  delay(2000);

  return result;
}