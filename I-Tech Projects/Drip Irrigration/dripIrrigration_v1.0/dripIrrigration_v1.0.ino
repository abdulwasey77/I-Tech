#include "Adafruit_Keypad.h"
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 10

OneWire oneWire(ONE_WIRE_BUS);
 float Celcius=0;
 float Fahrenheit=0;
DallasTemperature sensors(&oneWire);

int lcdColumns = 20;
int lcdRows = 4;
const byte ROWS = 4;
const byte COLS = 4;
int pHSense = A0;
int samples = 20;
float adc_resolution = 1023.0;
float analogvol;

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
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

char curDisplay[10], selDisplayFlag = 0, noDisplayFlag = 0, button, preButton;
int selDisplayCount = 0;
float voltage;

void setup() {
  Serial.begin(9600);
  customKeypad.begin();
  sensors.begin();
  lcd.init();
  lcd.backlight();
}

void loop() {
  keyPress();
  selDisplay();
  showDisplay();
  delay(1000);
}

void getPH(){
    int measurings=0;
  for (int i = 0; i < samples; i++)
  {
    measurings += analogRead(pHSense);
    delay(1);
  }
    analogvol=measurings/samples;
    analogvol=(analogvol*5)/1023;
    Serial.print("analog voltage = "); Serial.println(analogvol);
    voltage = 5 / adc_resolution * measurings/samples;
    voltage = 7 + ((2.5 - voltage) / 0.18);
}

void keyPress() {
  customKeypad.tick();
  while (customKeypad.available()) {
    keypadEvent e = customKeypad.read();
    //Serial.print((char)e.bit.KEY);
    if (e.bit.EVENT == KEY_JUST_PRESSED) {
      button = (char)e.bit.KEY;
      Serial.println(button);
      selDisplayFlag = 1;
      noDisplayFlag = 1;
    }
    //else if(e.bit.EVENT == KEY_JUST_RELEASED) Serial.println(" released");
  }
}

void showDisplay() {
  Home();
  Menu();
  Auto();
  Manual();
  PH();
  Temp();
  Cond();
  if (noDisplayFlag && selDisplayCount > -1 ) {
    curDisplay[--selDisplayCount] = '\0';
    Serial.print("curDisplay : "); Serial.println(curDisplay);
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

    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("*Main Menu*");

    lcd.setCursor(0, 1);
    lcd.print("1:");
    lcd.setCursor(0, 2);
    lcd.print("2:");
    lcd.setCursor(0, 3);
    lcd.print("3:");
    lcd.setCursor(10, 1);
    lcd.print("|");
    lcd.setCursor(10, 2);
    lcd.print("|");
    lcd.setCursor(10, 3);
    lcd.print("|");
    lcd.setCursor(11, 1);
    lcd.print("4:");
    lcd.setCursor(11, 2);
    lcd.print("5:");
    lcd.setCursor(11, 3);
    lcd.print("6:");

    lcd.setCursor(2, 1);
    lcd.print("Auto");
    lcd.setCursor(2, 2);
    lcd.print("Manual");
    lcd.setCursor(2, 3);
    lcd.print("PH");
    lcd.setCursor(13, 1);
    lcd.print("TEMP");
    lcd.setCursor(13, 2);
    lcd.print("COND");
    lcd.setCursor(13, 3);
    lcd.print("");
  }
}

