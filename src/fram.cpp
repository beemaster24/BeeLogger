#include "Global.h"
#include "fram.h"

void saveMeasurementToEEPROM(const Measurement& measurement) {
    // Чтение заголовка
    Wire.beginTransmission(EEPROM_I2C_ADDRESS);
    Wire.write(0); // Адрес заголовка
    Wire.endTransmission();
    Wire.requestFrom(EEPROM_I2C_ADDRESS, sizeof(EEPROMHeader));
    if (Wire.available() == sizeof(EEPROMHeader)) {
      Wire.readBytes((byte*)&eepromHeader, sizeof(EEPROMHeader));
    }
  
    // Поиск свободного места в EEPROM
    int address = sizeof(EEPROMHeader) + eepromHeader.recordCount * sizeof(Measurement);
    if (address + sizeof(Measurement) >= 8192) {
      // Если EEPROM заполнена, удаляем самое старое измерение
      address = sizeof(EEPROMHeader);
      eepromHeader.recordCount = 0;
    }
  
    // Запись данных в EEPROM
    Wire.beginTransmission(EEPROM_I2C_ADDRESS);
    Wire.write((int)(address >> 8));   // Старший байт адреса
    Wire.write((int)(address & 0xFF)); // Младший байт адреса
    Wire.write((byte*)&measurement, sizeof(measurement));
    Wire.endTransmission();
  
    // Обновление заголовка
    eepromHeader.recordCount++;
    Wire.beginTransmission(EEPROM_I2C_ADDRESS);
    Wire.write(0); // Адрес заголовка
    Wire.write((byte*)&eepromHeader, sizeof(EEPROMHeader));
    Wire.endTransmission();
  }
  
  void clearEEPROM() {
    // Очистка заголовка
    eepromHeader.recordCount = 0;
    Wire.beginTransmission(EEPROM_I2C_ADDRESS);
    Wire.write(0); // Адрес заголовка
    Wire.write((byte*)&eepromHeader, sizeof(EEPROMHeader));
    Wire.endTransmission();
  
    // Очистка данных
    for (int address = sizeof(EEPROMHeader); address < 8192; address++) {
      Wire.beginTransmission(EEPROM_I2C_ADDRESS);
      Wire.write((int)(address >> 8));   // Старший байт адреса
      Wire.write((int)(address & 0xFF)); // Младший байт адреса
      Wire.write(0); // Запись нуля
      Wire.endTransmission();
    }
  
    Serial.println("EEPROM очищена.");
  }