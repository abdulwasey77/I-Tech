#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <SPI.h>
#include <EEPROM.h>

#define Input1 A0
#define Input2 A1
#define Input3 A2
#define Input4 A3
#define Input5 A4
#define Input6 A5
#define Input7 A6
#define Input8 A7

int lcdColumns = 20;
int lcdRows = 4;
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
File myFile;

int address = 0;
int value;

String fileName="";
const int chipSelect = 53;
char charRead;
String dataWrite;
String clockData;

unsigned long lastDB1 = 0, lastDB2 = 0, lastDB3 = 0, lastDB4 = 0, lastDB5 = 0, lastDB6 = 0, lastDB7 = 0, lastDB8 = 0;
unsigned long secClockTimer = millis(), timer1sec = millis(), timer1min = millis(), timer1hour = millis(), timerInline0 = millis(), timerDisplay = millis();
int btn1State = 1, btn2State = 1, btn3State = 1, btn4State = 1, btn5State = 1, btn6State = 1, btn7State = 1, btn8State = 1;
int secCont1, secCont2, secCont3, secCont4, secCont5, secCont6, secCont7, secCont8;
int minCont1, minCont2, minCont3, minCont4, minCont5, minCont6, minCont7, minCont8;
int hourCont1, hourCont2, hourCont3, hourCont4, hourCont5, hourCont6, hourCont7, hourCont8;
int hallSen1 = 0, hallSen2 = 0, hallSen3 = 0, hallSen4 = 0, hallSen5 = 0, hallSen6 = 0, hallSen7 = 0, hallSen8 = 0;
int displayCount, firstPulse = 1;
float rawValue1=0, rawValue2=0, rawValue3=0, rawValue4=0, rawValue5=0, rawValue6=0, rawValue7=0, rawValue8=0;
float zeroLevel = 537;
float pulseVolts1, pulseVolts2, pulseVolts3, pulseVolts4, pulseVolts5, pulseVolts6, pulseVolts7, pulseVolts8;
float pulseVolts1sec, pulseVolts2sec, pulseVolts3sec, pulseVolts4sec, pulseVolts5sec, pulseVolts6sec, pulseVolts7sec, pulseVolts8sec;
float pulseVolts1min, pulseVolts2min, pulseVolts3min, pulseVolts4min, pulseVolts5min, pulseVolts6min, pulseVolts7min, pulseVolts8min;
float pulseVolts1hr, pulseVolts2hr, pulseVolts3hr, pulseVolts4hr, pulseVolts5hr, pulseVolts6hr, pulseVolts7hr, pulseVolts8hr;
char pulseFlag1, pulseFlag2, pulseFlag3, pulseFlag4, pulseFlag5, pulseFlag6, pulseFlag7, pulseFlag8;
String Sensor[8] = {"Sensor1", "Sensor2", "Sensor3", "Sensor4", "Sensor5", "Sensor6", "Sensor7", "Sensor8"};
String lastHallsen, firstHallsen;
int secClock = 0, minClock = 0, hourClock = 0, dayClock = 0, weekClock = 0;
String sdSensor,sdTime,sdData;
float sdmiliVolts1,sdmiliVolts2,sdmiliVolts3,sdmiliVolts4,sdmiliVolts5,sdmiliVolts6,sdmiliVolts7,sdmiliVolts8;
int sdflag1=0,sdflag2=0,sdflag3=0,sdflag4=0,sdflag5=0,sdflag6=0,sdflag7=0,sdflag8=0;

void setup() {
  Serial.begin(115200);
  value = EEPROM.read(address);
  fileName = "DL";
  fileName += String(value);
  fileName += ".txt";
  value++;
  Serial.print("value  "); Serial.println(value);
  Serial.print("filename  "); Serial.println(fileName);
  EEPROM.write(address, value);
  sdbegin();

  pinMode(Input1, INPUT);
  pinMode(Input2, INPUT);
  pinMode(Input3, INPUT);
  pinMode(Input4, INPUT);
  pinMode(Input5, INPUT);
  pinMode(Input6, INPUT);
  pinMode(Input7, INPUT);
  pinMode(Input8, INPUT);
  lcd.init();
  lcd.backlight();
}

