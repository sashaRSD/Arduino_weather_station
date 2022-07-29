#include <Wire.h>
#include <SPI.h>
#include "RTClib.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_BME280.h"
#include "nRF24L01.h"
#include "RF24.h"
#include <BlynkSimpleEsp8266.h>

#define SaveTime1_h 7
#define SaveTime1_m 0
#define SaveTime2_h 23
#define SaveTime2_m 0
#define SaveTimeN_1 23
#define SaveTimeN_2 8
#define SaveMinOs 200
#define SaveMaxOs 800


WidgetTerminal terminal(V7);
RF24 radio(2,15);
RTC_DS3231 rtc;
Adafruit_BME280 bme;

byte addresses[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};
char auth[] = "1Lwa5WyKKQQKZPFEqBFWgMLXX7fFbXOU";
char ssid[] = "Sasha20!8";
char pass[] = "password328";

int sun_sr;
int sun_max=800; int sun_min=200;
int time1_hour=7; int time1_min=0;
int time2_hour=23; int time2_min=0;
int NotTime1=00; int NotTime2=7;
byte WorkMotor = 77;
byte SetBackToTheFuture = 55;
bool BackToTheFuture, OneButtom, BlockNight;

void setup() {
  Serial.begin(9600);
  Serial.println();
  Blynk.begin(auth, ssid, pass);
  bme.begin(); 
  rtc.begin();
  rtc.adjust(DateTime(__DATE__, __TIME__));
  rtc.adjust(DateTime(rtc.now().unixtime()+100));

  radio.begin();
  radio.setAutoAck(1);                  
  radio.enableAckPayload();          
  radio.setRetries(0,15);              
  radio.setPayloadSize(8);    
  radio.openWritingPipe(addresses[0]);                 
  radio.setChannel(0x60);
  radio.setPALevel (RF24_PA_MAX);
  radio.setDataRate (RF24_1MBPS);   
  radio.stopListening();     
  radio.powerUp();

  WriteMotor(SetBackToTheFuture);
  Blynk.virtualWrite(V0, !BackToTheFuture);
  if (BackToTheFuture == 0) Blynk.virtualWrite(V5, "Подняты(UP)");
  else Blynk.virtualWrite(V5, "Опущены(DOWN)");

  PrintTerminalBlynk();

}

BLYNK_WRITE(V0){
  if(OneButtom == 0) OneButtom=1;
  WriteMotor(SetBackToTheFuture);
  WriteMotor(WorkMotor);
}

