
//--------------------------------------------Include libraries-----------------------//
#include "Energia.h"
#include <WiFi.h>
#include <WiFiServer.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include "DHT.h"
#include "RTC_Library.h"
#include "NTP_WiFi.h"
#include "time.h"
#include <SPI.h>
#include <SLFS.h>


//-----------------------------------Declare Variables------------------------------------//
DateTime myRTC;
time_t myEpochNTP, myEpochRTC;
tm myTimeNTP, myTimeRTC;

int c = 0,d=0;
// First time update of RTC


#define PUSH4 4
#define DHTPIN 5
#define DHTTYPE DHT22
#define Battery 6
#define CartLock 27
#define CartFlooded 14
#define CartPanFlooded 15
#define Relay 19
#define power24V 11
#define powerRelay 7
#define wifiLed 29

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;

bool flagNTP;
//------------integers-----------------//
int whileFlag = 1;
uint32_t sendNTPpacket(IPAddress& address);
int Battvalue;
int Battperct;
int WiFiperct;
int btnpress = 0,breakWifi=0;
int GMTint;


//-----------float------------------//
float GMTfloat;
//-----------Long-----------------//
unsigned long startMillis = 0;


//-----------Characters------------//
//char ssid_pd[] = "I-Tech-Router";
//char password_pd[] = "0512303189T";
char ssid_AP[] = "CC3200-launchpad";
char password_AP[] = "launchpad";
char ssid_Sta[] = "";
char password_Sta[] = "";
const char* host = "iotrpi.azurewebsites.net";

//----------Strings---------------//

//String temperatureRead="";;
//String humidityRead="";;
String macRead;
String sendRead;
String pressure;
String ConfigFlashParam;
String ssid;
String password;
String passwordnew;
String CoustomerID;
String CountryCode;
String TimeZone;
String CartLockStatus;
String CartFloodedStatus;
String CartPanFloodedStatus;
String RelayStatus;

//------------server-------------//
WiFiServer server(80);

//--------------------------------------SETUP-------------------------------//
void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("Here");
  SerFlash.begin();
  pinMode(wifiLed,OUTPUT);
  digitalWrite(wifiLed,HIGH);
  pinMode(PUSH4, INPUT_PULLUP);
  pinMode(Battery, INPUT);
  pinMode(CartLock, INPUT);
  pinMode(CartFlooded, INPUT);
  pinMode(CartPanFlooded, INPUT);
  pinMode(Relay, OUTPUT);
  pinMode(power24V, INPUT);
  pinMode(powerRelay, OUTPUT);
  attachInterrupt(PUSH4, callIntrpt, CHANGE); // Interrupt is fired whenever button is pressed
  checkpower();
  sensorBegin();
  myRTC.begin();
  getSsidPassFlash();
  if (ConfigFlashParam[0] != '$') {
    getSsidPassAP();
    connToSTA_Server();
    getNTP();
  } else {
    connToSTA_Server();
    getNTP();
  }
  Serial.println("System Ready....");
}

//--------------------------------LOOP----------------------------------------//
void loop() {
  checkpower();
  if (ConfigFlashParam[0] != '$') {
    getSsidPassAP();
    connToSTA_Server();
    getNTP();
  } else {
    delay(5000);
    checkpower();
    checkIntrpt();
    BattLevelCheck();
    getMacAddress();
    WiFiStrenghtInprcnt();
    readInputStatus();
    timeZoneSet();
    myEpochRTC = myRTC.getTime();
    myEpochRTC = myEpochRTC + GMTint; //+(3600*GMThours);  //1hour=3600sec
    convertEpoch2Structure(myEpochRTC, myTimeRTC);
    pressure = String(bmp.readPressure() / 248.84);
    sendRead = JSONString();
    Serial.print("Post String:  "); Serial.println(sendRead);
    delay(500);
    postData();
    delay(500);
  }
}
//-------------------------------------Post data on server--------------------------//
void checkpower() {
  if (digitalRead(power24V) == LOW)
  {
    d++;
    if (d == 1)
    {
    Serial.println("Power Shifted to 12v");
    digitalWrite(powerRelay, HIGH);
    delay(300);
    sensorBegin();
    c = 0;
    }
  }
  else if (digitalRead(power24V) == HIGH)
  {
    c++;
    if (c == 1)
    {
      digitalWrite(powerRelay, LOW);
      delay(300);
      sensorBegin();
      Serial.println("Power Shifted to 24v");
      d=0;
    }
  }
}


