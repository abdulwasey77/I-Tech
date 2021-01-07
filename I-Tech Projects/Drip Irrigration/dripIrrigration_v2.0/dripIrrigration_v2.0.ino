#include "Adafruit_Keypad.h"
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 10
#define PUMPpin 23


OneWire oneWire(ONE_WIRE_BUS);
float Celcius = 0;
float Fahrenheit = 0;
DallasTemperature sensors(&oneWire);

int lcdColumns = 20;
int lcdRows = 4;
const byte ROWS = 4;
const byte COLS = 4;
int pHSense = A0;
int samples = 20;
float adc_resolution = 1023.0;
float analogvol;

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); //3F,27 
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 4 , 3, 2}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Adafruit_Keypad customKeypad = Adafruit_Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);
unsigned long disTime = millis(), sensorData = millis();
String timeArray, orderArray, valveArray;
int min_35=5000;//2100000;
int setTime, setOrder; 
int selTime[12]={min_35,min_35,min_35,min_35,min_35,min_35,min_35,min_35,min_35,min_35,min_35,min_35}; 
int selOrder[12]={0,1,2,3,4,5,6,7,8,9,10,11};
char buf[10], curDisplay[10], selDisplayFlag = 0, noDisplayFlag = 0, button, timePress, timeFlag = 0, addTimeFlag = 0, zeroTime = 0, orderPress, orderFlag = 0, addOrderFlag = 0, zeroOrder = 0;
char valvePress, valveFlag = 0, addValveFlag = 0, zeroValve = 0;
char config1Flag=0,config2Flag=0,config3Flag=0,config4Flag=0,config5Flag=0,config6Flag=0;
int selDisplayCount = 0, disNum = 0;
float voltage;

char  pumpFlag;
unsigned long  startTime,pumpOFF;
int valve[13]={22,24,26,28,30,32,34,36,38,40,42,43};
int c=0;
int Time=5000;
bool pump=LOW,waitFlag=LOW,notset=LOW,set=HIGH; 


void setup() {
  Serial.begin(9600);
  customKeypad.begin();
  for(int i=0;i<12;i++)
  {
    pinMode(valve[i],OUTPUT);
    digitalWrite(valve[i],LOW);
  }
  pinMode(PUMPpin,OUTPUT);
  digitalWrite(PUMPpin,LOW);
  sensors.begin();
  lcd.init();
  lcd.backlight();
}

void loop() {
  keyPress();
  selDisplay();
  conFig();
  if (millis() - disTime > 500) {
    disTime = millis();
    showDisplay();
  }
}

void getPH() {
  int measurings = 0;
  for (int i = 0; i < samples; i++)
  {
    measurings += analogRead(pHSense);
    delay(1);
  }
  analogvol = measurings / samples;
  analogvol = (analogvol * 5) / 1023;
  Serial.print("analog voltage = "); Serial.println(analogvol);
  voltage = 5 / adc_resolution * measurings / samples;
  voltage = 7 + ((2.5 - voltage) / 0.18);
}

void keyPress() {
  customKeypad.tick();
  while (customKeypad.available()) {
    keypadEvent e = customKeypad.read();
    //Serial.print((char)e.bit.KEY);
    if (e.bit.EVENT == KEY_JUST_PRESSED) {
      if (timeFlag) {
        timePress = (char)e.bit.KEY;
        Serial.print("timePress: "); Serial.println(timePress);
        Serial.print("curDisplay on timePress: "); Serial.println(curDisplay);
        timeFlag = 0;
        addTimeFlag = 1;

      } else if (orderFlag) {
        orderPress = (char)e.bit.KEY;
        Serial.print("orderPress: "); Serial.println(orderPress);
        Serial.print("curDisplay on orderFlag: "); Serial.println(curDisplay);
        orderFlag = 0;
        addOrderFlag = 1;

      } else if (valveFlag) {
        valvePress = (char)e.bit.KEY;
        Serial.print("valvePress: "); Serial.println(valvePress);
        Serial.print("curDisplay on orderFlag: "); Serial.println(curDisplay);
        valveFlag = 0;
        addValveFlag = 1;

      }
      else {
        button = (char)e.bit.KEY;
        Serial.print("button: "); Serial.println(button);
        Serial.print("curDisplay on button: "); Serial.println(curDisplay);
        selDisplayFlag = 1;
        noDisplayFlag = 1;
      }
    }
    //else if(e.bit.EVENT == KEY_JUST_RELEASED) Serial.println(" released");
  }
}

