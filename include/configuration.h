
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <Arduino.h>  // Include this header for fixed-width integer types

#define CHANNEL_COUNT 16

// size of the SystemConfig struct is 92 bytes.
struct SystemConfig {
  char WIFI_SSID[32];       // Adjust size as needed
  char WIFI_PASSWORD[32];   // Adjust size as needed
  char DEVICE_NAME[16];     // Adjust size as needed
  int LORA_MODE;
  int utcOffset;            // UTC offset in hours
  uint32_t PAIRING_KEY;
};

enum SensorType : uint8_t {
  Unknown,
  VibratingWire,
  Barometric,
  GeoPhone,
  Inclinometer,
  RainGauege,
};

  // DO I NEED TO KNOW THE PIN?
  // vwpz - identify channel number on the mux - yes
  // baro - scanned by i2c using address - no
  // geophone - on adc - yes
  // saa - rs232 addressing - no
  // rain gauge - i2c or uart - no
struct DataCollectionConfig {

  uint8_t channel_count = CHANNEL_COUNT; // read this byte to process config
  SensorType type[CHANNEL_COUNT];
  uint8_t pin[CHANNEL_COUNT];
  bool enabled[CHANNEL_COUNT];
  uint16_t interval[CHANNEL_COUNT];
  uint32_t time[CHANNEL_COUNT];

};

struct InfluxConfig {
  char INFLUXDB_URL[256];    // Adjust size as needed
  char INFLUXDB_TOKEN[256];  // Adjust size as needed
  char INFLUXDB_ORG[64];     // Adjust size as needed
  char INFLUXDB_BUCKET[64];  // Adjust size as needed
  char TZ_INFO[64];          // Adjust size as needed
};

// Expose structs
extern SystemConfig systemConfig;
extern DataCollectionConfig dataConfig;
extern InfluxConfig InfluxDBConfig;

void load_system_configuration();
void update_system_configuration(String key, String value);

void load_data_collection_configuration();
void update_data_collection_configuration(int channel, String key, int value);

void load_influx_db_configuration();
void update_influx_db_configuration(String key, String value);


#endif