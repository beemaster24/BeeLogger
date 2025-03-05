/*
MIT License

Copyright (c) 2025 Vyacheslav V. Kunakov
*/

#include "Global.h"
#include "web.h"
#include "fram.h"
#include "config.h"

#include <Blinker.h>

unsigned long currentMillis;

const int arraySize = 4; // Размер массива
String stringArray[arraySize]; // Массив строк

// OLED-экран
GyverOLED<SSD1306_128x32, OLED_NO_BUFFER> oled;
unsigned int oledView = 10000;
unsigned int oledPrevMillis = 0;

// Кнопка
Button btn(D0); // GPIO16

Blinker led(2);

unsigned long previousMillis = 0;
const long interval = 3600000; // Интервал измерений (1 час)
unsigned long lastMeasurementTime = 0;

// Время последнего нажатия кнопки
unsigned long lastButtonPress = 0;

void startAP(const String& ap_ssid, const String& ap_password);
int getTimezoneOffset();
void viewData();
void oledLog(String newString);

void setup() {
  Serial.begin(115200);
  delay(10);

  // Инициализация I2C
  Wire.begin();

  // Инициализация OLED-экрана
  enOLED = true;
  oled.init();
  oled.clear();
  oled.setScale(1);
  
  oledLog("System starting...");
  delay(100);

  // Инициализация файловой системы
  if (!LittleFS.begin()) {
    Serial.println("Ошибка при монтировании LittleFS");
    return;
  }

  loadConfig(); // Загрузка конфигурации

  // Инициализация BMP280
  if (!bmp.begin(0x76)) { // Адрес BMP280 может быть 0x76 или 0x77
    Serial.println("Не удалось найти BMP280!");
    oledLog("BMP280 not found!");
    delay(100);
  } else {
    oledLog("BMP280 is OK...");
    delay(100);
  }

  // Инициализация RTC DS1307
  if (!rtc.begin()) {
    Serial.println("Не удалось найти RTC DS1307!");
    oledLog("RTC not found!");
    delay(100);
  } else if (!rtc.isrunning()) {
    Serial.println("RTC не работает! Установите время.");
    oledLog("RTC is OK...");
    delay(100);
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Установка текущего времени
  }

  // Инициализация HX711
  if (scale.is_ready()) {
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.set_scale(calibration_factor);
    //scale.tare(); // Сброс веса
    //scale.power_down();
    oledLog("Scale ready for work...");
    delay(100);
  } else {
    oledLog("Load cell not found!");
    delay(100);
  }

  WiFiConnector.setName(ap_ssid);
  WiFiConnector.setPass(ap_password);

  WiFiConnector.connect(wifi_ssid, wifi_password);

  oledLog("Connecting to WiFi");

  // Ожидание подключения
  unsigned long startTime = millis();
  while (WiFiConnector.connected() != true && millis() - startTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  // Если подключение не удалось, запускаем точку доступа
  WiFiConnector.onError([]() {
    Serial.println("\nНе удалось подключиться к Wi-Fi. Запуск точки доступа...");
    oledLog("No WiFi, AP started...");
    //Мигаем светодиодом если не подключились
    led.blinkForever(3000, 100);
    delay(100);
  });

  WiFiConnector.onConnect([]() {
    Serial.println("\nПодключение к Wi-Fi успешно!");
    oledLog("Connected to WiFi");
    Serial.println(WiFi.localIP());
    oledLog(WiFi.localIP().toString());
    delay(100);
    //Мигание светодиодом если подключились
    led.blinkForever(5000, 100);

    // Синхронизация времени с NTP
    timeClient.begin();
    timeClient.setTimeOffset(getTimezoneOffset());
    timeClient.update();
    rtc.adjust(DateTime(timeClient.getEpochTime())); // Установка времени RTC

    // Проверка обновлений
    checkForUpdates();
    delay(500);
  //}
  });

  // Инициализация веб-сервера
  initWebServer();

  currentMillis = millis();
  oledPrevMillis = currentMillis;
}

void loop() {

  currentMillis = millis();

  btn.tick();

  if (btn.hold(1) && enOLED == false) {
    //Клик-удержание - показываем текущие данные датчиков на OLED в течении 10 секунд
    viewData();
  }

  if (btn.hold(2)) {
    // Клик клик-удержание - откл/вкл WiFi. Выставляем флаг WiFi ON/OFF
  }

  //Отключаем OLED песле 10 секунд отображения онформации
  if (currentMillis - oledPrevMillis >= oledView && enOLED == true) {
    oled.clear();
    oled.setPower(false);
    enOLED = false;
  }

  // Периодическая проверка доступности Wi-Fi сети
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wi-Fi сеть недоступна. Запуск точки доступа...");
      startAP(default_ap_ssid, default_ap_password);
      led.blinkForever(3000, 100);
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

  // Выключение экрана через 10 секунд
  //oled.setPower(false);

  // Обслуживание веб-сервера
  server.handleClient();

  // Обрабатываем светодиод
  led.tick();

  WiFiConnector.tick();
}

void viewData() {
  enOLED = true;
  if (scale.is_ready()) {
    yield();
    weight = scale.get_units(10);
  } else {
    weight = 0;
  }
  float temp = bmp.readTemperature();
  float pres = bmp.readPressure();

  oled.clear();
  oled.setPower(true);
  oled.setScale(3);
  oled.setCursor(0, 8);
  oled.print(weight);
  oled.print("кг.");
  oled.setScale(2);
  oled.setCursor(64, 0);
  oled.print(temp);
  oled.print("мм");
  oled.setCursor(64, 16);
  oled.print(pres);
  oled.print("C");
  currentMillis = millis();
  oledPrevMillis = currentMillis;
}

void startAP(const String& ap_ssid, const String& ap_password) {
  // Запуск точки доступа
  WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
  Serial.println("Точка доступа запущена");
  Serial.print("IP адрес: ");
  Serial.println(WiFi.softAPIP());
  oledLog(WiFi.softAPIP().toString());
}

int getTimezoneOffset() {
  // Пример: "UTC+3" -> 3 * 3600 секунд
  if (timezone.startsWith("UTC")) {
    int offset = timezone.substring(3).toInt();
    return offset * 3600;
  }
  return 0; // По умолчанию UTC
}

void oledLog(String newString) {
  oled.clear();
  oled.home();

  // Сдвигаем массив влево если он полностью заполнен
  if (sizeof(stringArray) > arraySize) {
    for (int i = 1; i < arraySize; i++) {
      stringArray[i - 1] = stringArray[i];
    }
  }

  // Добавляем новую строку в конец массива
  stringArray[arraySize - 1] = newString;

  //Выводим инофрмацию о загрузке на экран
  Serial.println("Current array:");
  for (int i = 0; i < arraySize; i++) {
    Serial.println(stringArray[i]);
    oled.println(stringArray[i]);
  }
  Serial.println("End array\n");
}