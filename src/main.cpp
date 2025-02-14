#include "Global.h"
#include "web.h"
#include "fram.h"
#include "config.h"

#include <Blinker.h>

// OLED-экран
#define SSD1306_I2C_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Кнопка
const int BUTTON_PIN = D0; // GPIO16

// Встроенный светодиод
const int LED_PIN = LED_BUILTIN; // Встроенный светодиод
unsigned long lastLedBlink = 0;
bool ledState = LOW;
Blinker led(2);

unsigned long previousMillis = 0;
const long interval = 3600000; // Интервал измерений (1 час)
unsigned long lastMeasurementTime = 0;

// Время последнего нажатия кнопки
unsigned long lastButtonPress = 0;

void startAP(const String& ap_ssid, const String& ap_password);
int getTimezoneOffset();
void displaySensorData();
void blinkLED();

void setup() {
  Serial.begin(115200);
  delay(10);

  // Инициализация I2C
  Wire.begin();

  // Инициализация OLED-экрана
  if (!display.begin(SSD1306_I2C_ADDRESS, OLED_RESET)) {
    Serial.println("Не удалось найти OLED-экран!");
  } else {
    display.display();
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("TEST");
    display.display();
  }

  // Инициализация кнопки
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Инициализация светодиода
  //pinMode(LED_PIN, OUTPUT);
  //digitalWrite(LED_PIN, HIGH); // Выключить светодиод (активный LOW)

  // Инициализация файловой системы
  if (!LittleFS.begin()) {
    Serial.println("Ошибка при монтировании LittleFS");
    return;
  }

  loadConfig(); // Загрузка конфигурации

  // Инициализация BMP280
  if (!bmp.begin(0x76)) { // Адрес BMP280 может быть 0x76 или 0x77
    Serial.println("Не удалось найти BMP280!");
  }

  // Инициализация RTC DS1307
  if (!rtc.begin()) {
    Serial.println("Не удалось найти RTC DS1307!");
  } else if (!rtc.isrunning()) {
    Serial.println("RTC не работает! Установите время.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Установка текущего времени
  }

  // Инициализация HX711
  if (scale.is_ready()) {
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.set_scale(calibration_factor);
    scale.tare(); // Сброс веса
  }

  // Попытка подключения к Wi-Fi сети
  WiFi.begin(wifi_ssid, wifi_password);
  Serial.println("Подключение к Wi-Fi...");

  // Ожидание подключения
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  // Если подключение не удалось, запускаем точку доступа
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nНе удалось подключиться к Wi-Fi. Запуск точки доступа...");
    startAP(ap_ssid, ap_password);
    led.blinkForever(2, 1000);
  } else {
    Serial.println("\nПодключение к Wi-Fi успешно!");
    Serial.println(WiFi.localIP());
    led.blinkForever(1, 1000);

    // Синхронизация времени с NTP
    timeClient.begin();
    timeClient.setTimeOffset(getTimezoneOffset());
    timeClient.update();
    rtc.adjust(DateTime(timeClient.getEpochTime())); // Установка времени RTC

    // Проверка обновлений
    checkForUpdates();
  }

  // Инициализация веб-сервера
  initWebServer();
}

void loop() {
  unsigned long currentMillis = millis();

  // Периодическая проверка доступности Wi-Fi сети
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wi-Fi сеть недоступна. Запуск точки доступа...");
      startAP(default_ap_ssid, default_ap_password);
      led.blinkForever(2, 1000);
    } else {
      Serial.println("Wi-Fi сеть доступна.");
    }
  }

  // Измерение веса, температуры и давления раз в час
  if (currentMillis - lastMeasurementTime >= interval) {
    lastMeasurementTime = currentMillis;
    if (scale.is_ready()) {
        yield();
        weight = scale.get_units(5); // Получение среднего значения
    }
    float temperature = bmp.readTemperature();
    float pressure = bmp.readPressure() / 100.0F; // Переводим в гПа
    DateTime now = rtc.now();

    Measurement measurement = {now, weight, temperature, pressure};
    saveMeasurementToEEPROM(measurement);
    Serial.println("Измерение сохранено: " + String(weight) + " кг, " + String(temperature) + " °C, " + String(pressure) + " гПа");
  }

  // Обработка нажатия кнопки
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (currentMillis - lastButtonPress > 10000) { // Защита от дребезга
      lastButtonPress = currentMillis;
      displaySensorData();
    }
  }

  // Выключение экрана через 10 секунд
  if (currentMillis - lastButtonPress > 10000 && display.getRotation() != 0) {
    display.clearDisplay();
    display.display();
  }

  // Мигание светодиодом
  //blinkLED();

  // Обслуживание веб-сервера
  server.handleClient();
}

void startAP(const String& ap_ssid, const String& ap_password) {
  // Запуск точки доступа
  WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
  Serial.println("Точка доступа запущена");
  Serial.print("IP адрес: ");
  Serial.println(WiFi.softAPIP());
}

int getTimezoneOffset() {
  // Пример: "UTC+3" -> 3 * 3600 секунд
  if (timezone.startsWith("UTC")) {
    int offset = timezone.substring(3).toInt();
    return offset * 3600;
  }
  return 0; // По умолчанию UTC
}

void displaySensorData() {
  if (scale.is_ready()) {
    float weight = scale.get_units(10);
  }
  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure() / 100.0F;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Вес: " + String(weight) + " кг");
  display.println("Темп: " + String(temperature) + " °C");
  display.println("Давл: " + String(pressure) + " гПа");
  display.display();
}

void blinkLED() {
  unsigned long currentMillis = millis();
  int blinkInterval = (WiFi.getMode() == WIFI_AP) ? 500 : 1000; // 2 раза в секунду в AP, 1 раз в Wi-Fi

  if (currentMillis - lastLedBlink >= blinkInterval) {
    lastLedBlink = currentMillis;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? LOW : HIGH); // Инвертированное управление (активный LOW)
  }
}