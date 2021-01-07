/*
 *  This sketch sends random data over UDP on a ESP32 device
 *
 */
#include <WiFi.h>
#include <WiFiUdp.h>

// WiFi network name and password:
const char * networkName = "I-Tech-Router";
const char * networkPswd = "0512303189T";

//IP address to send UDP data to:
// either use the ip address of the server or 
// a network broadcast address
const char * udpAddress = "192.168.10.255";
const int udpPort = 3333;

//Are we currently connected?
boolean connected = false;
char packetBuffer[255]; //buffer to hold incoming packet
char  ReplyBuffer[] = "acknowledged";       // a string to send back
//The udp library class
WiFiUDP udp;

void setup(){
  // Initilize hardware serial:
  Serial.begin(115200);
  
  //Connect to the WiFi network
  connectToWiFi(networkName, networkPswd);
}
void loop(){
  //only send data when connected
  if(connected){
    int packetSize = udp.parsePacket();

    if (packetSize) {

    Serial.print("Received packet of size ");

    Serial.println(packetSize);

    Serial.print("From ");

    IPAddress remoteIp = udp.remoteIP();

    Serial.print(remoteIp);

    Serial.print(", port ");

    Serial.println(udp.remotePort());

    // read the packet into packetBufffer

    int len = udp.read(packetBuffer, 255);

    if (len > 0) {

      packetBuffer[len] = 0;

    }

    Serial.println("Contents:");

    Serial.println(packetBuffer);

    // send a reply, to the IP address and port that sent us the packet we received

        //Send a packet
    udp.beginPacket(udp.remoteIP(),udp.remotePort());
    udp.print(ReplyBuffer);
    udp.endPacket();
  }
    udp.beginPacket(udpAddress,udpPort);
    udp.print("Casting");
    udp.endPacket();
  }
  //Wait for 1 second
  delay(1000);
}

void connectToWiFi(const char * ssid, const char * pwd){
  Serial.println("Connecting to WiFi network: " + String(ssid));

  // delete old config
  WiFi.disconnect(true);
  //register event handler
  WiFi.onEvent(WiFiEvent);
  
  //Initiate connection
  WiFi.begin(ssid, pwd);

  Serial.println("Waiting for WIFI connection...");
}

//wifi event handler
void WiFiEvent(WiFiEvent_t event){
    switch(event) {
      case SYSTEM_EVENT_STA_GOT_IP:
          //When connected set 
          Serial.print("WiFi connected! IP address: ");
          Serial.println(WiFi.localIP());  
          //initializes the UDP state
          //This initializes the transfer buffer
          udp.begin(WiFi.localIP(),udpPort);
          connected = true;
          break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
          Serial.println("WiFi lost connection");
          connected = false;
          break;
      default: break;
    }
}
