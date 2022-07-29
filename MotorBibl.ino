#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

#define ButtomPin 2
RF24 radio(9,10);
const byte drvPins[] = {3, 4, 5, 6};
byte address[][6] = {"1Node","2Node","3Node","4Node","5Node","6Node"};

int kolv_st, kolv_ob, timing;
unsigned long BackMillis ;
bool WorkMotor, OutWorkMotor, BackToTheFuture, Block, OneBlock;
static byte tmpSTEPS;

void setup() {
  kolv_st = 300;
  //kolv_ob = 2;
  Serial.begin(9600);
  for (byte i = 0; i < 4; i++) pinMode(drvPins[i], OUTPUT);
  pinMode(ButtomPin, INPUT_PULLUP);

  radio.begin(); 
  radio.setAutoAck(1);         
  radio.setRetries(0,15);    
  radio.enableAckPayload();    
  radio.setPayloadSize(8);   
  radio.openReadingPipe(1,address[0]);
  radio.setChannel(0x60); 
  radio.setPALevel (RF24_PA_MAX);
  radio.setDataRate (RF24_1MBPS); 
  radio.powerUp(); 
  radio.startListening();
}

void loop() {
  byte pipeNo, gotByte;                          
  if( radio.available(&pipeNo)){  
    radio.read( &gotByte, sizeof(gotByte) ); 
    if(gotByte == 77 && Block==0) OutWorkMotor = 1; 
    if(gotByte == 55) radio.writeAckPayload(pipeNo,&BackToTheFuture, sizeof(BackToTheFuture) );
    Serial.print("Получено: "); Serial.println(gotByte);
   }
  
  if ( digitalRead(ButtomPin)==0){
    if(millis() - BackMillis >= 1){
      timing++;
      WorkMotor = 1;
      BackMillis=millis();
      if(timing > 10000) SetupMotor();
      else if(timing > 2000){
        if(OneBlock ==0){
          Block=!Block; 
          OneBlock = 1;
          Serial.print("Блокировка "); Serial.println(Block);}
        WorkMotor = 0;
        }
      }
  }
  else{
    if (WorkMotor == 1 || OutWorkMotor == 1)
    SetMotor(kolv_st, kolv_ob);
    WorkMotor = 0;
    OutWorkMotor = 0;
    OneBlock= 0;
    timing=0;}

}

void SetupMotor() { 
   int tmp_kolv_st;
   tmpSTEPS=0;
   BackToTheFuture = 1;
   
   while (digitalRead(ButtomPin) == 0){
    RunMotor(tmpSTEPS++); 
    tmp_kolv_st++;
    }
   for (int i = 0; i < 4; i++) digitalWrite(drvPins[i], 0);
   
   kolv_st = tmp_kolv_st;
   kolv_ob = 0;
   Block=!Block; 
   WorkMotor = 0;
}

void SetMotor(int STEPS, int oboroti){  
   if (STEPS > 200){
     oboroti = (STEPS / 200)+ kolv_ob; 
     STEPS = STEPS % 200;
     kolv_st = STEPS;
     kolv_ob = oboroti; 
    }
    radio.stopListening();
    BackToTheFuture= !BackToTheFuture;
   while(oboroti > 0){
    for (int i = 0; i < 200; i++) RunMotor(tmpSTEPS++);
    oboroti--;
    }
   for (int i = 0; i < STEPS; i++) RunMotor(tmpSTEPS++);
   
   for (int i = 0; i < 4; i++) digitalWrite(drvPins[i], 0);
   radio.startListening();
}

void RunMotor(byte thisStep) {  
  static const byte steps[] = {0b1010, 0b0110, 0b0101, 0b1001};
  static const byte BackSteps[] = {0b1001, 0b0101, 0b0110, 0b1010};
  for (byte i = 0; i < 4; i++){
  if(BackToTheFuture == 1)
  digitalWrite(drvPins[i], bitRead(steps[thisStep & 0b0011], i));
  else
  digitalWrite(drvPins[i], bitRead(BackSteps[thisStep & 0b0011], i));}
  delayMicroseconds(3000);
  /*
  10000 30об/мин
  7500 40об/мин
  6000 50об/мин
  5000 60об/мин
  4000 75об/мин
  3000 100об/мин
  2500 120об/мин
  2400 125об/мин
  2000 150об/мин
  1500 200об/мин
  1250 240об/мин
  */
}

