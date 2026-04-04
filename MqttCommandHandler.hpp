#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "Commutator.hpp"

#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

class MqttCommandHandler {
public:
  MqttCommandHandler(PubSubClient &client, Commutator &commutator, const char *deviceId)
      : client_(client), commutator_(commutator), deviceId_(deviceId) {
    instance_ = this;
  }

  void onMessage(char *topic, byte *payload, unsigned int length);

  void sendResponse(int messageId);
  void sendResponse(int messageId, const String &deliveryStatus);

  static void callbackTrampoline(char *topic, byte *payload, unsigned int length) {
    if (instance_) {
      instance_->onMessage(topic, payload, length);
    }
  }

private:
  static MqttCommandHandler *instance_;

  PubSubClient &client_;
  Commutator &commutator_;
  const char *deviceId_;

  static String payloadToString(const byte *payload, unsigned int length);
  void logIncoming(const String &topicStr, const String &payloadStr) const;
  bool topicIsForThisDevice(const String &topicStr) const;
  static bool parseJson(const String &payloadStr, DynamicJsonDocument &doc);
  void handleRelayWrite(JsonObject data);
  void handleBlink(long id, JsonObject data);
  void dispatchCommand(DynamicJsonDocument &doc, long id);
};

MqttCommandHandler *MqttCommandHandler::instance_ = nullptr;

inline String MqttCommandHandler::payloadToString(const byte *payload, unsigned int length) {
  String s;
  s.reserve(length);
  for (unsigned int i = 0; i < length; i++) {
    s += static_cast<char>(payload[i]);
  }
  return s;
}

inline void MqttCommandHandler::logIncoming(const String &topicStr, const String &payloadStr) const {
  Serial.println(F("Message received:"));
  Serial.println(String(F("Topic: ")) + topicStr);
  Serial.println(String(F("Payload length: ")) + String(payloadStr.length()));
  Serial.println(String(F("Payload: ")) + payloadStr);
}

inline bool MqttCommandHandler::topicIsForThisDevice(const String &topicStr) const {
  const char expectedPrefix[] = "consumer/request/";
  if (!topicStr.startsWith(expectedPrefix)) {
    return false;
  }
  String receivedDeviceId = topicStr.substring(sizeof(expectedPrefix) - 1);
  if (receivedDeviceId != deviceId_) {
    Serial.println(F("Device ID mismatch – ignoring"));
    return false;
  }
  return true;
}

inline bool MqttCommandHandler::parseJson(const String &payloadStr, DynamicJsonDocument &doc) {
  DeserializationError error = deserializeJson(doc, payloadStr);
  if (error) {
    Serial.print(F("JSON parsing error: "));
    Serial.println(error.c_str());
    return false;
  }
  return true;
}

inline void MqttCommandHandler::handleRelayWrite(JsonObject data) {
  if (data.isNull()) {
    Serial.println(F("Error: data field missing for digitalWrite command"));
    return;
  }
  int pinNumber = data["pinNumber"] | -1;
  bool pinValue = data["pinValue"] | false;
  if (!commutator_.isRelayNumberValid(pinNumber)) {
    Serial.println(F("Error: pinNumber missing or invalid"));
    return;
  }
  commutator_.write(pinNumber, pinValue);
  Serial.printf("digitalWrite: pin %d set to %s\n", pinNumber, pinValue ? "HIGH" : "LOW");
}

inline void MqttCommandHandler::handleBlink(long id, JsonObject data) {
  if (data.isNull()) {
    Serial.println(F("Error: data field missing for digitalWrite command"));
    sendResponse(id, "COMPLETED_WITH_ERROR");
    return;
  }
  int pinNumber = data["pinNumber"] | -1;
  int period = data["period"] | -1;
  int count = data["count"] | -1;
  if (!commutator_.isRelayNumberValid(pinNumber) || period == -1 || count == -1) {
    Serial.println(F("Error: some values missing or invalid"));
    sendResponse(id, "COMPLETED_WITH_ERROR");
    return;
  }
  Serial.println(F("Blink start"));
  commutator_.blink(pinNumber, period, count);
  Serial.println(F("Blink stop"));
  sendResponse(id, "COMPLETED");
}

inline void MqttCommandHandler::dispatchCommand(DynamicJsonDocument &doc, long id) {
  const char *command = doc["command"];
  Serial.print(F("Extracted command: "));
  Serial.println(command ? command : "(null)");

  if (!command) {
    return;
  }
  if (strcmp(command, "relayWrite") == 0) {
    handleRelayWrite(doc["data"].as<JsonObject>());
  } else if (strcmp(command, "blink") == 0) {
    handleBlink(id, doc["data"].as<JsonObject>());
  }else
  {
    Serial.println(F("Command not found!"));
  }
}

inline void MqttCommandHandler::onMessage(char *topic, byte *payload, unsigned int length) {
  String topicStr = String(topic);
  String payloadStr = payloadToString(payload, length);
  logIncoming(topicStr, payloadStr);
  if (!topicIsForThisDevice(topicStr)) {
    return;
  }

  DynamicJsonDocument doc(1024);
  if (!parseJson(payloadStr, doc)) {
    return;
  }

  long id = doc["id"];
  sendResponse(id);
  dispatchCommand(doc, id);
}

inline void MqttCommandHandler::sendResponse(int messageId, const String &deliveryStatus) {
  StaticJsonDocument<128> responseDoc;
  responseDoc["id"] = messageId;
  responseDoc["deliveryStatus"] = deliveryStatus;

  String responsePayload;
  serializeJson(responseDoc, responsePayload);

  String responseTopic = "consumer/response/" + String(deviceId_);
  if (client_.publish(responseTopic.c_str(), responsePayload.c_str())) {
    Serial.println(String(F("Response sent to ")) + responseTopic);
    Serial.println(String(F("Response payload: ")) + responsePayload);
  } else {
    Serial.println(F("Failed to send response"));
  }
}

inline void MqttCommandHandler::sendResponse(int messageId) {
  StaticJsonDocument<128> responseDoc;
  responseDoc["id"] = messageId;

  String responsePayload;
  serializeJson(responseDoc, responsePayload);

  String responseTopic = "consumer/response/" + String(deviceId_);
  if (client_.publish(responseTopic.c_str(), responsePayload.c_str())) {
    Serial.println(String(F("Response sent to ")) + responseTopic);
    Serial.println(String(F("Response payload: ")) + responsePayload);
  } else {
    Serial.println(F("Failed to send response"));
  }
}
#endif
