#include "Global.h"
#include "web.h"
#include "fram.h"
#include "config.h"

String getMenu() {
    String menu = "<div style='margin-bottom: 20px;'>";
    menu += "<a href='/'>Главная</a> | ";
    menu += "<a href='/weight'>Датчик веса</a> | ";
    menu += "<a href='/update'>Обновление</a> | ";
    menu += "<a href='/clear'>Очистить EEPROM</a>";
    menu += "</div>";
    return menu;
  }
  
  void initWebServer() {
    // Веб-интерфейс для настройки
    server.on("/", HTTP_GET, []() {
      String html = "<html><head><meta charset='UTF-8'></head><body>";
      html += getMenu();
      html += "<h1>Настройка Wi-Fi и точки доступа</h1>";
      html += "<form action='/save' method='POST'>";
      html += "Wi-Fi SSID: <input type='text' name='wifi_ssid' value='" + String(wifi_ssid) + "'><br>";
      html += "Wi-Fi Password: <input type='text' name='wifi_password' value='" + String(wifi_password) + "'><br>";
      html += "AP SSID: <input type='text' name='ap_ssid' value='" + String(ap_ssid) + "'><br>";
      html += "AP Password: <input type='text' name='ap_password' value='" + String(ap_password) + "'><br>";
      html += "Часовой пояс: <input type='text' name='timezone' value='" + timezone + "'><br>";
      html += "<input type='submit' value='Сохранить'>";
      html += "</form>";
      html += "<p>IP адрес точки доступа: " + WiFi.softAPIP().toString() + "</p>";
      html += "<p>Версия прошивки: " + String(currentFirmwareVersion) + "</p>";
      //html += "<p>Дата последнего обновления: " + rtc.now().timestamp() + "</p>";
      html += "</body></html>";
      server.send(200, "text/html", html);
    });
  
    server.on("/save", HTTP_POST, []() {
      wifi_ssid = server.arg("wifi_ssid");
      wifi_password = server.arg("wifi_password");
      ap_ssid = server.arg("ap_ssid");
      ap_password = server.arg("ap_password");
      calibration_factor = server.arg("calibration_factor").toFloat();
      timezone = server.arg("timezone");
      saveConfig();
      String html = "<html><head><meta charset='UTF-8'></head><body>";
      html += "<p>Конфигурация сохранена. Перезагрузите устройство.</p>";
      html += "</body></html>";
      server.send(200, "text/plain", html);
    });
  
    // Веб-интерфейс для работы с датчиком веса
    server.on("/weight", HTTP_GET, []() {
      String html = "<html><head><meta charset='UTF-8'></head><body>";
      html += getMenu();
      html += "<h1>Датчик веса</h1>";
      if (scale.is_ready()) {
          yield();
          html += "<p>Текущий вес: " + String(scale.get_units(5)) + " кг</p>";
      } else {
          html += "<p>Датчик веса не готов или не подключен</p>";
      }
      html += "<p>Текущая температура: " + String(bmp.readTemperature()) + " °C</p>";
      html += "<p>Текущее давление: " + String(bmp.readPressure() / 100.0F) + " гПа</p>";
      html += "<p>Текущее время: " + rtc.now().timestamp() + "</p>";
      html += "<p>Версия прошивки: " + String(currentFirmwareVersion) + "</p>";
      html += "<p>Дата последнего обновления: " + rtc.now().timestamp() + "</p>";
      html += "<br><br>";
      html += "<p>Прежде чем откалибровать весы, необходимо убрать<p>";
      html += "<p>с платформы всё, что на ней установлено и выполнить<p>";
      html += "<p>тарировку. После этого установить на платворму известный<p>";
      html += "<p>груз, вписать его значение в поле и нажать на кнопку.<p>";
      html += "<form action='/calibrate' method='POST'>";
      html += "Известный вес: <input type='text' name='load' value='" + String(load) + "'><br>";
      html += "Калибровочный коэффициент: <input type='text' name='calibration_factor'><br>";
      html += "<input type='submit' value='Калибровать'>";
      html += "</form>";
      html += "<form action='/tare' method='POST'>";
      html += "<input type='submit' value='Тарировать'>";
      html += "</form>";
      html += "<h2>История измерений</h2>";
      html += "<ul>";
  
      // Чтение заголовка
      Wire.beginTransmission(EEPROM_I2C_ADDRESS);
      Wire.write(0); // Адрес заголовка
      Wire.endTransmission();
      Wire.requestFrom(EEPROM_I2C_ADDRESS, sizeof(EEPROMHeader));
      if (Wire.available() == sizeof(EEPROMHeader)) {
        Wire.readBytes((byte*)&eepromHeader, sizeof(EEPROMHeader));
      }
  
      // Чтение данных из EEPROM
      for (int i = 0; i < eepromHeader.recordCount; i++) {
        int address = sizeof(EEPROMHeader) + i * sizeof(Measurement);
        Wire.beginTransmission(EEPROM_I2C_ADDRESS);
        Wire.write((int)(address >> 8));   // Старший байт адреса
        Wire.write((int)(address & 0xFF)); // Младший байт адреса
        Wire.endTransmission();
  
        Wire.requestFrom(EEPROM_I2C_ADDRESS, sizeof(Measurement));
        if (Wire.available() == sizeof(Measurement)) {
          Measurement measurement;
          Wire.readBytes((byte*)&measurement, sizeof(Measurement));
          html += "<li>" + measurement.timestamp.timestamp() + ": " + String(measurement.weight) + " кг, " + String(measurement.temperature) + " °C, " + String(measurement.pressure) + " гПа</li>";
        }
      }
  
      html += "</ul>";
      html += "</body></html>";
      server.send(200, "text/html", html);
    });
  
    server.on("/calibrate", HTTP_POST, []() {
      if (scale.is_ready()) {
          yield();
          calibration_factor = server.arg("calibration_factor").toFloat();
          load = server.arg("load").toFloat();
          //scale.set_scale(calibration_factor);
          calibration_factor = scale.get_units(10) / (load / units);
          saveConfig(); // Сохранение коэффициента
          server.send(200, "text/plain", "Калибровка выполнена.");
      } else {
          server.send(200, "text/plain", "No load cell found");
      }
    });
  
    server.on("/tare", HTTP_POST, []() {
      String html = "<html><head><meta charset='UTF-8'></head><body>";
      if (scale.is_ready()) {
          scale.tare();
      }
      server.send(200, "text/plain", "Тарирование выполнено.");
    });
  
    // Веб-интерфейс для OTA-обновлений
    server.on("/update", HTTP_GET, []() {
      String html = "<html><head><meta charset='UTF-8'></head><body>";
      html += getMenu();
      html += "<h1>Обновление прошивки</h1>";
      html += "<form action='/update' method='POST' enctype='multipart/form-data'>";
      html += "<input type='file' name='update'>";
      html += "<input type='submit' value='Обновить'>";
      html += "</form>";
      html += "</body></html>";
      server.send(200, "text/html", html);
    });
  
    server.on("/update", HTTP_POST, []() {
      //String html = "<html><head><meta charset='UTF-8'></head><body>";
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "Ошибка обновления" : "Обновление завершено");
      ESP.restart();
    }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        WiFiUDP::stopAll();
        Serial.printf("Обновление: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace)) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
          Serial.printf("Обновление успешно: %u байт\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
    });
  
    // Веб-интерфейс для очистки EEPROM
    server.on("/clear", HTTP_GET, []() {
      String html = "<html><head><meta charset='UTF-8'></head><body>";
      clearEEPROM();
      server.send(200, "text/plain", "EEPROM очищена.");
    });
  
    server.begin();
    Serial.println("Веб-сервер запущен");
  }

  void checkForUpdates() {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(wifiClient, firmwareVersionUrl);
      int httpCode = http.GET();
  
      if (httpCode == HTTP_CODE_OK) {
        String newVersion = http.getString();
        if (newVersion != currentFirmwareVersion) {
          Serial.println("Доступна новая версия прошивки: " + newVersion);
          // Здесь можно добавить уведомление пользователя через веб-интерфейс
        }
      }
      http.end();
    }
  }