void Auto()
{
  if (strcmp(curDisplay, "C1") == 0) {
    noDisplayFlag = 0;

    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("*CONFIGURATIONS*");

    lcd.setCursor(0, 1);
    lcd.print("1:CONFIG1");
    lcd.setCursor(0, 2);
    lcd.print("2:CONFIG2");
    lcd.setCursor(0, 3);
    lcd.print("3:CONFIG3");
    lcd.setCursor(10, 1);
    lcd.print("|");
    lcd.setCursor(10, 2);
    lcd.print("|");
    lcd.setCursor(10, 3);
    lcd.print("|");
    lcd.setCursor(11, 1);
    lcd.print("4:CONFIG4");
    lcd.setCursor(11, 2);
    lcd.print("5:CONFIG5");
    lcd.setCursor(11, 3);
    lcd.print("6:CONFIG6");
  }
  if (strcmp(curDisplay, "C11") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#1");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C11D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
    if (strcmp(curDisplay, "C12") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#2");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C12D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
    if (strcmp(curDisplay, "C13") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#3");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C13D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
    if (strcmp(curDisplay, "C14") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#4");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C14D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
    if (strcmp(curDisplay, "C15") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#5");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C15D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
    if (strcmp(curDisplay, "C16") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIGURATION#6");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C16D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
}

void Manual()
{
  if (strcmp(curDisplay, "C2") == 0) {
    noDisplayFlag = 0;

    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("*SELECT VALVE*");

    lcd.setCursor(0, 1);
    lcd.print("1");
    lcd.setCursor(0, 2);
    lcd.print("2");
    lcd.setCursor(0, 3);
    lcd.print("3");
    lcd.setCursor(3, 1);
    lcd.print("|");
    lcd.setCursor(3, 2);
    lcd.print("|");
    lcd.setCursor(3, 3);
    lcd.print("|");
    lcd.setCursor(6, 1);
    lcd.print("4");
    lcd.setCursor(6, 2);
    lcd.print("5");
    lcd.setCursor(6, 3);
    lcd.print("6");

    lcd.setCursor(9, 1);
    lcd.print("|");
    lcd.setCursor(9, 2);
    lcd.print("|");
    lcd.setCursor(9, 3);
    lcd.print("|");

    lcd.setCursor(12, 1);
    lcd.print("7");
    lcd.setCursor(12, 2);
    lcd.print("8");
    lcd.setCursor(12, 3);
    lcd.print("9");
    lcd.setCursor(15, 1);
    lcd.print("|");
    lcd.setCursor(15, 2);
    lcd.print("|");
    lcd.setCursor(15, 3);
    lcd.print("|");
    lcd.setCursor(18 , 1);
    lcd.print("10");
    lcd.setCursor(18, 2);
    lcd.print("11");
    lcd.setCursor(18, 3);
    lcd.print("12");

  }
  
    if (strcmp(curDisplay, "C21") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("VALVE#1");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C21D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
  
    if (strcmp(curDisplay, "C22") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("VALVE#2");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C22D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
  
    if (strcmp(curDisplay, "C23") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("VALVE#3");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C23D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
  
    if (strcmp(curDisplay, "C24") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("VALVE#4");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C24D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
  
    if (strcmp(curDisplay, "C25") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("VALVE#5");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C25D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }

    if (strcmp(curDisplay, "C26") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("VALVE#6");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C26D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }

    if (strcmp(curDisplay, "C27") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("VALVE#7");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C27D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }

    if (strcmp(curDisplay, "C28") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("VALVE#8");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C28D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }

    if (strcmp(curDisplay, "C29") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("VALVE#9");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C29D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }

    if (strcmp(curDisplay, "C210") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("VALVE#10");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C210D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
  
    if (strcmp(curDisplay, "C211") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("VALVE#11");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C211D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
  
    if (strcmp(curDisplay, "C212") == 0) {
    noDisplayFlag = 0;
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("VALVE#12");
    lcd.setCursor(2, 1);
    lcd.print("Enter to Select");
    lcd.setCursor(2, 2);
    lcd.print("Cancel to Abort");
  }
    if (strcmp(curDisplay, "C212D") == 0) {
    noDisplayFlag = 0;
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
    curDisplay[--selDisplayCount] = '\0';
  }
}

void PH()
{
  if (strcmp(curDisplay, "C3") == 0) {
    noDisplayFlag = 0;
    getPH();
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("PH = ");
    lcd.print(voltage);
    
  }
}

void Temp()
{
  if (strcmp(curDisplay, "C4") == 0) {
    noDisplayFlag = 0;
    sensors.requestTemperatures(); 
    Celcius=sensors.getTempCByIndex(0);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("TEMP = ");
    lcd.print(Celcius);
    lcd.print("C");
    
  }
}

void Cond()
{
  if (strcmp(curDisplay, "C5") == 0) {
    noDisplayFlag = 0;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("COND = 0");
  }
}
