
/* Data Collection Configuration */

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

extern String WIFI_SSID;
extern String WIFI_PASSWORD;
extern int LORA_MODE;
extern String DEVICE_NAME;
extern long gmtOffset_sec;
extern uint32_t  PAIRING_KEY;

extern DataCollectionConfig dataConfig;

void load_system_configuration();
void update_system_configuration(String key, String value);
void loadDataConfigFromPreferences();
void updateDataCollectionConfiguration(String type, int index, String key, String value);