void postData() {
  WiFiClient client;   // listen for incoming clients
  if (!client.connect(host, 80)) {
    Serial.println("connection failed");
  }
  if (client) {                             // if you get a client,

    while (client.connected())
    {
      Serial.println("Uploading Data to Kestrel Server");
      Serial.println();
      checkIntrpt();
      client.print("POST /IOT/AddIOTPiData HTTP/1.1\n");
      client.print("Host: iotrpi.azurewebsites.net\n");
      client.print("Connection: close\n");
      client.print("Content-Type: application/json\n");
      client.print("Accept: text/plain\n");

      client.print("Content-Length: ");
      client.print(sendRead.length());
      client.print("\n\n");
      client.print(sendRead);
      client.print("\r\n\r\n");

      long timedelay = millis();
      while (client.available() == 0)
      {
        checkIntrpt();
        if (millis() - timedelay > 5000)
        {
          client.stop();
          Serial.println("Nothing received in response...TimeOut");
          return;
        }
      }
      while (client.available())
      {
        char c = client.read();
        Serial.print(c);
      }
      delay(100);
    }
  }
}
//-------------------------------------SensorBegin Function----------------------------//

void sensorBegin() 
{
    if (!bmp.begin()) 
    {
      Serial.println("Could not find a valid BMP085/BMP180 sensor, check wiring!");
      while (1) {}
    }
  dht.begin();
}

//---------------------------Config STAmode and Start Server--------------------------//


void connToSTA_Server() {
  Serial.print("Connecting to : ");
  Serial.println(ssid_Sta);
  WiFi.begin(ssid_Sta, password_Sta);
  while ( WiFi.status() != WL_CONNECTED) 
  {
    checkIntrpt();
    if(breakWifi){
      breakWifi=0;
      break;
    }
    Serial.print(".");
    delay(300);
  }
  Serial.println("Connected");
  while (WiFi.localIP() == INADDR_NONE) 
  {
    delay(300);
  }
  Serial.print("IP Address :");Serial.println(WiFi.localIP());
  Serial.println("Starting webserver on port 80");
  server.begin();                           // start the web server on port 80
  Serial.println("Webserver started!");
  digitalWrite(wifiLed,LOW);
}


//-----------------------------Get time Date from NTP and Set RTC---------------------//


void getNTP() 
{
  Serial.println("Getting NTP time from Server ");
  while (whileFlag) 
  {
    flagNTP = getTimeNTP(myEpochNTP);
    if (myEpochNTP > 0) 
    {
      whileFlag = 0;
    }
    Serial.print(".");
    delay(250);
  }
  myRTC.setTime(myEpochNTP);
  Serial.println("RTC updated!");

  myEpochNTP = myEpochNTP;
  myEpochRTC = myRTC.getTime();
  myEpochRTC = myEpochRTC;
  convertEpoch2Structure(myEpochNTP, myTimeNTP);
  convertEpoch2Structure(myEpochRTC, myTimeRTC);

  Serial.print("RTC = \t");
  Serial.println(convertDateTime2String(myTimeRTC));
  myEpochRTC = 0;
}
//----------------------------Get MacAddress---------------------------------------//

void getMacAddress() 
{
  byte mac[6];
  WiFi.macAddress(mac);
  macRead = String(mac[5], HEX);
  macRead += ":";
  macRead += String(mac[4], HEX);
  macRead += ":";
  macRead += String(mac[3], HEX);
  macRead += ":";
  macRead += String(mac[2], HEX);
  macRead += ":";
  macRead += String(mac[1], HEX);
  macRead += ":";
  macRead += String(mac[0], HEX);
}


//------------------Get Value from DHT22 and convert it into String----------------//


String getDHT22humidity() 
{
  float h = dht.readHumidity();
  String sh = String(h);
  return sh;
}
String getDHT22temperature() 
{
  float t = dht.readTemperature();
  String st = String(t);
  return st;
}

//-----------------------------------Call JSON String Function-----------------------//

String JSONString() 
{
  String send = "";
  send = "{\"id\":";
  send += 0;
  send += ",\"ccCatID\":\"";
  send += macRead;
  send += "\",\"ccCustomerID\":\"";
  send += CoustomerID;
  send += "\",\"ccCountryCode\":\"";
  send += CountryCode;
  send += "\",\"ccTimeZone\":\"";
  send += TimeZone;
  send += "\",\"ccTimeStamp\":\"";
  send += (convertDateTime2String(myTimeRTC));
  send += "\",\"ccTempC\":\"";
  send += getDHT22temperature();
  send += "\",\"ccRH\":\"";
  send += getDHT22humidity();
  send += "\",\"ccBMP\":\"";
  send += pressure;
  send += "\",\"ccCartLock\":\"";
  send += CartLockStatus;
  send += "\",\"ccCartFlooded\":\"";
  send += CartFloodedStatus;
  send += "\",\"ccCartPanFlooded\":\"";
  send += CartPanFloodedStatus;
  send += "\",\"ccRelayOn\":\"";
  send += "unknown";
  send += "\",\"ccWifi\":\"";
  send += String(WiFiperct);
  send += "\",\"ccMalfunction\":\"";
  send += "unknown";
  send += "\",\"ccBattLevel\":\"";
  send += String(Battperct);
  send += "\",\"ccTimeCartChange\":\"";
  send += "not defined";
  send += "\",\"ccMessage\":\"";
  send += "not defined";
  send += "\",\"dateCreated\":\"";
  send += "not defined";
  send += "\",\"dateModified\":\"";
  send += "not defined";
  send += "\"}";
  return send;
}

