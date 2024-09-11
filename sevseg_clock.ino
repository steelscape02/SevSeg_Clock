#include "SevSeg.h"
#include "SPI.h"
#include "WiFi101.h"
#include "secrets.h"
#include "WiFiUdp.h"
#include "string.h"

char ssid[] = SSID;
char pass[] = PASS;
int status = WL_IDLE_STATUS;

unsigned int uPort = 2390;
IPAddress timeServer(129,6,15,28);

const int PACKET_SIZE = 48;

byte packetBuffer[PACKET_SIZE];

WiFiUDP Udp;

SevSeg sevseg; //Initiate a seven segment controller object


void setup() {
    pinMode(LED_BUILTIN,OUTPUT);
    while(status != WL_CONNECTED){
      digitalWrite(LED_BUILTIN,HIGH);
      status = WiFi.begin(ssid,pass);
      delay(5000);
      digitalWrite(LED_BUILTIN,LOW);
      delay(5000);
    }
    digitalWrite(LED_BUILTIN,HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN,LOW);

    Udp.begin(uPort);

    byte numDigits = 4;  
    byte digitPins[] = {0, 1, 2, 3};
    byte segmentPins[] = {4, 5, 6, 7, 8, 9, 10, 11};
    bool resistorsOnSegments = 0; 
    // variable above indicates that 4 resistors were placed on the digit pins.
    // set variable to 1 if you want to use 8 resistors on the segment pins.
    sevseg.begin(COMMON_CATHODE, numDigits, digitPins, segmentPins, resistorsOnSegments,true,false,false);
    sevseg.setBrightness(80);
}
int time = 0;
int lastTime = 0;
void loop() {
    time = getTime();
    if(time != NULL){lastTime = time;}
    updateDisplay(time);
}

void updateDisplay(int time){
  if(time == NULL){time = lastTime;}
  sevseg.setNumber(time, 2);
  int start = millis();
  int end = millis();
  while((end - start) < 5000){
    sevseg.refreshDisplay(); // Must run repeatedly
    end = millis();
  }
  sevseg.setNumber(time, 2);
}

int getTime(){
  int hour;
  int minute;
  sendNTPpacket(timeServer);
  //delay(1000);
  sevseg.setNumber(time, 2);
  int start = millis();
  int end = millis();
  while((end - start) < 1200){
    sevseg.refreshDisplay(); // Must run repeatedly
    end = millis();
  }
  if(Udp.parsePacket()){
    Udp.read(packetBuffer,PACKET_SIZE);
    //get the time in a super fun code block
    unsigned long highWord = word(packetBuffer[40],packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42],packetBuffer[43]);
    unsigned long sec1900 = highWord << 16 | lowWord;
    const unsigned long seventy = 2208988800UL;
    unsigned long epoch = sec1900 - seventy;
    epoch -= 21600; //mst offset
    
    hour = (epoch  % 86400L) / 3600;
    minute = (epoch  % 3600) / 60;
  }
  // wait ten seconds before asking for the time again
  
  int time= (hour * 100) + minute;
  return time;
}

unsigned long sendNTPpacket(IPAddress& address)
{
  memset(packetBuffer, 0, PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, PACKET_SIZE);
  Udp.endPacket();
}
