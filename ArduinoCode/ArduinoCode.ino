#include <Wire.h>
#include <TroykaRTC.h>
#include <OneWire.h>
#include <GPRS_Shield_Arduino.h>
#include <sim900.h>
#include <SoftwareSerial.h>
#include <string.h>
#include <stdio.h>
#define PhoneNumb "+79104677537"
OneWire ds(7);

SoftwareSerial SmsSerial(10, 11);
GPRS gprs(SmsSerial);
RTC clock;
// массив для хранения текущего времени
char ttime[12];
// массив для хранения текущей даты
// массив для хранения текущего дня недели
char weekDay[12];
char sendtext1[160];
char sendtext2[160];
char part[24];
char TempForTemp[6];
float TempData[48];
bool ElecData[48];
bool  TempWarningSent = false;

void setup() {
  for (int i = 0; i<48; i++){
    TempData[i] = 0.0;
    ElecData[i] = false;
  }
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(8, INPUT);
  // инициализация часов
  clock.begin();
  SmsSerial.begin(9600);
  gprs.powerOn();
  digitalWrite(13, HIGH);
  while (!gprs.init()) {
    // если связи нет, ждём 1 секунду
    // и выводим сообщение об ошибке
    // процесс повторяется в цикле
    // пока не появится ответ от GPRS устройства
    delay(1000);
  }
  digitalWrite(13, LOW);
  digitalWrite(12, HIGH);
  gprs.sendSMS(PhoneNumb, "Device is on");
}

//------------------------------------------------------------------------------------------

