
/* Data Collection Configuration */

#include <Arduino.h>  // Include this header for fixed-width integer types

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

struct DataCollectionConfig {
  SensorType adcSensorType[16];  // 16 * 1 byte = 16 bytes
  bool adcEnabled[16];           // 16 * 1 byte = 16 bytes
  uint16_t adcInterval[16];      // 16 * 2 bytes = 32 bytes
  SensorType uartSensorType[2];  // 2 * 1 byte = 2 bytes
  bool uartEnabled[2];           // 2 * 1 byte = 2 bytes
  uint16_t uartInterval[2];      // 2 * 2 bytes = 4 bytes
  SensorType i2cSensorType[5];   // 5 * 1 byte = 5 bytes
  bool i2cEnabled[5];            // 5 * 1 byte = 5 bytes
  uint16_t i2cInterval[5];       // 5 * 2 bytes = 10 bytes
};

// Expose structs
extern SystemConfig systemConfig;
extern DataCollectionConfig dataConfig;

void load_system_configuration();
void update_system_configuration(String key, String value);
void loadDataConfigFromPreferences();
void updateDataCollectionConfiguration(String type, int index, String key, String value);