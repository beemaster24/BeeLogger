#include "Global.h"
#include "config.h"

void loadConfig() {
    if (LittleFS.exists(config_file)) {
      File file = LittleFS.open(config_file, "r");
      if (file) {
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, file);
        if (!error) {
          wifi_ssid = doc["wifi_ssid"].as<String>();
          wifi_password = doc["wifi_password"].as<String>();
          ap_ssid = doc["ap_ssid"].as<String>();
          ap_password = doc["ap_password"].as<String>();
          calibration_factor = doc["calibration_factor"];
          timezone = doc["timezone"].as<String>();
          currentFirmwareVersion = doc["firmware_version"].as<String>();
          Serial.println("Загружена конфигурация: версия " + String(currentFirmwareVersion));
        } else {
          Serial.println("Ошибка при чтении конфигурации.");
        }
        file.close();
      }
    } else {
      // Использование значений по умолчанию, если файл не существует
      wifi_ssid = _default_ssid;
      wifi_password = default_password;
      ap_ssid = default_ap_ssid;
      ap_password = default_ap_password;
      saveConfig();
    }
  }
  
  void saveConfig() {
    StaticJsonDocument<512> doc;
    doc["wifi_ssid"] = wifi_ssid;
    doc["wifi_password"] = wifi_password;
    doc["ap_ssid"] = ap_ssid;
    doc["ap_password"] = ap_password;
    doc["calibration_factor"] = calibration_factor;
    doc["timezone"] = timezone;
    doc["firmware_version"] = currentFirmwareVersion;
    doc["last_update"] = rtc.now().timestamp(); // Дата последнего обновления
  
    File file = LittleFS.open(config_file, "w");
    if (file) {
      serializeJson(doc, file);
      file.close();
      Serial.println("Конфигурация сохранена.");
    } else {
      Serial.println("Ошибка при сохранении конфигурации.");
    }
  }