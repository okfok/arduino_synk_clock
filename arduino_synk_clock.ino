// RTC demo for ESP32, that includes TZ and DST adjustments
// Get the POSIX style TZ format string from  https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

#include <WiFi.h>
#include "time.h"
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


// void clockDisplaySerial();
// void clockDisplayVGA();
// void printDigits(int digits);
// void printDigitsVGA(int digits);
// time_t getNtpTime();
// void sendNTPpacket(IPAddress &address);

VGA3Bit vga;


void setTimezone(String timezone){
  Serial.printf("  Setting Timezone to %s\n",timezone.c_str());
  setenv("TZ",timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

void initTime(String timezone){
  struct tm timeinfo;

  Serial.println("Setting up time");
  configTime(0, 0, ntpServerName);    // First connect to NTP server, with 0 TZ offset
  if(!getLocalTime(&timeinfo)){
    Serial.println("  Failed to obtain time");
    return;
  }
  Serial.println("  Got the time from NTP");
  // Now we can set the real timezone
  setTimezone(timezone);
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time 1");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
}


void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst){
  struct tm tm;

  tm.tm_year = yr - 1900;   // Set date
  tm.tm_mon = month-1;
  tm.tm_mday = mday;
  tm.tm_hour = hr;      // Set time
  tm.tm_min = minute;
  tm.tm_sec = sec;
  tm.tm_isdst = isDst;  // 1 or 0
  time_t t = mktime(&tm);
  Serial.printf("Setting time: %s", asctime(&tm));
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
}

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

  // Serial.println("waiting for sync");
  // setSyncProvider(getNtpTime);
  // setSyncInterval(synkInterval);

  initTime("EET-2EEST,M3.5.0/3,M10.5.0/4");   // Set for Europe/Kiev
}

time_t prevDisplay = 0;

void loop() {
  // if (timeStatus() != timeNotSet) {
  //   if (now() != prevDisplay) {
  //     prevDisplay = now();
  //     clockDisplaySerial();
  //     clockDisplayVGA();
  //   }
  // }

  delay(500);
  
  printLocalTime();
}
/*
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

//-------- NTP code ----------


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

*/