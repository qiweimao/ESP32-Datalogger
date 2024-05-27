#include "utils.h"

extern String WIFI_SSID;
extern String WIFI_PASSWORD;
extern int LORA_MODE;
extern String DEVICE_NAME;
extern long gmtOffset_sec;

void load_system_configuration();
void update_system_configuration(String newSSID, String newWiFiPassword, long newgmtOffset_sec, int newLORA_MODE, String newProjectName);