void showDisplay() {
  Home();
  Menu();
  Preset();
  Set();
  Ferti();
  Sensor();
  if (noDisplayFlag && selDisplayCount > -1 ) {
    curDisplay[--selDisplayCount] = '\0';
    Serial.print("curDisplay on noDisplay: "); Serial.println(curDisplay);
  }

}

void selDisplay() {
  if (selDisplayFlag) {
    selDisplayFlag = 0;
    if (button != '#') {
      if (button == 'C') {
        while (selDisplayCount != 0) {
          curDisplay[--selDisplayCount] = '\0';
        }
        curDisplay[selDisplayCount++] = button;
      } else {
        curDisplay[selDisplayCount++] = button;
      }
    }
    else {
      if (selDisplayCount > -1) {
        curDisplay[--selDisplayCount] = '\0';
      }
    }
  }

}

void Home()
{
  if (strcmp(curDisplay, "") == 0) {
    noDisplayFlag = 0;

    lcd.clear();
    lcd.setCursor(8, 1);
    lcd.print("DRIP");
    lcd.setCursor(5, 2);
    lcd.print("IRRIGRATION");
  }
}

void Menu()
{
  if (strcmp(curDisplay, "C") == 0)
  {
    noDisplayFlag = 0;

    valveFlag = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("*Main Menu*");

    lcd.setCursor(0, 1);
    lcd.print("1:");
    lcd.setCursor(0, 3);
    lcd.print("3:");
    lcd.setCursor(10, 1);
    lcd.print("|");
    lcd.setCursor(10, 3);
    lcd.print("|");
    lcd.setCursor(11, 1);
    lcd.print("2:");
    lcd.setCursor(11, 3);
    lcd.print("4:");

    lcd.setCursor(2, 1);
    lcd.print("PRESET");
    lcd.setCursor(2, 3);
    lcd.print("FERTI");
    lcd.setCursor(13, 1);
    lcd.print("SET");
    lcd.setCursor(13, 3);
    lcd.print("SENSOR");
  }
}

