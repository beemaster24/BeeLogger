#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <HX711.h>
#include <Wire.h> // Для работы с I2C
#include <Adafruit_BMP280.h> // Для BMP280
#include <RTClib.h> // Для DS1307
#include <NTPClient.h> // Для синхронизации времени
#include <WiFiUdp.h> // Для NTP
#include <GyverOLED.h> //Меняем OLED на Gyver'a
#include <ESP8266HTTPClient.h> // Для HTTP-запросов
#include <ESP8266httpUpdate.h> // Для OTA-обновлений
#include <EncButton.h> // Юзаем кнопку от Gyver

// Настройки по умолчанию
extern const char* _default_ssid;       // SSID Wi-Fi по умолчанию
extern const char* default_password; // Пароль Wi-Fi по умолчанию
extern const char* default_ap_ssid;   // Имя точки доступа по умолчанию
extern const char* default_ap_password; // Пароль точки доступа по умолчанию
extern const char* default_timezone;       // Часовой пояс по умолчанию

// Файл для хранения конфигурации
extern const char* config_file;

extern float calibration_factor;

extern String wifi_ssid;
extern String wifi_password;
extern String ap_ssid;
extern String ap_password;

extern const int LOADCELL_DOUT_PIN;
extern const int LOADCELL_SCK_PIN;


extern WiFiClient wifiClient;
extern ESP8266WebServer server;

extern HX711 scale;

extern WiFiUDP ntpUDP;
extern NTPClient timeClient;

// URL для проверки обновлений
extern const char* firmwareUpdateUrl;
extern const char* firmwareVersionUrl;

// Текущая версия прошивки
extern String currentFirmwareVersion;

extern float weight;
extern float load; // Вес устанавливаемый на весы для колибровки > 0
extern float units;

extern String timezone;

// Датчик BMP280
extern Adafruit_BMP280 bmp;

// Модуль RTC DS1307
extern RTC_DS1307 rtc;

// Адрес EEPROM FM24CL64B
#define EEPROM_I2C_ADDRESS 0x50

//Флаг вкл/выкл экрана OLED
extern bool enOLED;

// Структура для хранения данных измерений
struct Measurement {
    DateTime timestamp; // Время с RTC
    float weight;       // Вес
    float temperature;  // Температура с BMP280
    float pressure;     // Давление с BMP280
  };
  
  // Заголовок EEPROM
  struct EEPROMHeader {
    uint16_t recordCount; // Количество записей
  };
  
  extern EEPROMHeader eepromHeader;