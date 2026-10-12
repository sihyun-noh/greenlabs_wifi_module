#include <ArduinoJson.h>
#include <MQTTClient.h>
#include <WiFiClientSecure.h>
#include <global.h>

#include "WiFi.h"
#include "secrets.h"

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

//#define AWS_IOT_PUBLISH_TOPIC   "farmlabs/sensor_node/pub"
//#define AWS_IOT_PUBLISH_TOPIC   "farmlabs/sensor_node/pub"

#define AWS_IOT_SHADOW_GET "$aws/things/Esp32Policy/shadow/get/accepted"
#define AWS_IOT_SENT_GET "$aws/things/Esp32Policy/shadow/get"
#define AWS_IOT_SHADOW_UPDATE "$aws/things/Esp32Policy/shadow/update"

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client_AWS_MQTT = MQTTClient(256);

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  //  StaticJsonDocument<200> doc;
  //  deserializeJson(doc, payload);
  //  const char* message = doc["message"];
}

void ConnectAWS() {
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client_AWS_MQTT.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client_AWS_MQTT.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client_AWS_MQTT.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if (!client_AWS_MQTT.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client_AWS_MQTT.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void publishMessage() {
  StaticJsonDocument<512> doc;
  doc["time"] = 111111111;
  doc["sensor_a0"] = 123123;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);  // print to client
  client_AWS_MQTT.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}
/*
void publishMessage_SensorNode(char *str)
{
  client.publish(AWS_IOT_PUBLISH_TOPIC, str);
  client.loop();
}
*/
void AWS_IOT_publishMessage_SensorNode(char *str) {
  if (client_AWS_MQTT.publish(AWS_IOT_PUBLISH_TOPIC, str) != 0)
    Serial.println("Result published");
  else
    Serial.println("Failed published !!_ Error Check!!");
}

void AWS_IOT_shadow_update_SensorNode(char *str) {
  if (client_AWS_MQTT.publish(AWS_IOT_SHADOW_UPDATE, str) != 0)
    Serial.println("Result AWS_IOT_SHADOW_UPDATE");
  else
    Serial.println("Failed AWS_IOT_SHADOW_UPDATE !!_ Error Check!!");
}

void connected_check() {
  // Loop until we're reconnected
  while (!client_AWS_MQTT.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client_AWS_MQTT.connect(THINGNAME)) {  // change to ClientID
      Serial.println("connected");

      // ... and resubscribe
      client_AWS_MQTT.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

      // Once connected, publish an announcement...
      client_AWS_MQTT.publish(AWS_IOT_PUBLISH_TOPIC, "reconnect msg");

    } else {
      Serial.println(" try again in 5 seconds");
      delay(5000);

      // Wait 5 seconds before retrying
    }
  }
}

void Mqtt_loop() {
  client_AWS_MQTT.loop();
}

