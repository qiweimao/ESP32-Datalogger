#ifndef AWS_MQTT_H
#define AWS_MQTT_H

extern WiFiClientSecure net;
extern PubSubClient client;

#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

void messageHandler(char* topic, byte* payload, unsigned int length);
void connectAWS();
void publishMessage();

#endif  // AWS_MQTT_H