#include <GPRS_Shield_Arduino.h>
#include <sim900.h>
#include <SoftwareSerial.h>

SoftwareSerial SmsSerial(10, 11);
GPRS gprs(SmsSerial);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  SmsSerial.begin(9600);
  gprs.powerOn();
  while (!gprs.init()) {
    // если связи нет, ждём 1 секунду
    // и выводим сообщение об ошибке
    // процесс повторяется в цикле
    // пока не появится ответ от GPRS устройства
    delay(1000);
    Serial.print("Init error\r\n");
  }
  Serial.println("GPRS init success");
  
}

void loop() {
  // put your main code here, to run repeatedly:

}
