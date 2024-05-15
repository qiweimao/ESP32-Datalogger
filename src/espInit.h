
// extern struct_message myData;

void initESP_NOW();

void espSenderInit();
void testChannel(int32_t channel);
esp_err_t espSendData();
void espResponderInit();