void loop() {
  Clock();
  if (millis() - timerInline0 > 5000) {
    timerInline0 = millis();
    if (displayCount < 6) {
      displayCount++;
    } else {
      displayCount = 0;
    }
  }
    readPulse();
    showData();
    storeData();

  if (millis() - timerDisplay > 500) {
    timerDisplay = millis();
    lcd.clear();
    showDisplay();
  }

}

void storeData(){
    if(sdflag1){
    sdflag1=0;
    sdData+=clockData+" :: "+Sensor[0]+" :: "+String(sdmiliVolts1);
    writeToFile();
    sdData="";
  }
    if(sdflag2){
    sdflag2=0;
    sdData+=clockData+" :: "+Sensor[1]+" :: "+String(sdmiliVolts2);
    writeToFile();
    sdData="";
  }
    if(sdflag3){
    sdflag3=0;
    sdData+=clockData+" :: "+Sensor[2]+" :: "+String(sdmiliVolts3);
    writeToFile();
    sdData="";
  }
    if(sdflag4){
    sdflag4=0;
    sdData+=clockData+" :: "+Sensor[3]+" :: "+String(sdmiliVolts4);
    writeToFile();
    sdData="";
  }
    if(sdflag5){
    sdflag5=0;
    sdData+=clockData+" :: "+Sensor[4]+" :: "+String(sdmiliVolts5);
    writeToFile();
    sdData="";
  }
    if(sdflag6){
    sdflag6=0;
    sdData+=clockData+" :: "+Sensor[5]+" :: "+String(sdmiliVolts6);
    writeToFile();
    sdData="";
  }
  if(sdflag7){
    sdflag7=0;
    sdData+=clockData+" :: "+Sensor[6]+" :: "+String(sdmiliVolts7);
    writeToFile();
    sdData="";
  }
   if(sdflag8){
    sdflag8=0;
    sdData+=clockData+" :: "+Sensor[7]+" :: "+String(sdmiliVolts8);
    writeToFile();
    sdData="";
  }
}

void Clock() {
  clockData="";
  if (millis() - secClockTimer > 1000) {
    secClockTimer = millis();
    secClock++;
  }
  if (secClock == 60) {
    secClock = 0;
    minClock++;
  }
  if (minClock == 60) {
    minClock = 0;
    hourClock++;
  }
  if (hourClock == 24) {
    hourClock = 0;
    dayClock++;
  }
  if (dayClock == 30) {
    dayClock = 0;
    weekClock++;
  }
  clockData+=String(weekClock)+':'+String(dayClock)+':'+String(hourClock)+':'+String(minClock)+':'+String(secClock);
}