BLYNK_WRITE(V7)
{
  String terminalString = param.asStr();
    String a, b, c, c2, c3, c11, c12, c13;
    a = terminalString.substring(0, 1);
    switch (a.toInt()) {
      case 1:
        b = terminalString.substring(2, 4);
        c = terminalString.substring(5, 7);
        if( (constrain(b.toInt(), 0, 23) == b.toInt()) && (constrain(c.toInt(), 0, 59) == c.toInt()) ){
        time1_hour = b.toInt(); time1_min = c.toInt();}
        else terminal.print(" Неправильное значение!");
        break;
      case 2:
        b = terminalString.substring(2, 4);
        c = terminalString.substring(5, 7);
        if( (constrain(b.toInt(), 0, 23) == b.toInt()) && (constrain(c.toInt(), 0, 59) == c.toInt()) ){
        time2_hour = b.toInt(); time2_min = c.toInt();}
        else terminal.print(" Неправильное значение!");
        break;
      case 3:
        b = terminalString.substring(2, 4);
        c = terminalString.substring(5, 7);
        if( (constrain(b.toInt(), 0, 23) == b.toInt()) && (constrain(c.toInt(), 0, 23) == c.toInt()) ){
        NotTime1 = b.toInt(); NotTime2 = c.toInt();}
        else terminal.print(" Неправильное значение!");
        break;
      case 4:
        b = terminalString.substring(2, 5);
        c = terminalString.substring(6, 9);
        if( (constrain(b.toInt(), 100, 999) == b.toInt()) && (constrain(c.toInt(), 100, 999) == c.toInt()) && (b.toInt()<c.toInt())){
        sun_min = b.toInt(); sun_max = c.toInt();}
        else terminal.print(" Неправильное значение!");
        break;
      case 5:
        c = terminalString.substring(2, 4); c2 = terminalString.substring(5, 7); c3 = terminalString.substring(8, 12);
        c11 = terminalString.substring(14, 16); c12 = terminalString.substring(17, 19); c13 = terminalString.substring(20, 23);
        if( (constrain(c.toInt(), 0, 31) == c.toInt()) && (constrain(c2.toInt(), 0, 12) == c2.toInt()) && (constrain(c3.toInt(), 2020, 2099) == c3.toInt())){        
        if( (constrain(c11.toInt(), 0, 23) == c11.toInt()) && (constrain(c12.toInt(), 0, 59) == c12.toInt()) && (constrain(c13.toInt(), 0, 59) == c13.toInt()) )
        rtc.adjust(DateTime(c3.toInt(), c2.toInt(), c.toInt(), c11.toInt(), c12.toInt(), c13.toInt()));}
        else terminal.print(" Неправильное значение!");
        break;
      case 7:
        b = terminalString.substring(2, 3);
        if(b.toInt()==1){
        rtc.adjust(DateTime(__DATE__, __TIME__));
        time1_hour = SaveTime1_h; time1_min = SaveTime1_m;
        time2_hour = SaveTime2_h; time2_min = SaveTime2_m;
        NotTime1 = SaveTimeN_1; NotTime2 = SaveTimeN_2;
        sun_min = SaveMinOs; sun_max = SaveMaxOs;}
        break;
      default:
        terminal.println("Неправильная команда!");
    }
    terminal.flush();
  PrintTerminalBlynk();
}

void loop() {
  Blynk.run();
  SetSun();
  SetMotor(); 
  PrintValBlynk();
  PrintValSerial();
  Serial.println();
  delay(3000);
}

void SetSun(){
  int sun = analogRead(A0);
  sun_sr = sun_sr + (sun-sun_sr)*0.5;
}

void SetMotor(){
  DateTime now = rtc.now();
  if(now.hour()==NotTime1) BlockNight =1;
  if(now.hour()==NotTime2) BlockNight =0;
  if( (sun_sr > sun_max || sun_sr < sun_min || (now.hour()==time2_hour && now.minute()==time2_min) ||(now.hour()==time1_hour && now.minute()==time2_min)) && BlockNight ==0 ){
  WriteMotor(SetBackToTheFuture);
  if((sun_sr > sun_max && BackToTheFuture ==0) || (sun_sr < sun_min && BackToTheFuture ==1)) WriteMotor(WorkMotor);
  if((now.hour()==time2_hour && now.minute()==time2_min && BackToTheFuture ==0) || (now.hour()==time1_hour && now.minute()==time2_min && BackToTheFuture ==1)) WriteMotor(WorkMotor);}
}

void WriteMotor(byte data){                             
    byte gotByte;                                              
    Serial.print("Отправка: "); Serial.println(data);                                                        
    if ( radio.write(&data, sizeof(data)) ){
      if(data == SetBackToTheFuture){                         
        while(radio.available() ){
        radio.read( &BackToTheFuture, sizeof(BackToTheFuture) );    
        Serial.print("Получил ответ: "); Serial.println(BackToTheFuture);                             
        } 
      }             
    }else       
    Serial.println("Ошибка.");  
}

void PrintValBlynk() {
  if (OneButtom == 1){
    if (BackToTheFuture == 1) Blynk.virtualWrite(V5, "Подняты(UP)");
    else Blynk.virtualWrite(V5, "Опущены(DOWN)");
    Blynk.virtualWrite(V0, BackToTheFuture);}
  Blynk.virtualWrite(V1, sun_sr);
  Blynk.virtualWrite(V2,bme.readHumidity());
  Blynk.virtualWrite(V3,bme.readTemperature());
  Blynk.virtualWrite(V4,bme.readPressure() / 133.32);
}

