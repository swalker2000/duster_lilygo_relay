#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "Secret.h"

#ifndef NET_SETTINGS_H
#define NET_SETTINGS_H

class NetSettings {
public:
  static constexpr size_t kSsidMax = 33;
  static constexpr size_t kWifiPassMax = 65;
  static constexpr size_t kMqttHostMax = 129;
  static constexpr size_t kMqttUserMax = 65;
  static constexpr size_t kMqttPassMax = 65;

  void begin() { prefs_.begin("netcfg", false); }

  void reloadFromNvs() {
    strlcpy(wifiSsid_, prefs_.getString("ssid", SECRET_WIFI_SSID).c_str(), sizeof(wifiSsid_));
    strlcpy(wifiPass_, prefs_.getString("wifipass", WIFI_PASS).c_str(), sizeof(wifiPass_));
    strlcpy(mqttHost_, prefs_.getString("mhost", URL).c_str(), sizeof(mqttHost_));
    mqttPort_ = static_cast<uint16_t>(prefs_.getUInt("mport", static_cast<uint32_t>(PORT)));
    if (mqttPort_ == 0) {
      mqttPort_ = PORT;
    }
    strlcpy(mqttUser_, prefs_.getString("muser", MQTT_USERNAME).c_str(), sizeof(mqttUser_));
    strlcpy(mqttPass_, prefs_.getString("mpass", MQTT_PASS).c_str(), sizeof(mqttPass_));
  }

  const char *wifiSsid() const { return wifiSsid_; }
  const char *wifiPass() const { return wifiPass_; }
  const char *mqttHost() const { return mqttHost_; }
  uint16_t mqttPort() const { return mqttPort_; }
  const char *mqttUser() const { return mqttUser_; }
  const char *mqttPass() const { return mqttPass_; }

  void reconnectWiFi() {
    WiFi.disconnect(true);
    delay(200);
    Serial.print(F("Wi-Fi: подключение к "));
    Serial.println(wifiSsid_);
    WiFi.begin(wifiSsid_, wifiPass_);
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000UL) {
      delay(500);
      Serial.print(F("."));
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("Wi-Fi подключён"));
    } else {
      Serial.println(F("Wi-Fi: таймаут, проверьте ssid / wifipass"));
    }
  }

  void reconnectMqtt(WiFiClientSecure &net, PubSubClient &client, const char *deviceId,
                     void (*mqttCallback)(char *, byte *, unsigned int)) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println(F("MQTT: сначала нужен Wi-Fi"));
      return;
    }
    client.disconnect();
    delay(100);
    Serial.print(F("MQTT: подключение к "));
    Serial.print(mqttHost_);
    Serial.print(F(":"));
    Serial.println(mqttPort_);

    net.setInsecure();
    client.setServer(mqttHost_, mqttPort_);
    client.setBufferSize(2048);
    client.setCallback(mqttCallback);

    unsigned long t0 = millis();
    while (!client.connected() && millis() - t0 < 20000UL) {
      Serial.print(F("."));
      if (client.connect(deviceId, mqttUser_, mqttPass_)) {
        Serial.println(F("\nMQTT подключён"));
        String subscribeTopic = "consumer/request/" + String(deviceId);
        client.subscribe(subscribeTopic.c_str());
        Serial.println(String(F("Подписка: ")) + subscribeTopic);
        return;
      }
      delay(1000);
    }
    Serial.println(F("\nMQTT: не удалось подключиться"));
  }

  void printHelp() const {
    Serial.println(F("--- Консоль настроек (дефолты из Secret.h) ---"));
    Serial.println(F("help              — эта справка"));
    Serial.println(F("show              — текущие значения"));
    Serial.println(F("ssid <текст>      — Wi-Fi SSID"));
    Serial.println(F("wifipass <текст>  — пароль Wi-Fi"));
    Serial.println(F("mhost <текст>     — хост MQTT (было URL)"));
    Serial.println(F("mport <число>     — порт MQTT"));
    Serial.println(F("muser <текст>     — логин MQTT"));
    Serial.println(F("mpass <текст>     — пароль MQTT"));
    Serial.println(F("default <ключ>    — сброс одного параметра к Secret.h"));
    Serial.println(F("  ключи: ssid | wifipass | mhost | mport | muser | mpass | all"));
    Serial.println(F("reconnect         — переподключить Wi-Fi и MQTT по текущим значениям"));
    Serial.println(F("reboot            — перезагрузка ESP32"));
  }

  void pollSerial(WiFiClientSecure &net, PubSubClient &client, const char *deviceId,
                  void (*mqttCallback)(char *, byte *, unsigned int)) {
    while (Serial.available() > 0) {
      char c = static_cast<char>(Serial.read());
      if (c == '\r') {
        continue;
      }
      if (c == '\n') {
        processCommandLine(serialLine_, net, client, deviceId, mqttCallback);
        serialLine_ = "";
        continue;
      }
      if (serialLine_.length() < 512) {
        serialLine_ += c;
      }
    }
  }