void readPulse()
{
  for (int i = 0; i <= 10; i++)
  {
    rawValue1 = rawValue1 + analogRead(Input1) - zeroLevel;
    rawValue2 = rawValue2 + analogRead(Input2) - zeroLevel;
    rawValue3 = rawValue3 + analogRead(Input3) - zeroLevel;
    rawValue4 = rawValue4 + analogRead(Input4) - zeroLevel;
    rawValue5 = rawValue5 + analogRead(Input5) - zeroLevel;
    rawValue6 = rawValue6 + analogRead(Input6) - zeroLevel;
    rawValue7 = rawValue7 + analogRead(Input7) - zeroLevel;
    rawValue8 = rawValue8 + analogRead(Input8) - zeroLevel;
delay(1);
  }
  rawValue1 /= 10; rawValue2 /= 10; rawValue3 /= 10; rawValue4 /= 10; rawValue5 /= 10; rawValue6 /= 10; rawValue7 /= 10; rawValue8 /= 10;
  pulseVolts1 = (rawValue1 * 5) / 1023;
  pulseVolts2 = (rawValue2 * 5) / 1023;
  pulseVolts3 = (rawValue3 * 5) / 1023;
  pulseVolts4 = (rawValue4 * 5) / 1023;
  pulseVolts5 = (rawValue5 * 5) / 1023;
  pulseVolts6 = (rawValue6 * 5) / 1023;
  pulseVolts7 = (rawValue7 * 5) / 1023;
  pulseVolts8 = (rawValue8 * 5) / 1023;
  

  if (rawValue1 > 0.05)
  {
    if (pulseFlag1)
    {
      sdflag1=1;
      pulseFlag1 = 0;
      secCont1++;
      pulseVolts1sec=pulseVolts1sec+(pulseVolts1*1000);
      sdmiliVolts1=(pulseVolts1*1000);
      sdTime=clockData;
      lastHallsen = Sensor[0];
      if (firstPulse) {
        firstPulse = 0;
        firstHallsen = Sensor[0];
      }
    }
  }
  if (pulseVolts1 < 0.01)
    pulseFlag1 = 1;

  if (pulseVolts2 > 0.05)
  {
    if (pulseFlag2)
    {
      sdflag2=1;
      pulseFlag2 = 0;
      secCont2++;
      pulseVolts2sec=pulseVolts2sec+(pulseVolts2*1000);
      sdmiliVolts2=(pulseVolts2*1000);
      sdTime=clockData;
      lastHallsen = Sensor[1];
      if (firstPulse) {
        firstPulse = 0;
        firstHallsen = Sensor[1];
      }
    }
  }
  if (pulseVolts2 < 0.01)
    pulseFlag2 = 1;

  if (pulseVolts3 > 0.05)
  {
    if (pulseFlag3)
    {
      sdflag3=1;
      pulseFlag3 = 0;
      secCont3++;
      pulseVolts3sec=pulseVolts3sec+(pulseVolts3*1000);
      sdmiliVolts3=(pulseVolts3*1000);
      sdTime=clockData;
      lastHallsen = Sensor[2];
      if (firstPulse) {
        firstPulse = 0;
        firstHallsen = Sensor[2];
      }
    }
  }
  if (pulseVolts3 < 0.01)
    pulseFlag3 = 1;

  if (pulseVolts4 > 0.05)
  {
    if (pulseFlag4)
    {
      sdflag4=1;
      pulseFlag4 = 0;
      secCont4++;
      pulseVolts4sec=pulseVolts4sec+(pulseVolts4*1000);
      sdmiliVolts4=(pulseVolts4*1000);
      sdTime=clockData;
      lastHallsen = Sensor[3];
      if (firstPulse) {
        firstPulse = 0;
        firstHallsen = Sensor[3];
      }
    }
  }
  if (pulseVolts4 < 0.01)
    pulseFlag4 = 1;

  if (pulseVolts5 > 0.05)
  {
    if (pulseFlag5)
    {
      sdflag5=1;
      pulseFlag5 = 0;
      secCont5++;
      pulseVolts5sec=pulseVolts5sec+(pulseVolts5*1000);
      sdmiliVolts5=(pulseVolts5*1000);
      sdTime=clockData;
      lastHallsen = Sensor[4];
      if (firstPulse) {
        firstPulse = 0;
        firstHallsen = Sensor[4];
      }
    }
  }
  if (pulseVolts5 < 0.01)
    pulseFlag5 = 1;

  if (pulseVolts6 > 0.05)
  {
    if (pulseFlag6)
    {
      sdflag6=1;
      pulseFlag6 = 0;
      secCont6++;
      pulseVolts6sec=pulseVolts6sec+(pulseVolts6*1000);
      sdmiliVolts6=(pulseVolts6*1000);
      sdTime=clockData;
      lastHallsen = Sensor[5];
      if (firstPulse) {
        firstPulse = 0;
        firstHallsen = Sensor[5];
      }
    }
  }
  if (pulseVolts6 < 0.01)
    pulseFlag6 = 1;

  if (pulseVolts7 > 0.05)
  {
    if (pulseFlag7)
    {
      sdflag7=1;
      pulseFlag7 = 0;
      secCont7++;
      pulseVolts7sec=pulseVolts7sec+(pulseVolts7*1000);
      sdmiliVolts7=(pulseVolts7*1000);
      sdTime=clockData;
      lastHallsen = Sensor[6];
      if (firstPulse) {
        firstPulse = 0;
        firstHallsen = Sensor[6];
      }
    }
  }
  if (pulseVolts7 < 0.01)
    pulseFlag7 = 1;

  if (pulseVolts8 > 0.05)
  {
    if (pulseFlag8)
    {
      sdflag8=1;
      pulseFlag8 = 0;
      secCont8++;
      pulseVolts8sec=pulseVolts8sec+(pulseVolts8*1000);
      sdmiliVolts8=(pulseVolts8*1000);
      sdTime=clockData;
      lastHallsen = Sensor[7];
      if (firstPulse) {
        firstPulse = 0;
        firstHallsen = Sensor[7];
      }
    }
  }
  if (pulseVolts8 < 0.01)
    pulseFlag8 = 1;

}

