#include <Wire.h>
#include <Adafruit_ADS1015.h>
 
Adafruit_ADS1115 ads1(0x48);
Adafruit_ADS1115 ads2(0x49);
 unsigned long sec1;
 int c=0;
void setup()
{
Serial.begin(115200);
Serial.println("Hello!");
 
Serial.println("Getting single-ended readings from AIN0..3");
Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");
 
ads1.begin();
ads2.begin();
delay(3000);
sec1=millis();
}
 
void loop()
{
int16_t adc10, adc11, adc12, adc13, adc20, adc21, adc22, adc23;
 adc10 = ads1.readADC_SingleEnded(0);
 c++;
 Serial.print(c); Serial.print("   "); Serial.println(adc10);
 while(millis()-sec1>1000){
  
 }

adc11 = ads1.readADC_SingleEnded(1);
adc12 = ads1.readADC_SingleEnded(2);
adc13 = ads1.readADC_SingleEnded(3);
 
adc20 = ads2.readADC_SingleEnded(0);
adc21 = ads2.readADC_SingleEnded(1);
adc22 = ads2.readADC_SingleEnded(2);
adc23 = ads2.readADC_SingleEnded(3);
//Serial.print("DATA : ");

//Serial.print("   ");
//Serial.println(adc11);
//Serial.print("   ");
//Serial.print(adc12);
//Serial.print("   ");
//Serial.print(adc13);
//Serial.print("   ");
//Serial.print(adc20);
//Serial.print("   ");
//Serial.print(adc21);
//Serial.print("   ");
//Serial.print(adc22);
//Serial.print("   ");
//Serial.println(adc23); 
}