void PrintTerminalBlynk() {
  delay(3000);
  terminal.clear();
  terminal.println("Время автоматического подъема(UP):"); terminal.flush();
  terminal.println("Синтаксис:1-ХХ-XX");
  terminal.println("Значения: часы(00-23),минуты(00-59)"); 
  terminal.print("Сейчас ");
  if(time1_hour <10) terminal.print("0"); terminal.print(time1_hour); 
  terminal.print(":");
  if(time1_min <10) terminal.print("0"); terminal.println(time1_min);
  terminal.println();terminal.flush();
  
  terminal.println("Время автоматического опускания(DOWN):"); terminal.flush();
  terminal.println("Синтаксис:2-ХХ-XX");
  terminal.println("Значения: часы(00-23),минуты(00-59)");;
  terminal.print("Сейчас ");
  if(time2_hour <10) terminal.print("0"); terminal.print(time2_hour); 
  terminal.print(":"); 
  if(time2_min <10) terminal.print("0"); terminal.println(time2_min);
  terminal.println();terminal.flush();

  terminal.println("Ночное бездействие:"); terminal.flush();
  terminal.println("Синтаксис:3-XX-XX"); terminal.flush();
  terminal.println("Значения: 00-23, начало-конец");
  terminal.print("Сейчас: ["); terminal.print(NotTime1); terminal.print("-"); terminal.print(NotTime2); terminal.println(")");
  terminal.println();terminal.flush();
  
  terminal.println("Ввод min и max освещения:"); terminal.flush();
  terminal.println("Синтаксис:4-ХХХ-XXX");
  terminal.println("Значения: 100-999, min<max");
  terminal.print("Сейчас: "); terminal.print(sun_min); terminal.print(","); terminal.println(sun_max); 
  terminal.println();
  
  DateTime now = rtc.now(); terminal.flush();
  terminal.println("Редактирование времени:");
  terminal.println("Синтаксис:5-ХХ-XX-XXXX--XX-XX-XX");terminal.flush();
  terminal.println("Значения: "); terminal.flush();
  terminal.println("день(0-31),месяц(0-12),год(2020-2099),"); terminal.flush();
  terminal.println("часы(0-23),минуты(0-59),секунды(0-59)."); terminal.flush();
  terminal.println("При последнем обновлении: "); terminal.print(now.day()); terminal.print("/"); terminal.print(now.month()); terminal.print("/"); terminal.println(now.year());
   if(now.hour() <10) terminal.print("0"); terminal.print(now.hour()); terminal.print("/"); 
    if(now.minute() <10) terminal.print("0");terminal.print(now.minute()); terminal.print("/"); 
     if(now.second() <10) terminal.print("0");terminal.println(now.second());
  terminal.println();


  terminal.println("Обновить:7");
  terminal.println("Сброс настроек:7-1");
  terminal.flush();
}

void PrintValSerial() {
  DateTime now = rtc.now();
  Serial.print(now.day(), DEC); Serial.print('/');  Serial.print(now.month(), DEC); Serial.print('/');  Serial.println(now.year(), DEC);
  if(now.hour() <10) Serial.print("0"); Serial.print(now.hour(), DEC);  Serial.print(':');  
  if(now.minute() <10) Serial.print("0"); Serial.print(now.minute(), DEC);  Serial.print(':');  
   if(now.second() <10) Serial.print("0"); Serial.println(now.second(), DEC);

  Serial.print("Освещенность = "); Serial.print(sun_sr); Serial.println(" (0-1023)");
  Serial.print("Температура = "); Serial.print(bme.readTemperature()); Serial.println(" *C");
  Serial.print("Давление = "); Serial.print(bme.readPressure() / 133.32); Serial.println(" mm.rt.st");
  Serial.print("Влажность = "); Serial.print(bme.readHumidity()); Serial.println("%");
}