void showDisplay()
{
  if (displayCount == 0)
  {
    lcd.setCursor(0, 0);
    lcd.print("P");
    lcd.setCursor(0, 1);
    lcd.print("/");
    lcd.setCursor(0, 2);
    lcd.print("S");
    lcd.setCursor(1, 0);
    lcd.print("|");
    lcd.setCursor(1, 1);
    lcd.print("|");
    lcd.setCursor(1, 2);
    lcd.print("|");
    lcd.setCursor(1, 3);
    lcd.print("|");

    lcd.setCursor(2, 0);
    lcd.print("1:");
    lcd.setCursor(2, 1);
    lcd.print("2:");
    lcd.setCursor(2, 2);
    lcd.print("3:");
    lcd.setCursor(2, 3);
    lcd.print("4:");
  
    lcd.setCursor(10, 0);
    lcd.print("|");
    lcd.setCursor(10, 1);
    lcd.print("|");
    lcd.setCursor(10, 2);
    lcd.print("|");
    lcd.setCursor(10, 3);
    lcd.print("|");
    
    lcd.setCursor(11, 0);
    lcd.print("5:");
    lcd.setCursor(11, 1);
    lcd.print("6:");
    lcd.setCursor(11, 2);
    lcd.print("7:");
    lcd.setCursor(11, 3);
    lcd.print("8:");

    lcd.setCursor(4, 0);
    lcd.print(secCont1);
    lcd.setCursor(4, 1);
    lcd.print(secCont2);
    lcd.setCursor(4, 2);
    lcd.print(secCont3);
    lcd.setCursor(4, 3);
    lcd.print(secCont4);
    lcd.setCursor(13, 0);
    lcd.print(secCont5);
    lcd.setCursor(13, 1);
    lcd.print(secCont6);
    lcd.setCursor(13, 2);
    lcd.print(secCont7);
    lcd.setCursor(13, 3);
    lcd.print(secCont8);
  }
  else if (displayCount == 1)
  {
    lcd.setCursor(0, 0);
    lcd.print("P");
    lcd.setCursor(0, 1);
    lcd.print("/");
    lcd.setCursor(0, 2);
    lcd.print("M");
    lcd.setCursor(1, 0);
    lcd.print("|");
    lcd.setCursor(1, 1);
    lcd.print("|");
    lcd.setCursor(1, 2);
    lcd.print("|");
    lcd.setCursor(1, 3);
    lcd.print("|");

    lcd.setCursor(2, 0);
    lcd.print("1:");
    lcd.setCursor(2, 1);
    lcd.print("2:");
    lcd.setCursor(2, 2);
    lcd.print("3:");
    lcd.setCursor(2, 3);
    lcd.print("4:");

    lcd.setCursor(10, 0);
    lcd.print("|");
    lcd.setCursor(10, 1);
    lcd.print("|");
    lcd.setCursor(10, 2);
    lcd.print("|");
    lcd.setCursor(10, 3);
    lcd.print("|");
    
    lcd.setCursor(11, 0);
    lcd.print("5:");
    lcd.setCursor(11, 1);
    lcd.print("6:");
    lcd.setCursor(11, 2);
    lcd.print("7:");
    lcd.setCursor(11, 3);
    lcd.print("8:");

    lcd.setCursor(4, 0);
    lcd.print(minCont1);
    lcd.setCursor(4, 1);
    lcd.print(minCont2);
    lcd.setCursor(4, 2);
    lcd.print(minCont3);
    lcd.setCursor(4, 3);
    lcd.print(minCont4);
    lcd.setCursor(13, 0);
    lcd.print(minCont5);
    lcd.setCursor(13, 1);
    lcd.print(minCont6);
    lcd.setCursor(13, 2);
    lcd.print(minCont7);
    lcd.setCursor(13, 3);
    lcd.print(minCont8);
    
  } else if (displayCount == 2) {
    lcd.setCursor(0, 0);
    lcd.print("P");
    lcd.setCursor(0, 1);
    lcd.print("/");
    lcd.setCursor(0, 2);
    lcd.print("H");
    lcd.setCursor(1, 0);
    lcd.print("|");
    lcd.setCursor(1, 1);
    lcd.print("|");
    lcd.setCursor(1, 2);
    lcd.print("|");
    lcd.setCursor(1, 3);
    lcd.print("|");

    lcd.setCursor(2, 0);
    lcd.print("1:");
    lcd.setCursor(2, 1);
    lcd.print("2:");
    lcd.setCursor(2, 2);
    lcd.print("3:");
    lcd.setCursor(2, 3);
    lcd.print("4:");

    lcd.setCursor(10, 0);
    lcd.print("|");
    lcd.setCursor(10, 1);
    lcd.print("|");
    lcd.setCursor(10, 2);
    lcd.print("|");
    lcd.setCursor(10, 3);
    lcd.print("|");
    
    lcd.setCursor(11, 0);
    lcd.print("5:");
    lcd.setCursor(11, 1);
    lcd.print("6:");
    lcd.setCursor(11, 2);
    lcd.print("7:");
    lcd.setCursor(11, 3);
    lcd.print("8:");

    lcd.setCursor(4, 0);
    lcd.print(hourCont1);
    lcd.setCursor(4, 1);
    lcd.print(hourCont2);
    lcd.setCursor(4, 2);
    lcd.print(hourCont3);
    lcd.setCursor(4, 3);
    lcd.print(hourCont4);
    lcd.setCursor(13, 0);
    lcd.print(hourCont5);
    lcd.setCursor(13, 1);
    lcd.print(hourCont6);
    lcd.setCursor(13, 2);
    lcd.print(hourCont7);
    lcd.setCursor(13, 3);
    lcd.print(hourCont8);

  }  else if (displayCount == 3)
  {
    lcd.setCursor(0, 0);
    lcd.print("m");
    lcd.setCursor(0, 1);
    lcd.print("V");
    lcd.setCursor(0, 2);
    lcd.print("/");
    lcd.setCursor(0, 3);
    lcd.print("s");
    lcd.setCursor(1, 0);
    lcd.print("|");
    lcd.setCursor(1, 1);
    lcd.print("|");
    lcd.setCursor(1, 2);
    lcd.print("|");
    lcd.setCursor(1, 3);
    lcd.print("|");

    lcd.setCursor(2, 0);
    lcd.print("1:");
    lcd.setCursor(2, 1);
    lcd.print("2:");
    lcd.setCursor(2, 2);
    lcd.print("3:");
    lcd.setCursor(2, 3);
    lcd.print("4:");

    lcd.setCursor(10, 0);
    lcd.print("|");
    lcd.setCursor(10, 1);
    lcd.print("|");
    lcd.setCursor(10, 2);
    lcd.print("|");
    lcd.setCursor(10, 3);
    lcd.print("|");
    
    lcd.setCursor(11, 0);
    lcd.print("5:");
    lcd.setCursor(11, 1);
    lcd.print("6:");
    lcd.setCursor(11, 2);
    lcd.print("7:");
    lcd.setCursor(11, 3);
    lcd.print("8:");

    lcd.setCursor(4, 0);
    lcd.print(pulseVolts1sec);
    lcd.setCursor(4, 1);
    lcd.print(pulseVolts2sec);
    lcd.setCursor(4, 2);
    lcd.print(pulseVolts3sec);
    lcd.setCursor(4, 3);
    lcd.print(pulseVolts4sec);
    lcd.setCursor(13, 0);
    lcd.print(pulseVolts5sec);
    lcd.setCursor(13, 1);
    lcd.print(pulseVolts6sec);
    lcd.setCursor(13, 2);
    lcd.print(pulseVolts7sec);
    lcd.setCursor(13, 3);
    lcd.print(pulseVolts8sec);

  } 
  else if (displayCount == 4)
  {
    lcd.setCursor(0, 0);
    lcd.print("m");
    lcd.setCursor(0, 1);
    lcd.print("V");
    lcd.setCursor(0, 2);
    lcd.print("/");
    lcd.setCursor(0, 3);
    lcd.print("M");
    lcd.setCursor(1, 0);
    lcd.print("|");
    lcd.setCursor(1, 1);
    lcd.print("|");
    lcd.setCursor(1, 2);
    lcd.print("|");
    lcd.setCursor(1, 3);
    lcd.print("|");

    lcd.setCursor(2, 0);
    lcd.print("1:");
    lcd.setCursor(2, 1);
    lcd.print("2:");
    lcd.setCursor(2, 2);
    lcd.print("3:");
    lcd.setCursor(2, 3);
    lcd.print("4:");

    lcd.setCursor(10, 0);
    lcd.print("|");
    lcd.setCursor(10, 1);
    lcd.print("|");
    lcd.setCursor(10, 2);
    lcd.print("|");
    lcd.setCursor(10, 3);
    lcd.print("|");
    
    lcd.setCursor(11, 0);
    lcd.print("5:");
    lcd.setCursor(11, 1);
    lcd.print("6:");
    lcd.setCursor(11, 2);
    lcd.print("7:");
    lcd.setCursor(11, 3);
    lcd.print("8:");

    lcd.setCursor(4, 0);
    lcd.print(int(pulseVolts1min));
    lcd.setCursor(4, 1);
    lcd.print(int(pulseVolts2min));
    lcd.setCursor(4, 2);
    lcd.print(int(pulseVolts3min));
    lcd.setCursor(4, 3);
    lcd.print(int(pulseVolts4min));
    lcd.setCursor(13, 0);
    lcd.print(int(pulseVolts5min));
    lcd.setCursor(13, 1);
    lcd.print(int(pulseVolts6min));
    lcd.setCursor(13, 2);
    lcd.print(int(pulseVolts7min));
    lcd.setCursor(13, 3);
    lcd.print(int(pulseVolts8min));

  }
  else if (displayCount == 5)
  {
    lcd.setCursor(0, 0);
    lcd.print("m");
    lcd.setCursor(0, 1);
    lcd.print("V");
    lcd.setCursor(0, 2);
    lcd.print("/");
    lcd.setCursor(0, 3);
    lcd.print("H");
    lcd.setCursor(1, 0);
    lcd.print("|");
    lcd.setCursor(1, 1);
    lcd.print("|");
    lcd.setCursor(1, 2);
    lcd.print("|");
    lcd.setCursor(1, 3);
    lcd.print("|");

    lcd.setCursor(2, 0);
    lcd.print("1:");
    lcd.setCursor(2, 1);
    lcd.print("2:");
    lcd.setCursor(2, 2);
    lcd.print("3:");
    lcd.setCursor(2, 3);
    lcd.print("4:");

    lcd.setCursor(10, 0);
    lcd.print("|");
    lcd.setCursor(10, 1);
    lcd.print("|");
    lcd.setCursor(10, 2);
    lcd.print("|");
    lcd.setCursor(10, 3);
    lcd.print("|");
    
    lcd.setCursor(11, 0);
    lcd.print("5:");
    lcd.setCursor(11, 1);
    lcd.print("6:");
    lcd.setCursor(11, 2);
    lcd.print("7:");
    lcd.setCursor(11, 3);
    lcd.print("8:");

    lcd.setCursor(4, 0);
    lcd.print(int(pulseVolts1hr));
    lcd.setCursor(4, 1);
    lcd.print(int(pulseVolts2hr));
    lcd.setCursor(4, 2);
    lcd.print(int(pulseVolts3hr));
    lcd.setCursor(4, 3);
    lcd.print(int(pulseVolts4hr));
    lcd.setCursor(13, 0);
    lcd.print(int(pulseVolts5hr));
    lcd.setCursor(13, 1);
    lcd.print(int(pulseVolts6hr));
    lcd.setCursor(13, 2);
    lcd.print(int(pulseVolts7hr));
    lcd.setCursor(13, 3);
    lcd.print(int(pulseVolts8hr));

  }
  else if (displayCount == 6) {
    
    lcd.setCursor(6, 0);
    lcd.print("W:D:H:M:S");
    lcd.setCursor(6, 1);
    lcd.print(clockData);
   
    lcd.setCursor(0, 2);
    lcd.print("1st Active :");
    lcd.setCursor(13, 2);
    lcd.print(firstHallsen);

    lcd.setCursor(0, 3);
    lcd.print("Lst Active :");
    lcd.setCursor(13, 3);
    lcd.print(lastHallsen);
  }
}

