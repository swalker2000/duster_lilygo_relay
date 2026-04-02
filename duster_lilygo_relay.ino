#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "Secret.h"
#include "Commutator.hpp"
#include "NetSettings.hpp"
#include "MqttCommandHandler.hpp"

const char deviceId[] = "device1";

WiFiClientSecure net;
PubSubClient client(net);
Commutator *commutator = new Commutator();
MqttCommandHandler mqttHandler(client, *commutator, deviceId);
NetSettings netSettings;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nESP32 MQTT JSON Example (PubSubClient + digitalWrite)");

  netSettings.begin();
  netSettings.reloadFromNvs();
  netSettings.printHelp();

  netSettings.reconnectWiFi();
  netSettings.reconnectMqtt(net, client, deviceId, MqttCommandHandler::callbackTrampoline);
}

void loop() {
  netSettings.pollSerial(net, client, deviceId, MqttCommandHandler::callbackTrampoline);
  client.loop();

  if (!client.connected()) {
    Serial.println("MQTT disconnected, reconnecting...");
    netSettings.reconnectMqtt(net, client, deviceId, MqttCommandHandler::callbackTrampoline);
  }
}
