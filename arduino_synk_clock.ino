#include <TimeLib.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Lib.h>
#include <Ressources/Font6x8.h>
#include <Ressources/CodePage437_8x16.h>
#include <Ressources/CodePage437_9x16.h>
#include "config.h"

const int redPin = 19;
const int greenPin = 19;
const int bluePin = 19;
const int hsyncPin = 5;
const int vsyncPin = 0;


char month_name[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

WiFiUDP Udp;

time_t getNtpTime();
void clockDisplaySerial();
void clockDisplayVGA();
void printDigits(int digits);
void printDigitsVGA(int digits);
void sendNTPpacket(IPAddress &address);

VGA3Bit vga;

void setup() {
  Serial.begin(115200);

  // VGA
  vga.setFrameBufferCount(2);
  Mode myMode = vga.MODE320x240.custom(80, 60);

  vga.init(myMode, redPin, greenPin, bluePin, hsyncPin, vsyncPin);

  vga.setFont(CodePage437_8x16);
  vga.backColor = vga.RGB(0);

  // Wifi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());

  // Sync
  Serial.println("Starting UDP");
  Udp.begin(localPort);

  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(synkInterval);
}

time_t prevDisplay = 0;

void loop() {
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) {
      prevDisplay = now();
      clockDisplaySerial();
      clockDisplayVGA();
    }
  }

  delay(500);
}

void clockDisplaySerial() {
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();
}

void clockDisplayVGA() {
  vga.clear();
  vga.setCursor(5, 30);

  printDigitsVGA(hour());
  vga.print(':');
  printDigitsVGA(minute());
  vga.print(':');
  printDigitsVGA(second());

  vga.setCursor(13, 15);
  printDigitsVGA(day());
  vga.print(" ");
  vga.print(month_name[month() - 1]);

  vga.show();
}

void printDigits(int digits) {
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void printDigitsVGA(int digits) {
  if (digits < 10)
    vga.print('0');
  vga.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48;      // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE];  //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  IPAddress ntpServerIP;  // NTP server's ip address

  while (Udp.parsePacket() > 0)
    ;  // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0;  // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123);  //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
