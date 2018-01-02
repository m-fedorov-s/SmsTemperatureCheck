#include <Wire.h>
#include <TroykaRTC.h>
#include <OneWire.h>
#include <GPRS_Shield_Arduino.h>
#include <sim900.h>
#include <SoftwareSerial.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define _LOG
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
char part[12];

const int history_size = 48;
float TempData[history_size];
bool ElecData[history_size];

int previous_index = -1;

enum TMode {
  Mode_Normal,
  Mode_OutOfElectricity
};

TMode currentMode = Mode_Normal;

void setup() {
  for (int i = 0; i < history_size; i++){
    TempData[i] = 0.0;
    ElecData[i] = false;
  }
  previous_index = -1;
  currentMode = Mode_Normal;
  
  pinMode(8, INPUT);
  // инициализация часов
  clock.begin();

  Serial.begin(9600); //!!!
  while (!Serial) {    
  }
  
  SmsSerial.begin(9600);

////////////////
  Serial.println("starting...");
  
  gprs.powerOn();
  while (!gprs.init()) {
    // если связи нет, ждём 1 секунду
    // и выводим сообщение об ошибке
    // процесс повторяется в цикле
    // пока не появится ответ от GPRS устройства
    delay(1000);
    Serial.println("can't init gprs...");
  }
  Serial.print("ready to send sms...");
  gprs.sendSMS(PhoneNumb, "device is on!!!");
  Serial.println("sms sent!");
/////////////////

  SendSms("Device is on");
}

//------------------------------------------------------------------------------------------

bool ElectroOn() {
  return digitalRead(8) == 1;
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

void power_on_gprs()
{
  gprs.powerOn();
  while (!gprs.init()) {
    // если связи нет, ждём 1 секунду
    // и выводим сообщение об ошибке
    // процесс повторяется в цикле
    // пока не появится ответ от GPRS устройства
    delay(1000);
  }    
}

void power_off_gprs()
{
  gprs.powerOff();
  while ( gprs.checkPowerUp() ) {
    gprs.powerOff();
    delay(1000);
  }
}

void SendSms(char* text)
{
  power_on_gprs();
  SendSms_if_power_on(text);
  power_off_gprs();
}

//  отправляет смс, не включая и не выключая gprs модуль
void SendSms_if_power_on(char* text)
{
  //assert( gprs.checkPowerUp() );
  gprs.sendSMS(PhoneNumb, text);
  delay(10000);
}

void Log(char* text)
{
#ifdef _LOG
  Serial.println(text);
#endif
}

//  записывает одну строку отчета в dest (буфер памяти должен быть достаточно большого размера,
//  чтобы в него поместилась одна строка). Перевод строки не добавляется
//  Возвращает указатель на конец строки
char* report_line(bool electroOn, int hours, int minutes, float temperature, char* dest) 
{
  char* dest_position = dest;
  *dest_position++ = electroOn ? ' ' : 'x';
  *dest_position++ = ' ';

  //  формируем строку с температурой, т.к. %f не работает в ардуино
  char temperature_buffer[7];
  dtostrf(temperature, 4, 1, temperature_buffer);
  sprintf(dest_position, "%02d:%02d  %s", hours, minutes, temperature_buffer);

  return dest + strlen(dest);
}

//  добавляет конец строки
char* add_endline(char* dest) 
{
  sprintf(dest, "\n");
  return dest + strlen(dest);
}

//  конвертирует индекс в массиве с историческими данными в часы и минуты
void index_to_time( int index, int& hours, int& minutes )
{
  hours = index / 2;
  minutes = (index % 2) * 30;
}

//  конвертирует время в индекс в массиве
int time_to_index( int hours, int minutes ) 
{
  const int index = hours * 2 + minutes / 30;
  assert(index >= 0 && index < history_size);
  return index;
}

//  приводит индекс в интервал [0, history_size)
int index_to_range(int index)
{
  while (index < 0)
    index += history_size;
  while (index >= history_size)
    index -= history_size;
  return index;
}

//  создает отчет за 12 отсчетов времени с интервалом (step * 30) минут, и отсылает смс
void report_half(int start_index, int step) 
{
  char sms_buffer[200];
  char* dest = sms_buffer;
  for (int i = 0; i < 6; i++ ) {
    const int index = (start_index + i * step) % history_size;
    int hours = 0;
    int minutes = 0;
    index_to_time( index, hours, minutes );
    dest = report_line(ElecData[index], hours, minutes, TempData[index], dest);
    if( i < 6 - 1 ) {
      dest = add_endline(dest);
    }
  }
  SendSms_if_power_on(sms_buffer);
  delay(2000);
}

//  создает два отчета за 24 отсчета времени с интервалом 30 минут, и отсылает 2 смс
void report_30_all(int start_index) 
{
  power_on_gprs();
  report_half(start_index, 1);
  report_half((start_index + 6) % history_size, 1);
  power_off_gprs();
}

//  создает два отчета за 24 отсчета времени с интервалом 60 минут, и отсылает 2 смс
void report_60_all(int start_index) 
{
  power_on_gprs();
  report_half(start_index, 2);
  report_half((start_index + 12) % history_size, 2);
  power_off_gprs();
}

//  добавляет свежие данные измерений. Если данные с таким временем уже были добавлены прямо перед этим,
//  то ничего не добавляется, и возвращает false
bool add_new_data(int hours, int minutes, float temperature, bool is_electricity_on)
{
  const int index = time_to_index(hours, minutes);
  if (index == previous_index)
    return false;
  TempData[index] = temperature;
  ElecData[index] = is_electricity_on;
  
  previous_index = index;
  return true;
}

void loop() {
  //  получаем текущие значения температуры, времени и наличия напряжения
  const float temperature = GetTemper();
  const int minutes = GetMinute();
  const int hours = GetHour();
  const bool is_electricity_on = ElectroOn();

  if (is_electricity_on && currentMode == Mode_OutOfElectricity) {
    currentMode = Mode_Normal;
    SendSms("Electricity is on");
  }

  if (!is_electricity_on && currentMode == Mode_Normal) {
    currentMode = Mode_OutOfElectricity;
    SendSms("Electricity is off!");
  }

  if (minutes % 30 == 0) {
    const bool was_data_added = add_new_data(hours, minutes, temperature, is_electricity_on);
    if (was_data_added) { 
      const int current_index = time_to_index(hours, minutes);
      if (currentMode == Mode_OutOfElectricity && minutes == 0) {
        const int start_index = index_to_range(current_index - 11);
        report_30_all(start_index);
      }
      if (currentMode == Mode_Normal && minutes == 0 && hours % 12 == 9) {
        const int start_index = index_to_range(current_index - 22);
        report_60_all(start_index);
      }
    }
  }
  delay(35000);
  digitalWrite(12, LOW);
}