void Preset()
{
  if (strcmp(curDisplay, "C1") == 0) {
    noDisplayFlag = 0;
    if(config1Flag){
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#1");
    lcd.setCursor(2, 1);
    lcd.print("Already Running");
    lcd.setCursor(2, 2);
    lcd.print("Enter to STOP");
    lcd.setCursor(2, 3);
    lcd.print("Cancel to back");   
    }
    else if(config2Flag){
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#2");
    lcd.setCursor(2, 1);
    lcd.print("Already Running");
    lcd.setCursor(2, 2);
    lcd.print("Enter to STOP");
    lcd.setCursor(2, 3);
    lcd.print("Cancel to back");    
    }
    else if(config3Flag){
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#3");
    lcd.setCursor(2, 1);
    lcd.print("Already Running");
    lcd.setCursor(2, 2);
    lcd.print("Enter to STOP");
    lcd.setCursor(2, 3);
    lcd.print("Cancel to back");    
    }
    else if(config4Flag){
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#4");
    lcd.setCursor(2, 1);
    lcd.print("Already Running");
    lcd.setCursor(2, 2);
    lcd.print("Enter to STOP");
    lcd.setCursor(2, 3);
    lcd.print("Cancel to back");    
    }
    else if(config5Flag){
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#5");
    lcd.setCursor(2, 1);
    lcd.print("Already Running");
    lcd.setCursor(2, 2);
    lcd.print("Enter to STOP");
    lcd.setCursor(2, 3);
    lcd.print("Cancel to back");    
    }
    else if(config6Flag){
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#6");
    lcd.setCursor(2, 1);
    lcd.print("Already Running");
    lcd.setCursor(2, 2);
    lcd.print("Enter to STOP");
    lcd.setCursor(2, 3);
    lcd.print("Cancel to back");    
    }
    else{
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("*CONFIGURATIONS*");

    lcd.setCursor(0, 1);
    lcd.print("1:CONFIG1");
    lcd.setCursor(0, 2);
    lcd.print("3:CONFIG3");
    lcd.setCursor(0, 3);
    lcd.print("5:CONFIG5");
    lcd.setCursor(10, 1);
    lcd.print("|");
    lcd.setCursor(10, 2);
    lcd.print("|");
    lcd.setCursor(10, 3);
    lcd.print("|");
    lcd.setCursor(11, 1);
    lcd.print("2:CONFIG2");
    lcd.setCursor(11, 2);
    lcd.print("4:CONFIG4");
    lcd.setCursor(11, 3);
    lcd.print("6:CONFIG6");
    }
  }
    if (strcmp(curDisplay, "C1D") == 0) {
    config1Flag=0;config2Flag=0;config3Flag=0;config4Flag=0;config5Flag=0;config6Flag=0;
      for(int i=0;i<12;i++)
      {
         digitalWrite(valve[i],LOW);
      } 
      pumpOFF=millis();
      waitFlag=LOW;
      notset=LOW;
      set=HIGH; 
      c=-1;
      curDisplay[--selDisplayCount] = '\0';
  }
  if (strcmp(curDisplay, "C11") == 0) {
    noDisplayFlag = 0;
    if(config1Flag||config2Flag||config3Flag||config4Flag||config5Flag||config6Flag){
    curDisplay[--selDisplayCount] = '\0';
    }else{
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#1");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
    }
  }
  if (strcmp(curDisplay, "C11D") == 0) {
    noDisplayFlag = 0;
    config1Flag=1;
    waitFlag=LOW;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
  if (strcmp(curDisplay, "C12") == 0) {
    noDisplayFlag = 0;
    if(config1Flag||config2Flag||config3Flag||config4Flag||config5Flag||config6Flag){
    curDisplay[--selDisplayCount] = '\0';
    }else{
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#2");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
    }
  }
  if (strcmp(curDisplay, "C12D") == 0) {
    noDisplayFlag = 0;
    config2Flag=1;
    waitFlag=LOW;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
  if (strcmp(curDisplay, "C13") == 0) {
    noDisplayFlag = 0;
    if(config1Flag||config2Flag||config3Flag||config4Flag||config5Flag||config6Flag){
    curDisplay[--selDisplayCount] = '\0';
    }else{
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#3");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
    }
  }
  if (strcmp(curDisplay, "C13D") == 0) {
    noDisplayFlag = 0;
    config3Flag=1;
    waitFlag=LOW;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
  if (strcmp(curDisplay, "C14") == 0) {
    noDisplayFlag = 0;  
    if(config1Flag||config2Flag||config3Flag||config4Flag||config5Flag||config6Flag){
    curDisplay[--selDisplayCount] = '\0';
    }else{
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#4");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
    }
  }
  if (strcmp(curDisplay, "C14D") == 0) {
    noDisplayFlag = 0;
    config4Flag=1;
    waitFlag=LOW;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
  if (strcmp(curDisplay, "C15") == 0) {
    noDisplayFlag = 0;
    if(config1Flag||config2Flag||config3Flag||config4Flag||config5Flag||config6Flag){
    curDisplay[--selDisplayCount] = '\0';
    }else{  
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#5");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
    }
  }
  if (strcmp(curDisplay, "C15D") == 0) {
    noDisplayFlag = 0;
    config5Flag=1;
    waitFlag=LOW;
    notset=LOW;
    set=HIGH;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
  if (strcmp(curDisplay, "C16") == 0) {
    noDisplayFlag = 0; 
    if(config1Flag||config2Flag||config3Flag||config4Flag||config5Flag||config6Flag){
    curDisplay[--selDisplayCount] = '\0';
    }else{   
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#6");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
    }
  }
  if (strcmp(curDisplay, "C16D") == 0) {
    noDisplayFlag = 0;
    config6Flag=1;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
}

void Set()
{
  if (strcmp(curDisplay, "C2") == 0) {
    noDisplayFlag = 0;

    valveFlag = 1;
    getValve();
    int maxVal = valveArray.toInt();
    if (maxVal == 0 || maxVal > 12) {
      valveArray = "";
      Serial.print("maxVal: "); Serial.println(maxVal);
    }
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("*SELECT VALVE*");

    lcd.setCursor(6, 1);
    lcd.print("VALVE#");
    lcd.print(valveArray);
    lcd.setCursor(2, 2);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 3);
    lcd.print("Cancel to Abort");
    zeroValve = 1;
  }

  if (strcmp(curDisplay, "C2D") == 0) {
    noDisplayFlag = 0;
    valveFlag = 0;

    if (zeroValve) {
      zeroValve = 0;
      Serial.print("valveArray: "); Serial.println(valveArray);
      disNum = valveArray.toInt();
      if (disNum < 10) {
        curDisplay[selDisplayCount++] = valveArray[0];
      } else {
        curDisplay[selDisplayCount++] = valveArray[0];
        curDisplay[selDisplayCount++] = valveArray[1];
      }
      valveArray = "";
    } else {
      curDisplay[--selDisplayCount] = '\0';
    }

  }

  selValve(disNum);
}

void Ferti()
{
  if (strcmp(curDisplay, "C3") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("FERTI#1");
    lcd.setCursor(2, 1);
    lcd.print("1: ON FERTI#1");
    lcd.setCursor(2, 2);
    lcd.print("2: OFF FERTI#1");
  }
  if (strcmp(curDisplay, "C31") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("FERTI#1 ON");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
  if (strcmp(curDisplay, "C31D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
  if (strcmp(curDisplay, "C32") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("FERTI#1 OFF");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
  if (strcmp(curDisplay, "C32D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
}

void Sensor()
{
  if (strcmp(curDisplay, "C4") == 0) {
    noDisplayFlag = 0;
    if (millis() - sensorData > 1000) {
      sensorData = millis();
      getPH();
      sensors.requestTemperatures();
      Celcius = sensors.getTempCByIndex(0);
    }
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("PH = ");
    lcd.print(voltage);
    lcd.setCursor(2, 1);
    lcd.print("TEMP = ");
    lcd.print(Celcius);
    lcd.print("C");
    lcd.setCursor(2, 2);
    lcd.print("COND = 0");
    lcd.setCursor(2, 3);
    lcd.print("MOSIT = 0");

  }
}

void selValve(int i) {

  String C2DX = "C2D" + String(i);
  C2DX.toCharArray(buf, (C2DX.length() + 1) );
  if (strcmp(curDisplay, buf) == 0) {
    memset(buf, 0, sizeof(buf));
    noDisplayFlag = 0;
    timeFlag = 0;
    orderFlag = 0;
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("*VALVE#");
    lcd.print(i);
    lcd.print(" SETTING*");
    lcd.setCursor(2, 1);
    lcd.print("1: Time(mins)");
    lcd.setCursor(2, 2);
    lcd.print("2: Order");
  }
  String C2DX1 = "C2D" + String(i) + '1';
  C2DX1.toCharArray(buf, (C2DX1.length() + 1) );
  if (strcmp(curDisplay, buf) == 0) {
    noDisplayFlag = 0;
    timeFlag = 1;
    getTime();
    int maxVal = timeArray.toInt();
    if (maxVal == 0 || maxVal > 999) {
      timeArray = "";
    }
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("VALVE#");
    lcd.print(i);
    lcd.print(" Time(min)");
    lcd.setCursor(5, 2);
    lcd.print("Time = ");
    lcd.print(timeArray);
    zeroTime = 1;
  }

  String C2DX1D = "C2D" + String(i) + "1D";
  C2DX1D.toCharArray(buf, (C2DX1D.length() + 1) );
  if (strcmp(curDisplay, buf) == 0) {
    memset(buf, 0, sizeof(buf));
    noDisplayFlag = 0;
    timeFlag = 0;
    if (zeroTime) {
      zeroTime = 0;
      Serial.print("timeArray: "); Serial.println(timeArray);
      setTime = timeArray.toInt();
      timeArray = "";
    }
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Time For V");
    lcd.print(i);
    lcd.print(" = ");
    lcd.print(setTime);
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }

  String C2DX1DD = "C2D" + String(i) + "1DD";
  C2DX1DD.toCharArray(buf, (C2DX1DD.length() + 1) );
  if (strcmp(curDisplay, buf) == 0) {
    memset(buf, 0, sizeof(buf));
    noDisplayFlag = 0;
    selTime[i - 1] = setTime;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }

  String C2DX2 = "C2D" + String(i) + '2';
  C2DX2.toCharArray(buf, (C2DX2.length() + 1) );
  if (strcmp(curDisplay, buf) == 0) {
    memset(buf, 0, sizeof(buf));
    noDisplayFlag = 0;
    orderFlag = 1;
    getOrder();
    int maxVal = orderArray.toInt();

    if (maxVal == 0 || maxVal > 12) {
      orderArray = "";
    }
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("VALVE#");
    lcd.print(i);
    lcd.print(" Order");
    lcd.setCursor(5, 2);
    lcd.print("Order = ");
    lcd.print(orderArray);
    zeroOrder = 1;
  }

  String C2DX2D = "C2D" + String(i) + "2D";
  C2DX2D.toCharArray(buf, (C2DX2D.length() + 1) );
  if (strcmp(curDisplay, buf) == 0) {
    memset(buf, 0, sizeof(buf));
    noDisplayFlag = 0;
    orderFlag = 0;
    if (zeroOrder) {
      zeroOrder = 0;
      Serial.print("orderArray: "); Serial.println(orderArray);
      setOrder = orderArray.toInt();
      orderArray = "";
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("*SELECT VALVE ORDER*");

    lcd.setCursor(6, 1);
    lcd.print("Order#");
    lcd.print(setOrder);
    lcd.setCursor(2, 2);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 3);
    lcd.print("Cancel to Abort");
  }

  String C2DX2DD = "C2D" + String(i) + "2DD";
  C2DX2DD.toCharArray(buf, (C2DX2DD.length() + 1) );
  if (strcmp(curDisplay, buf) == 0) {
    memset(buf, 0, sizeof(buf));
    noDisplayFlag = 0;
    selOrder[i - 1] = setOrder;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
}

void getOrder() {
  if (orderPress == 'D') {
    curDisplay[selDisplayCount++] = orderPress;
    Serial.print("IngetOrder: "); Serial.println(curDisplay);
  }
  else if (orderPress == '#') {
    orderArray = "";
    curDisplay[--selDisplayCount] = '\0';
  }
  else {
    if (addOrderFlag) {
      addOrderFlag = 0;
      if (orderPress == 'A' || orderPress == 'B' || orderPress == 'C' || orderPress == '*' ) {}
      else {
        orderArray += String(orderPress);
        Serial.print("orderArray in getOrder : "); Serial.println(timeArray);
      }
    }
  }
  orderPress = '\0';
}

void getValve() {
  if (valvePress == 'D') {
    curDisplay[selDisplayCount++] = valvePress;
    Serial.print("IngetValve: "); Serial.println(curDisplay);
  }
  else if (valvePress == '#') {
    valveArray = "";
    curDisplay[--selDisplayCount] = '\0';
  }
  else {
    if (addValveFlag) {
      addValveFlag = 0;
      if (valvePress == 'A' || valvePress == 'B' || valvePress == 'C' || valvePress == '*' ) {}
      else {
        Serial.print("valveArray in getValve : "); Serial.println(valveArray);
        valveArray += String(valvePress);
      }
    }
  }
  valvePress = '\0';
}

void getTime() {
  if (timePress == 'D') {
    curDisplay[selDisplayCount++] = timePress;
    Serial.print("IngetTime: "); Serial.println(curDisplay);
  }
  else if (timePress == '#') {
    timeArray = "";
    curDisplay[--selDisplayCount] = '\0';
  }
  else {
    if (addTimeFlag) {
      addTimeFlag = 0;
      if (timePress == 'A' || timePress == 'B' || timePress == 'C' || timePress == '*' ) {}
      else {
        timeArray += String(timePress);
        Serial.print("timeArray in getTime : "); Serial.println(timeArray);
      }
    }
  }
  timePress = '\0';
}

void conFig(){
  if(config1Flag||config2Flag||config3Flag||config4Flag||config5Flag)
  {
    if(!pump)
  {
    digitalWrite(PUMPpin,HIGH);
    pump=HIGH;  
    startTime=millis();
    c=-1;
  }
  else
  {
    if(c<12)
    {
      process();
    }
    else
    {
      digitalWrite(PUMPpin,LOW);
      pump=LOW;
      config1Flag=0;config2Flag=0;config3Flag=0;config4Flag=0;config5Flag=0;
    }
  }
 }else{
  if(pump && millis()-pumpOFF>5000){
    pump=LOW;
    digitalWrite(PUMPpin,LOW);
  }
 }
}

void process()
{
  
  if(waitFlag)
  {
    if(config5Flag && set){
    set=LOW;notset=HIGH;
    digitalWrite(valve[c++],HIGH);
    digitalWrite(valve[c],HIGH);
    }else{
    digitalWrite(valve[c],HIGH);
    }
    Delay(Time);
  }
  else
  {
    sec_10();
  }
}

void Delay(int t)
{
  if(millis() - startTime>t)
  {
    startTime = millis();
    if(config5Flag && notset){
    notset=LOW;set=HIGH;  
    digitalWrite(valve[c--],LOW);
    digitalWrite(valve[c],LOW);
    c++;
    }else{
    digitalWrite(valve[c],LOW);
    }
    waitFlag=LOW;
  }
}
void sec_10()
{
  if(millis() - startTime>5000)
  {
    c++;
    startTime = millis();
    waitFlag=HIGH;
    Serial.print("Valve = ");Serial.println(c);
  }
}
