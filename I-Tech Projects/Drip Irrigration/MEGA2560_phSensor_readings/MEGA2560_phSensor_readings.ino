#include <Arduino.h>
int pHSense = A1;
float samples = 500.0;
float adc_resolution = 1023.0;
float analogvol;
float ph;
void setup()
{
  Serial.begin(9600);
  delay(100);
  Serial.println("cimpleo pH Sense");
}


void loop(){
  long measurings=0;
  for (int i = 0; i < samples; i++)
  {
    measurings += analogRead(pHSense);
    delay(10);
  }
    analogvol=measurings/samples;
    analogvol=(analogvol*5.0)/1023.0;
    Serial.print("analog voltage = "); Serial.println(analogvol);
    ph= 7 + ((2.57 - analogvol) / 0.18);
    if(ph<=0.3)
    ph=0.0;
    else if(ph>0.3 && ph<3.5)
    ph=ph-0.3;
    else
    ph=ph+0.2;
    
    Serial.print("pH= ");
    Serial.println(ph);
    delay(3000);
}
