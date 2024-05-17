// Include Libraries
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "esp_now_init.h"
#include "utils.h"

int node_send_fail_count = 0;

void esp_now_setup(){
  Serial.println("\n*** Starting ESP-NOW ***");
  if (ESP_NOW_MODE == ESP_NOW_SENDER){
    Serial.println("Initialized as Sender");
    NodeInit();
  }
  if (ESP_NOW_MODE == ESP_NOW_RESPONDER){
    Serial.println("Initialized as Responder");
    GatewayInit();
  }
}

// ---------------------------- esp_ now -------------------------
void printMAC(const uint8_t * mac_addr){
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  oled_print(macStr);
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {

  if (ESP_NOW_MODE == ESP_NOW_SENDER){
    if(status != ESP_NOW_SEND_SUCCESS){
      node_send_fail_count++;
    }
    else{
      node_send_fail_count = 0;
    }
  }


  Serial.print("Last Packet Send Status: ");
  Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success to " : "Delivery Fail to ");
  printMAC(mac_addr);
  Serial.println();
}