void showData() {
  if (millis() - timer1sec > 1000) {
    timer1sec = millis();
    minCont1 = minCont1 + secCont1;
    minCont2 = minCont2 + secCont2;
    minCont3 = minCont3 + secCont3;
    minCont4 = minCont4 + secCont4;
    minCont5 = minCont5 + secCont5;
    minCont6 = minCont6 + secCont6;
    minCont7 = minCont7 + secCont7;
    minCont8 = minCont8 + secCont8;
    pulseVolts1min=pulseVolts1min+pulseVolts1sec;
    pulseVolts2min=pulseVolts2min+pulseVolts2sec;
    pulseVolts3min=pulseVolts3min+pulseVolts3sec;
    pulseVolts4min=pulseVolts4min+pulseVolts4sec;
    pulseVolts5min=pulseVolts5min+pulseVolts5sec;
    pulseVolts6min=pulseVolts6min+pulseVolts6sec;
    pulseVolts7min=pulseVolts7min+pulseVolts7sec;
    pulseVolts8min=pulseVolts8min+pulseVolts8sec;
    pulseVolts1sec=0;pulseVolts2sec=0;pulseVolts3sec;pulseVolts4sec=0;pulseVolts5sec=0;pulseVolts6sec=0;pulseVolts7sec=0;pulseVolts8sec=0;
    secCont1 = 0; secCont2 = 0; secCont3 = 0; secCont4 = 0; secCont5 = 0; secCont6 = 0; secCont7 = 0; secCont8 = 0;
  }

  if (millis() - timer1min > 60000) {
    timer1min = millis();
    showDisplay();
    hourCont1 = hourCont1 + minCont1;
    hourCont2 = hourCont2 + minCont2;
    hourCont3 = hourCont3 + minCont3;
    hourCont4 = hourCont4 + minCont4;
    hourCont5 = hourCont5 + minCont5;
    hourCont6 = hourCont6 + minCont6;
    hourCont7 = hourCont7 + minCont7;
    hourCont8 = hourCont8 + minCont8;
    pulseVolts1hr=pulseVolts1hr+pulseVolts1min;
    pulseVolts2hr=pulseVolts2hr+pulseVolts2min;
    pulseVolts3hr=pulseVolts3hr+pulseVolts3min;
    pulseVolts4hr=pulseVolts4hr+pulseVolts4min;
    pulseVolts5hr=pulseVolts5hr+pulseVolts5min;
    pulseVolts6hr=pulseVolts6hr+pulseVolts6min;
    pulseVolts7hr=pulseVolts7hr+pulseVolts7min;
    pulseVolts8hr=pulseVolts8hr+pulseVolts8min;
    pulseVolts1min=0;pulseVolts2min=0;pulseVolts3min=0;pulseVolts4min=0;pulseVolts5min=0;pulseVolts6min=0;pulseVolts7min=0;pulseVolts8min=0;
    minCont1 = 0; minCont2 = 0; minCont3 = 0; minCont4 = 0; minCont5 = 0; minCont6 = 0; minCont7 = 0; minCont8 = 0;

  }
  if (millis() - timer1hour > 3600000) {
    timer1hour = millis();
    showDisplay();
    pulseVolts1hr=0;pulseVolts2hr=0;pulseVolts3hr=0;pulseVolts4hr=0;pulseVolts5hr=0;pulseVolts6hr=0;pulseVolts7hr=0;pulseVolts8hr=0;
    hourCont1 = 0; hourCont2 = 0; hourCont3 = 0; hourCont4 = 0; hourCont5 = 0; hourCont6 = 0; hourCont7 = 0; hourCont8 = 0;
  }

}

