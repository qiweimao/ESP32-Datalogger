#include "utils.h"

/* Data Collection Configuration */
struct DataCollectionConfig {
  String adcSensorType[16];
  bool adcEnabled[16];
  int adcInterval[16];
  String uartSensorType[2];
  bool uartEnabled[2];
  int uartInterval[2];
  String i2cSensorType[5];
  bool i2cEnabled[5];
  int i2cInterval[5];
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
void load_data_collection_configuration();
void update_data_collection_configuration(String type, int index, String key, String value);