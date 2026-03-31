#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "Secret.h"
#include "Commutator.hpp"

// ================= НАСТРОЙКИ =================
const char ssid[]     = SSID;
const char password[] = WIFI_PASS;

const char mqtt_broker[]   = URL;
const int  mqtt_port       = PORT;                 // для SSL
const char mqtt_username[] = MQTT_USERNAME;
const char mqtt_password[] = MQTT_PASS;

const char deviceId[] = "device1";                // идентификатор устройства

// ================= ГЛОБАЛЬНЫЕ ОБЪЕКТЫ =================
WiFiClientSecure net;
PubSubClient client(net);
Commutator *commutator = new Commutator();

void sendResponse(int messageId);
void sendResponse(int messageId, String deliveryStatus);
void messageReceived(char* topic, byte* payload, unsigned int length);

// ================= ФУНКЦИИ ПОДКЛЮЧЕНИЯ =================
void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected");
}

void connectToMQTT() {
  Serial.print("Connecting to MQTT broker");

  net.setInsecure();   // отключаем проверку сертификата (для теста)

  client.setServer(mqtt_broker, mqtt_port);
  client.setBufferSize(2048);              // увеличиваем буфер до 2048 байт
  client.setCallback(messageReceived);

  while (!client.connected()) {
    Serial.print(".");
    if (client.connect(deviceId, mqtt_username, mqtt_password)) {
      Serial.println("\nMQTT connected");
      String subscribeTopic = "consumer/request/" + String(deviceId);
      client.subscribe(subscribeTopic.c_str());
      Serial.println("Subscribed to: " + subscribeTopic);
    } else {
      delay(1000);
    }
  }
}

// ================= MQTT: разбор входящего сообщения =================
static String mqttPayloadToString(const byte* payload, unsigned int length) {
  String s;
  s.reserve(length);
  for (unsigned int i = 0; i < length; i++) {
    s += static_cast<char>(payload[i]);
  }
  return s;
}

static void logIncomingMqtt(const String& topicStr, const String& payloadStr) {
  Serial.println("Message received:");
  Serial.println("Topic: " + topicStr);
  Serial.println("Payload length: " + String(payloadStr.length()));
  Serial.println("Payload: " + payloadStr);
}

static bool mqttTopicIsForThisDevice(const String& topicStr) {
  const char expectedPrefix[] = "consumer/request/";
  if (!topicStr.startsWith(expectedPrefix)) {
    return false;
  }
  String receivedDeviceId = topicStr.substring(sizeof(expectedPrefix) - 1);
  if (receivedDeviceId != deviceId) {
    Serial.println("Device ID mismatch – ignoring");
    return false;
  }
  return true;
}

static bool parseMqttJson(const String& payloadStr, DynamicJsonDocument& doc) {
  DeserializationError error = deserializeJson(doc, payloadStr);
  if (error) {
    Serial.print("JSON parsing error: ");
    Serial.println(error.c_str());
    return false;
  }
  return true;
}

static void handleRelayWriteCommand(JsonObject data) {
  if (data.isNull()) {
    Serial.println("Error: data field missing for digitalWrite command");
    return;
  }
  int pinNumber = data["pinNumber"] | -1;
  bool pinValue = data["pinValue"] | false;
  if (!commutator->isRelayNumberValid(pinNumber)) {
    Serial.println("Error: pinNumber missing or invalid");
    return;
  }
  commutator->write(pinNumber, pinValue);
  Serial.printf("digitalWrite: pin %d set to %s\n", pinNumber, pinValue ? "HIGH" : "LOW");
}

static void handleBlinkCommand(long id, JsonObject data) {
  if (data.isNull()) {
    Serial.println("Error: data field missing for digitalWrite command");
    sendResponse(id, "COMPLETED_WITH_ERROR");
    return;
  }
  int pinNumber = data["pinNumber"] | -1;
  int period = data["period"] | -1;
  int count = data["count"] | -1;
  if (!commutator->isRelayNumberValid(pinNumber) || period == -1 || count == -1) {
    Serial.println("Error: some values missing or invalid");
    sendResponse(id, "COMPLETED_WITH_ERROR");
    return;
  }
  Serial.println("Blink start");
  commutator->blink(pinNumber, period, count);
  Serial.println("Blink stop");
  sendResponse(id, "COMPLETED");
}

static void dispatchMqttCommand(DynamicJsonDocument& doc, long id) {
  const char* command = doc["command"];
  Serial.print("Extracted command: ");
  Serial.println(command ? command : "(null)");

  if (!command) {
    return;
  }
  if (strcmp(command, "relaylWrite") == 0) {
    handleRelayWriteCommand(doc["data"].as<JsonObject>());
  } else if (strcmp(command, "blink") == 0) {
    handleBlinkCommand(id, doc["data"].as<JsonObject>());
  }
}

void messageReceived(char* topic, byte* payload, unsigned int length) {
  String topicStr = String(topic);
  String payloadStr = mqttPayloadToString(payload, length);
  logIncomingMqtt(topicStr, payloadStr);
  if (!mqttTopicIsForThisDevice(topicStr)) {
    return;
  }

  DynamicJsonDocument doc(1024);
  if (!parseMqttJson(payloadStr, doc)) {
    return;
  }

  long id = doc["id"];
  sendResponse(id);
  dispatchMqttCommand(doc, id);
}


void sendResponse(int messageId, String deliveryStatus)
{
  // --- Формирование и отправка ответа (всегда содержит только id) ---
  StaticJsonDocument<128> responseDoc;
  responseDoc["id"] = messageId;
  responseDoc["deliveryStatus"] = deliveryStatus;

  String responsePayload;
  serializeJson(responseDoc, responsePayload);

  String responseTopic = "consumer/response/" + String(deviceId);
  if (client.publish(responseTopic.c_str(), responsePayload.c_str())) {
    Serial.println("Response sent to " + responseTopic);
    Serial.println("Response payload: " + responsePayload);
  } else {
    Serial.println("Failed to send response");
  }
}

void sendResponse(int messageId)
{
  // --- Формирование и отправка ответа (всегда содержит только id) ---
  StaticJsonDocument<128> responseDoc;
  responseDoc["id"] = messageId;

  String responsePayload;
  serializeJson(responseDoc, responsePayload);

  String responseTopic = "consumer/response/" + String(deviceId);
  if (client.publish(responseTopic.c_str(), responsePayload.c_str())) {
    Serial.println("Response sent to " + responseTopic);
    Serial.println("Response payload: " + responsePayload);
  } else {
    Serial.println("Failed to send response");
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nESP32 MQTT JSON Example (PubSubClient + digitalWrite)");

  connectToWiFi();
  connectToMQTT();
}

// ================= LOOP =================
void loop() {
  client.loop();   // обязательно для поддержания связи и приёма сообщений

  // Если потеряли соединение с MQTT – переподключаемся
  if (!client.connected()) {
    Serial.println("MQTT disconnected, reconnecting...");
    connectToMQTT();
  }

  // Другие задачи...
}