void readFromFile()
{
  byte i = 0; //counter
  char inputString[100]; //string to hold read string

  //now read it back and show on Serial monitor
  // Check to see if the file exists:
  if (!SD.exists(fileName)){
    Serial.print(fileName); Serial.println(" doesn't exist.");
  }else{
  Serial.print("Reading from "); Serial.println(fileName);
  }
  myFile = SD.open(fileName);
  //--------------------------------
  while (myFile.available())
  {
    char inputChar = myFile.read(); // Gets one byte from serial buffer
    if (inputChar == '\n') //end of line (or 10)
    {
      inputString[i] = 0;  //terminate the string correctly
      Serial.println(inputString);
      i = 0;
    }
    else
    {
      inputString[i] = inputChar; // Store it
      i++; // Increment where to write next
      if (i > sizeof(inputString))
      {
        Serial.println("Incoming string longer than array allows");
        Serial.println(sizeof(inputString));
        while (1);
      }
    }
  }
}

void writeToFile()
{
  myFile = SD.open(fileName, FILE_WRITE);
  if (myFile) // it opened OK
  {
    Serial.print("Writing to ");
    Serial.print(fileName);
    myFile.println(sdData);
    myFile.close();
    Serial.println("Done");
  }
  else
    Serial.print("Error opening ");
    Serial.println(fileName);
}

