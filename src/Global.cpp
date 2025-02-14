#include "Global.h"

// Настройки по умолчанию
const char* _default_ssid = "RT-GPON-6EE6";       // SSID Wi-Fi по умолчанию
const char* default_password = "UAQYHKZV"; // Пароль Wi-Fi по умолчанию
const char* default_ap_ssid = "BeeLogger";   // Имя точки доступа по умолчанию
const char* default_ap_password = "12345678"; // Пароль точки доступа по умолчанию
const char* default_timezone = "UTC+0";       // Часовой пояс по умолчанию

// Файл для хранения конфигурации
const char* config_file = "/config.json";

// Калибровочный коэффициент (хранится в LittleFS)
float calibration_factor = -7050.0; // Замените на ваш коэффициент

// Параметры HX711
const int LOADCELL_DOUT_PIN = D5;
const int LOADCELL_SCK_PIN = D6;
HX711 scale;

//Переменные для WiFi
String wifi_ssid;
String wifi_password;
String ap_ssid;
String ap_password;

WiFiClient wifiClient;
ESP8266WebServer server(80);

// NTP-клиент
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0); // Смещение 0, будем корректировать часовым поясом

// URL для проверки обновлений
const char* firmwareUpdateUrl = "http://kunakov.net/42902D74/firmware.bin";
const char* firmwareVersionUrl = "http://kunakov.net/42902D74/version.txt";

// Текущая версия прошивки
String currentFirmwareVersion = "3.1.6";

float weight;
float load; // Вес устанавливаемый на весы для колибровки > 0
float units = 0.035274; // Указываем коэффициент для перевода из унций в граммы

// Часовой пояс
String timezone = default_timezone;

// Датчик BMP280
Adafruit_BMP280 bmp; // I2C

// Модуль RTC DS1307
RTC_DS1307 rtc;

EEPROMHeader eepromHeader;