bool ElectroOn() {
  if (digitalRead(8) == 1) return true;
  else return false;
}
int GetMinute() {
   clock.read();
   clock.getTimeStamp(ttime, part, weekDay);
   int minut = (ttime[3] - '0')*10+(ttime[4] - '0');
   return minut;
}
int GetHour() {
   clock.read();
   clock.getTimeStamp(ttime, part, weekDay);
   int hour = (ttime[0] - '0')*10+(ttime[1] - '0');
   return hour;
}
float GetTemper() {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;
  ds.reset_search();
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
//---------------------------------------------------------------------------------------
float tem;
int minut, hour, mode = 1;


void loop() {
  tem = GetTemper();
  ////ОБРАБОТКА ВХОДА/////////////////////////////////////////////////////////////////////////
//  if (gprs.ifSMSNow()) {
//    // читаем его
//    gprs.readSMS(sendtext1, ttime, part);
//    //if (ttime[5] == PhoneNumb[5] && ttime[7] == PhoneNumb[7] && ttime[9] == PhoneNumb[9]){
//      if(sendtext1[0] == 'S' && sendtext1[1] == 'e' && sendtext1[2] == 't' && sendtext1[6] == ':'){
//        hour = (sendtext1[4]-'0')*10 + (sendtext1[5]-'0');
//        minut = (sendtext1[7]-'0')*10 + (sendtext1[8]-'0');
//        clock.read();
//        clock.getTimeStamp(ttime, part, weekDay);
//        clock.set(hour, minut, 0, (part[0]-'0')*10+part[1]-'0', (part[3]-'0')*10+part[4]-'0', (part[6]-'0')*1000+(part[7]-'0')*100+(part[8]-'0')*10+part[9]-'0', weekDay);
//        sprintf(part, "OK now %02d:%02d", hour, minut);
//        gprs.sendSMS(PhoneNumb, part);
//      }
//      if(sendtext1[0] == 'M' && sendtext1[1] == 'o' && sendtext1[2] == 'd' && sendtext1[3] == 'e'){
//        mode = sendtext1[5]-'0';
//        if (mode>1 || mode == 0)
//          mode = 1;
//        sprintf(part, "Mode is %d", mode);
//        gprs.sendSMS(PhoneNumb, part);
//      }
//      if(sendtext1[0] == 'W' && sendtext1[1] == 'h' && sendtext1[2] == 'a' && sendtext1[3] == 't'){
//        dtostrf(tem, 4, 2, TempForTemp);
//        sprintf(sendtext1, "Temperature is : %s!", TempForTemp);
//        gprs.sendSMS(PhoneNumb, sendtext1);
//      }
//    //}
//  }
  //////////////////////////////////////////////////////
  minut = GetMinute();
  hour = GetHour();
  if (ElectroOn()) {
    if (mode == 0) gprs.sendSMS(PhoneNumb, "Electricity is on");
    mode = 1;
  } else {
    if (mode != 0) gprs.sendSMS(PhoneNumb, "Electricity is off!");
    mode = 0;
  }
  if (tem < 1 && !TempWarningSent){
    dtostrf(tem, 4, 2, TempForTemp);
    sprintf(sendtext1, "Temperature is low : %s!", TempForTemp);
    gprs.sendSMS(PhoneNumb, sendtext1);
    TempWarningSent = true;
  }
  if (tem > 0) 
    TempWarningSent = false;
  if  (minut%30 == 0) {
    TempData[hour * 2 + minut/30] = tem;
    ElecData[hour * 2 + minut/30] = ElectroOn();
  }
  //////////////ЭКСТРА_РЕЖИМ./////////////////////////////////////////////////////////////////
  if (mode == 0 && minut == 30) {
    int a, i = 0;
    if (hour * 2 + minut/30 >= 11) a = hour * 2 + minut/30 - 11;
    else a = 48 + hour * 2 + minut/30 - 11;
    sprintf(sendtext1, "");
    for (int j = -11; j< -5; j++){
      if (not ElecData[a]){
        strcpy(sendtext2, sendtext1);
        sprintf (sendtext1, "%s%s", sendtext2, "!");
      }
        else{
        strcpy(sendtext2, sendtext1);
        sprintf (sendtext1, "%s%s", sendtext2, "!");
      }
      dtostrf(TempData[a], 4, 2, TempForTemp);
      sprintf(part, "%02d:%02d  %s\n",a/2, a%2*30, TempForTemp);
      strcpy(sendtext2, sendtext1);
      sprintf(sendtext1, "%s%s", sendtext2, part);
      a += 1;
      if (a > 47) a-=48;
    }
    gprs.sendSMS(PhoneNumb, sendtext1);
    delay(2000);
    sprintf (sendtext2, "");
    for (int j = -5; j<= 0; j++){
      if (not ElecData[a]){
        strcpy(sendtext2, sendtext1);
        sprintf (sendtext2, "%s%s", sendtext1, "!");
      }
        else{
        strcpy(sendtext2, sendtext1);
        sprintf (sendtext2, "%s%s", sendtext1, "!");
      }
      dtostrf(TempData[a], 4, 2, TempForTemp);
      sprintf(part, "%02d:%02d  %s\n",a/2, a%2*30, TempForTemp);
      strcpy(sendtext1, sendtext2);
      sprintf(sendtext2, "%s%s", sendtext1, part);
      a += 1;
      if (a > 47) a-=48;
    }
    gprs.sendSMS(PhoneNumb, sendtext2);
  }
  ////////////СТАНДАРТНЫЙ..///////////////////////////////////////////////////////////////////
  if (mode == 1 && hour%12 == 9 && minut == 0) {
    int a, i = 0;
    if (hour * 2 + minut/30 >= 22) a = hour * 2 + minut/30 - 20;
    else a = 48 + hour * 2 + minut/30 - 22;
    sprintf(sendtext1, "");
    for (int j = -11; j< -5; j++){
      if (not ElecData[a]){
        strcpy(sendtext2, sendtext1);
        sprintf (sendtext1, "%s%s", sendtext2, "!");
      }
        else{
        strcpy(sendtext2, sendtext1);
        sprintf (sendtext1, "%s%s", sendtext2, "!");
      }
      dtostrf(TempData[a], 4, 2, TempForTemp);
      sprintf (part, "%02d:%02d  %s\n",a/2, a%2*30, TempForTemp);
      strcpy(sendtext2, sendtext1);
      sprintf (sendtext1, "%s%s", sendtext2, part);
      a += 2;
      if (a > 47) a-=48;
    }
    gprs.sendSMS(PhoneNumb, sendtext1);
    delay(2000);
    sprintf (sendtext2, "");
    for (int j = -5; j<= 0; j++){
      if (not ElecData[a]){
        strcpy(sendtext2, sendtext1);
        sprintf (sendtext2, "%s%s", sendtext1, "!");
      }
        else{
        strcpy(sendtext2, sendtext1);
        sprintf (sendtext2, "%s%s", sendtext1, "!");
      }
      dtostrf(TempData[a], 4, 2, TempForTemp);
      sprintf (part, "%02d:%02d  %s\n",a/2, a%2*30, TempForTemp);
      strcpy(sendtext1, sendtext2);
      sprintf (sendtext2, "%s%s", sendtext1, part);
      a += 2;
      if (a > 47) a-=48;
    }
    gprs.sendSMS(PhoneNumb, sendtext2);
  }
  ////////////////////////////////////
  delay(40000);
  digitalWrite(12, LOW);
}