void combineData()
{
  dataWrite += '$' + pulseVolts1 + ',' + pulseVolts2 + ',' + pulseVolts3 + ',' + pulseVolts4 + ',' + pulseVolts5 + ',' + pulseVolts6 + ',' + pulseVolts7 + ',' + pulseVolts8 + '#';
}

void ReadRTC() {
  //   DateTime now = rtc.now();
  //   Serial.print(now.hour(), DEC);
  //   Serial.print(":");
  //   Serial.print(now.minute(), DEC);
  //   Serial.print(":");
  //   Serial.print(now.second(), DEC);
  //   Serial.print("  ");
  //   Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  //   Serial.print("  ");
  //   Serial.print(now.day(), DEC);
  //   Serial.print("/");
  //   Serial.print(now.month(), DEC);
  //   Serial.print("/");
  //   Serial.println(now.year(), DEC);
}

void RTCbegin() {
    //  if (! rtc.begin())
    //  {
    //    Serial.println("Couldn't find RTC");
    //    while (1);
    //  }
    //  rtc.adjust(DateTime(__DATE__, __TIME__));
}

void sdbegin() {
     if (SD.begin(chipSelect))
      {
      Serial.println("SD card is present & ready");
      }
      else
      {
      Serial.println("SD card missing or failure");
      while(1);  //wait here forever
      }
}
