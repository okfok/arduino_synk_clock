// RTC demo for ESP32, that includes TZ and DST adjustments
// Get the POSIX style TZ format string from  https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

#include <WiFi.h>
#include "esp_sntp.h"
// #include "time.h"
#include <ESP32Lib.h>
// #include <Ressources/Font6x8.h>
#include <Ressources/CodePage437_8x16.h>
// #include "font_scripts/CodePage437_40x80.h"
#include "config.h"

const int redPin = 19;
const int greenPin = 19;
const int bluePin = 19;
const int hsyncPin = 5;
const int vsyncPin = 0;


const int scale = 1;

// char month_name[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

VGA3Bit vga;

void printLocalTime(struct tm timeinfo){
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
  // Serial.println(sntp_get_sync_status());
}

void printLocalTimeVGA(struct tm timeinfo){
  char time[9];
  strftime(time, 9, "%H:%M:%S", &timeinfo);
  char date[7];
  strftime(date, 7, "%d %B", &timeinfo);

  date[6] = 0;


  vga.clear();


  vga.setFont(CodePage437_8x16);

  
  vga.setCursor(13 * scale, 15 * scale);
  vga.print(date);

  vga.setCursor(5 * scale, 30 * scale);
  vga.print(time);



  vga.show();

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

void call(timeval*){
  Serial.print("CAlled");
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // VGA
  vga.setFrameBufferCount(2);
  // Mode myMode = vga.MODE320x240.custom(80, 60);
  Mode myMode = vga.MODE320x240.custom(80 * scale, 60 * scale);

  vga.init(myMode, redPin, greenPin, bluePin, hsyncPin, vsyncPin);

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

  // NTP
  struct tm timeinfo;

  Serial.println("Setting up time");
  configTime(0, 0, ntpServerName);    // First connect to NTP server, with 0 TZ offset
  if(!getLocalTime(&timeinfo)){
    Serial.println("  Failed to obtain time");
    // return;
  }

  Serial.println("  Got the time from NTP");
  // Now we can set the real timezone
  Serial.printf("  Setting Timezone to %s\n", timezone);
  setenv("TZ", timezone, 1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();

  // Serial.println("waiting for sync");
  // setSyncProvider(getNtpTime);
  // setSyncInterval(synkInterval);

  sntp_set_sync_interval(synkInterval * 1000);
  // sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
  // esp_sntp_setservername(0, ntpServerName);

  // Serial.print("Enabled: ");
  // Serial.println(esp_sntp_enabled());

  // esp_sntp_init();

  Serial.print("Restart: ");
  Serial.println(sntp_restart());

  // setTime(2024, 3, 31, 8, 0, 0, 0); 
  
}


void loop() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
      printLocalTime(timeinfo);
      printLocalTimeVGA(timeinfo);
      // clockDisplayVGA(timeinfo);
  } else {
    Serial.println("Failed to obtain time!");
  }

  delay(500);
  
}
