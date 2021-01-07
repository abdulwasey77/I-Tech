#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

#define RES2 820.0
#define ECREF 200.0
#define EC_PIN A0
float voltage,ecValue;

float Celcius=0;
float Fahrenheit=0;
 
void setup()
{
  Serial.begin(115200);  
  sensors.begin();
}

void loop()
{
    static unsigned long timepoint = millis();
    if(millis()-timepoint>3000U)  //time interval: 1s
    {
      timepoint = millis();
      sensors.requestTemperatures(); 
      Celcius=sensors.getTempCByIndex(0);
      voltage=0.0;
      for(int i=0; i++; i<50){
        voltage+=analogRead(EC_PIN);
        delay(1);
      }
      voltage /=50;
      voltage = analogRead(EC_PIN)/1024.0*5;
      Serial.print("voltage: "); Serial.println(voltage);    
      ecValue=1000000*voltage/RES2/ECREF;
      ecValue/=(1.0+0.0185*(Celcius-25.0));
      Serial.print("temperature:");
      Serial.print(Celcius);
      Serial.print("^C  EC:");
      Serial.print(ecValue);
      Serial.println("ms/cm");
      float EC_ppm=ecValue*640;
      Serial.print("EC in PPM: "); Serial.println(EC_ppm);
    }
}