//-----------------------------------WiFi strength in percentage---------------------//

void WiFiStrenghtInprcnt() 
{
  long rssi = WiFi.RSSI();
  rssi = -rssi;
  if (rssi == 0) 
  {
  }
  else if (rssi < 27 && rssi != 0) 
  {
    WiFiperct = 100;
  }
  else if (rssi >= 27 && rssi < 33) 
  {
    WiFiperct = 150 - (5 / 2.7) * rssi;
  }
  else if (rssi >= 33 && rssi < 36) 
  {
    WiFiperct = 150 - (5 / 3) * rssi;
  }
  else if (rssi >= 36 && rssi < 40)
  {
    WiFiperct = 150 - (5 / 3.3) * rssi;
  }
  else if (rssi >= 40 && rssi < 90) 
  {
    WiFiperct = 150 - (5 / 3.5) * rssi;
  }
  else if (rssi >= 90 && rssi < 99) 
  {
    WiFiperct = 150 - (5 / 3.3) * rssi;
  }
}

//-----------------------------------Print Wifi Status Function----------------------//

void printWifiStatus() 
{
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}


//-----------------------------Check Battery level Function-----------------------//


void BattLevelCheck() 
{
  Battvalue = 0;
  for (int i = 0; i < 100; i++) 
  {
    Battvalue = Battvalue + analogRead(Battery);
    delay(1);
  }
  Battvalue = Battvalue / 100;
  if (Battvalue >= 3400 && Battvalue <= 4100) 
  {
    Battperct = Battvalue - 4100;
    Battperct = Battperct + 700;
    Battperct = (Battperct * 100) / 700;
  }
  else if (Battvalue > 4100) 
  {
    Battperct = 100;
  }
  else 
  {
    Battperct = 0;
  }
}

//--------------------------Read INPUT pins status---------------------------//
void readInputStatus() 
{
  if (digitalRead(CartLock) == HIGH) 
  {
    CartLockStatus = "Yes";
    Serial.println("Cartage Locked" );
  } 
  else 
  {
    CartLockStatus = "No";
    
    Serial.println("Cartage not Locked" );
  }
  if (digitalRead(CartFlooded) == HIGH) 
  {
    CartFloodedStatus = "Yes";
    
    Serial.println("Cartage Flooded" );
  } 
  else 
  {
    CartFloodedStatus = "No";
  }
  if (digitalRead(CartPanFlooded) == HIGH) 
  {
    CartPanFloodedStatus = "Yes";
    
    Serial.println("Cartage Pan Flooded" );
  } 
  else 
  {
    CartPanFloodedStatus = "No";
  }
}

//---------------------------Check interrupt is called----------------------------------//

void callIntrpt()
{
  if (digitalRead(PUSH4) == HIGH) 
  {
    startMillis = millis();
    btnpress = 1;
  }
}

//----------------------------Call interrupt function------------------------------------//

void checkIntrpt() 
{

  if (btnpress == 1 && (millis() - startMillis) >= 3000) {
    deleteSsidPassFlash();
    WiFi._initialized = false;
    WiFi._connecting = false;
    btnpress = 0;
    breakWifi=1;
    Serial.println(" Ready to Config Again...");
  }
}

//----------------------------------Go to APmode and read SSID PASS---------------------------//