private:
  Preferences prefs_;
  char wifiSsid_[kSsidMax]{};
  char wifiPass_[kWifiPassMax]{};
  char mqttHost_[kMqttHostMax]{};
  char mqttUser_[kMqttUserMax]{};
  char mqttPass_[kMqttPassMax]{};
  uint16_t mqttPort_ = PORT;
  String serialLine_;

  void printCurrentConfig() const {
    Serial.println(F("--- Активные настройки ---"));
    Serial.print(F("ssid:     "));
    Serial.println(wifiSsid_);
    Serial.print(F("wifipass: "));
    Serial.println(strlen(wifiPass_) ? F("***") : F("(пусто)"));
    Serial.print(F("mhost:    "));
    Serial.println(mqttHost_);
    Serial.print(F("mport:    "));
    Serial.println(mqttPort_);
    Serial.print(F("muser:    "));
    Serial.println(mqttUser_);
    Serial.print(F("mpass:    "));
    Serial.println(strlen(mqttPass_) ? F("***") : F("(пусто)"));
    Serial.println(F("(значения без переопределения в NVS совпадают с Secret.h)"));
  }

  static bool copyToBuf(const String &src, char *dst, size_t dstSize, const char *name) {
    if (src.length() >= dstSize) {
      Serial.print(name);
      Serial.print(F(": слишком длинная строка, макс. "));
      Serial.println(static_cast<int>(dstSize - 1));
      return false;
    }
    strlcpy(dst, src.c_str(), dstSize);
    return true;
  }

  void processCommandLine(const String &lineIn, WiFiClientSecure &net, PubSubClient &client,
                          const char *deviceId, void (*mqttCallback)(char *, byte *, unsigned int)) {
    String line = lineIn;
    line.trim();
    if (line.length() == 0) {
      return;
    }

    int sp = line.indexOf(' ');
    String cmd = sp < 0 ? line : line.substring(0, sp);
    cmd.trim();
    String arg = sp < 0 ? String() : line.substring(sp + 1);
    arg.trim();

    if (cmd.equalsIgnoreCase("help") || cmd == "?") {
      printHelp();
      return;
    }
    if (cmd.equalsIgnoreCase("show")) {
      printCurrentConfig();
      return;
    }
    if (cmd.equalsIgnoreCase("reconnect")) {
      reconnectWiFi();
      reconnectMqtt(net, client, deviceId, mqttCallback);
      return;
    }
    if (cmd.equalsIgnoreCase("reboot")) {
      Serial.println(F("Перезагрузка..."));
      delay(300);
      ESP.restart();
      return;
    }

    if (cmd.equalsIgnoreCase("default")) {
      if (arg.length() == 0) {
        Serial.println(F("Укажите ключ: ssid | wifipass | mhost | mport | muser | mpass | all"));
        return;
      }
      if (arg.equalsIgnoreCase("all")) {
        prefs_.clear();
        reloadFromNvs();
        Serial.println(F("Все параметры сброшены к Secret.h. Выполните reconnect или reboot."));
        return;
      }
      if (arg.equalsIgnoreCase("ssid")) {
        prefs_.remove("ssid");
      } else if (arg.equalsIgnoreCase("wifipass")) {
        prefs_.remove("wifipass");
      } else if (arg.equalsIgnoreCase("mhost")) {
        prefs_.remove("mhost");
      } else if (arg.equalsIgnoreCase("mport")) {
        prefs_.remove("mport");
      } else if (arg.equalsIgnoreCase("muser")) {
        prefs_.remove("muser");
      } else if (arg.equalsIgnoreCase("mpass")) {
        prefs_.remove("mpass");
      } else {
        Serial.println(F("Неизвестный ключ для default"));
        return;
      }
      reloadFromNvs();
      Serial.println(F("Параметр сброшен к Secret.h. При необходимости: reconnect"));
      return;
    }

    bool changed = false;
    if (cmd.equalsIgnoreCase("ssid")) {
      if (arg.length() == 0) {
        Serial.println(F("Пример: ssid MyNetwork"));
        return;
      }
      if (!copyToBuf(arg, wifiSsid_, sizeof(wifiSsid_), "ssid")) {
        return;
      }
      prefs_.putString("ssid", arg);
      changed = true;
      Serial.println(F("ssid сохранён в NVS"));
    } else if (cmd.equalsIgnoreCase("wifipass")) {
      if (!copyToBuf(arg, wifiPass_, sizeof(wifiPass_), "wifipass")) {
        return;
      }
      prefs_.putString("wifipass", arg);
      changed = true;
      Serial.println(F("wifipass сохранён в NVS"));
    } else if (cmd.equalsIgnoreCase("mhost")) {
      if (arg.length() == 0) {
        Serial.println(F("Пример: mhost 192.168.1.10"));
        return;
      }
      if (!copyToBuf(arg, mqttHost_, sizeof(mqttHost_), "mhost")) {
        return;
      }
      prefs_.putString("mhost", arg);
      changed = true;
      Serial.println(F("mhost сохранён в NVS"));
    } else if (cmd.equalsIgnoreCase("mport")) {
      int p = arg.toInt();
      if (p < 1 || p > 65535) {
        Serial.println(F("mport: укажите число 1…65535"));
        return;
      }
      mqttPort_ = static_cast<uint16_t>(p);
      prefs_.putUInt("mport", static_cast<uint32_t>(mqttPort_));
      changed = true;
      Serial.println(F("mport сохранён в NVS"));
    } else if (cmd.equalsIgnoreCase("muser")) {
      if (!copyToBuf(arg, mqttUser_, sizeof(mqttUser_), "muser")) {
        return;
      }
      prefs_.putString("muser", arg);
      changed = true;
      Serial.println(F("muser сохранён в NVS"));
    } else if (cmd.equalsIgnoreCase("mpass")) {
      if (!copyToBuf(arg, mqttPass_, sizeof(mqttPass_), "mpass")) {
        return;
      }
      prefs_.putString("mpass", arg);
      changed = true;
      Serial.println(F("mpass сохранён в NVS"));
    } else {
      Serial.print(F("Неизвестная команда: "));
      Serial.println(cmd);
      Serial.println(F("Введите help"));
      return;
    }

    if (changed) {
      Serial.println(F("Чтобы применить сеть/MQTT: reconnect"));
    }
  }
};
#endif
