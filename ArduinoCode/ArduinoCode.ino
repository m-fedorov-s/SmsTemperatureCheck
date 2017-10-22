#include <OneWire.h>
#include <GPRS_Shield_Arduino.h>
#include <sim900.h>
#include <SoftwareSerial.h>

OneWire ds(7);

SoftwareSerial SmsSerial(10, 11);
GPRS gprs(SmsSerial);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  SmsSerial.begin(9600);
  gprs.powerOn();
//  while (!gprs.init()) {
//    // если связи нет, ждём 1 секунду
//    // и выводим сообщение об ошибке
//    // процесс повторяется в цикле
//    // пока не появится ответ от GPRS устройства
//    delay(1000);
//    Serial.print("Init error\r\n");
//  }
//  Serial.println("GPRS init success");
  
}

float GetTemper() {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;
  if ( !ds.search(addr)) {
  ds.reset_search();
  delay(250);
  return 0.001;
  }
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44); // начинаем преобразование, используя ds.write(0x44,1) с "паразитным" питанием
  delay(1000); // 750 может быть достаточно, а может быть и не хватит
  // мы могли бы использовать тут ds.depower(), но reset позаботится об этом
  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);
  for ( i = 0; i < 9; i++) { // нам необходимо 9 байт
  data[i] = ds.read();
  }
  // конвертируем данный в фактическую температуру
  // так как результат является 16 битным целым, его надо хранить в
  // переменной с типом данных "int16_t", которая всегда равна 16 битам,
  // даже если мы проводим компиляцию на 32-х битном процессоре
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
  raw = raw << 3; // разрешение 9 бит по умолчанию
  if (data[7] == 0x10) {
  raw = (raw & 0xFFF0) + 12 - data[6];
  }
  } else {
  byte cfg = (data[4] & 0x60);
  // при маленьких значениях, малые биты не определены, давайте их обнулим
  if (cfg == 0x00) raw = raw & ~7; // разрешение 9 бит, 93.75 мс
  else if (cfg == 0x20) raw = raw & ~3; // разрешение 10 бит, 187.5 мс
  else if (cfg == 0x40) raw = raw & ~1; // разрешение 11 бит, 375 мс
  //// разрешение по умолчанию равно 12 бит, время преобразования - 750 мс
  }
  celsius = (float)raw / 16.0;
  return celsius;

}

void loop() {
  Serial.println(GetTemper());
  delay(5000);
  float f = GetTemper();
}