void getSsidPassAP() 
{
  Serial.print("Setting Ap-Mode");
  WiFi.beginNetwork(ssid_AP, password_AP);

  while (WiFi.localIP() == INADDR_NONE)
  {
    Serial.println('.');
    delay(300);
  }
  IPAddress ip = WiFi.localIP();
  Serial.print("Webserver IP address = ");
  Serial.println(ip);

  server.begin();           // start the web server on port 80
  
  Serial.print("Ready For config..");
  while (ConfigFlashParam[0] != '$') 
  {
    int i = 0;
    String temp = "";               //sting temp
    WiFiClient client = server.available();   // listen for incoming clients
    if (client) 
    {
      char buffer[150] = {0};                 // make a buffer to hold incoming data
      while (client.connected()) 
      {            // loop while the client's connected
        if (client.available()) 
        {             // if there's bytes to read from the client,
          char c = client.read();             // read a byte, then
          temp += c;                          // temp is string and it will store incoming bytes in string (SSID,PASSWORD is stored in temp)
          Serial.write(c);                    // print it out the serial monitor
          if (c == '\n') 
          {                    // if the byte is a newline character
          }
          else if (c != '\r') 
          {    // if you got anything else but a carriage return character,
            buffer[i++] = c;      // add it to the end of the currentLine
          }
        }
      }
      // close the connection:
      ConfigFlashParam = temp;
      if (ConfigFlashParam[0] == '$') 
      {
        client.stop();
        StoreSsisPassFlash();
        getSsidPassFlash();
        WiFi._initialized = false;
        WiFi._connecting = false;
      }
    }
  }
}

//----------------------SSID check and convert it into char----------------------------------------//

void CheckingSsidPass() 
{
  int posHeader = ConfigFlashParam.indexOf('$');
  int posTail = ConfigFlashParam.indexOf('*');
  if (posHeader == 0) 
  {

    int posComma1 = ConfigFlashParam.indexOf(',');
    int posComma2 = ConfigFlashParam.indexOf(',' , posComma1 + 1);
    int posComma3 = ConfigFlashParam.indexOf(',' , posComma2 + 1);
    int posComma4 = ConfigFlashParam.indexOf(',' , posComma3 + 1);

    passwordnew = ConfigFlashParam.substring((posComma2), posComma1 + 1);
    ssid = ConfigFlashParam.substring((posComma1), posHeader + 1);
    CoustomerID = ConfigFlashParam.substring((posComma3), posComma2 + 1);
    CountryCode = ConfigFlashParam.substring((posComma4), posComma3 + 1);
    TimeZone = ConfigFlashParam.substring((posTail), posComma4 + 1);
  }
  if (ConfigFlashParam != 0) 
  {
    int ssid_len = ssid.length() + 1;              //length of ssid string
    int pass_len = passwordnew.length() + 1;           //length of password string

    ssid_Sta[ssid_len];                       //ssid as char[]
    ssid.toCharArray(ssid_Sta, ssid_len);         //string to char array
    password_Sta[pass_len];                    //password as char[]
    passwordnew.toCharArray(password_Sta, pass_len);   //string to char array
    Serial.print("SSID STA : ");
    Serial.println(ssid_Sta);
    Serial.print("Password STA : ");
    Serial.println(password_Sta);
    Serial.print("CoustomerID : ");
    Serial.println(CoustomerID);
    Serial.print("CountryCode : ");
    Serial.println(CountryCode);
    Serial.print("TimeZone : ");
    Serial.println(TimeZone);
    Serial.println("");

    Serial.println("Configed Succefully...");

  }
}
//------------------------------TimeZone to int--------------------------------------------//
void timeZoneSet() 
{
  String testconvert = TimeZone.substring(3);
  GMTfloat = testconvert.toFloat();
  GMTfloat = 3600 * GMTfloat;
  GMTint = (int)GMTfloat;
}

//---------------------------Read File Containing SSID PASS---------------------------------//

void getSsidPassFlash() 
{
  Serial.println("Storing Config file in Flash");
  SerFlash.open("/storage/newtest.txt", FS_MODE_OPEN_READ);
  char buf[1024];
  buf[0] = '\0';  // Init buf in case readBytes doesn't actually do anything (and returns 0)
  size_t read_length = SerFlash.readBytes(buf, 1023);
  SerFlash.close();
  String s((const __FlashStringHelper*) buf);
  ConfigFlashParam = s;
  Serial.print("Config Flash Parameters are: "); Serial.println(ConfigFlashParam);
  CheckingSsidPass();
}

//---------------------------Create File Containing SSID PASS---------------------------------//

void StoreSsisPassFlash() 
{
  SerFlash.open("/storage/newtest.txt",
                FS_MODE_OPEN_CREATE(512, _FS_FILE_OPEN_FLAG_COMMIT));
  Serial.println("Creating and Writing file /storage/newtest.txt");
  SerFlash.print(ConfigFlashParam); //Save ssid and password
  SerFlash.close();
}

//---------------------------Delete File Containing SSID PASS---------------------------------//

void deleteSsidPassFlash() 
{
  SerFlash.del("/storage/newtest.txt");
  SerFlash.close();
  ConfigFlashParam = "";
  Serial.println("File deleted Sucessfully");
  digitalWrite(wifiLed,HIGH);
}
