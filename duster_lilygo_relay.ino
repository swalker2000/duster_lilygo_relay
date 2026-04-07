#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "Secret.h"
#include "Commutator.hpp"
#include "NetSettings.hpp"
#include "MqttCommandHandler.hpp"

const char deviceId[] = "esp32-relay-1";

WiFiClient mqttPlain;
WiFiClientSecure mqttSecure;
PubSubClient client(mqttPlain);
Commutator *commutator = new Commutator();
MqttCommandHandler mqttHandler(client, *commutator, deviceId);
NetSettings netSettings;

void setup() {
  Serial.begin(115200);
  commutator->begin();
  delay(1000);
  Serial.println("\nESP32 MQTT JSON Example (PubSubClient + digitalWrite)");

  netSettings.begin();
  netSettings.reloadFromNvs();
  netSettings.printHelp();

  netSettings.reconnectWiFi();
  netSettings.reconnectMqtt(mqttPlain, mqttSecure, client, deviceId,
                            MqttCommandHandler::callbackTrampoline);
}

void loop() {
  netSettings.pollSerial(mqttPlain, mqttSecure, client, deviceId,
                         MqttCommandHandler::callbackTrampoline);
  client.loop();

  if (!client.connected()) {
    Serial.println("MQTT disconnected, reconnecting...");
    netSettings.reconnectMqtt(mqttPlain, mqttSecure, client, deviceId,
                              MqttCommandHandler::callbackTrampoline);
  }
}
