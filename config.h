// WIFI
const char ssid[] = "*****";  //  your network SSID (name)
const char pass[] = "*****";       // your network password

static const char ntpServerName[] = "ntp.time.in.ua"; // NTP Server
unsigned int localPort = 8888;  // local port to listen for UDP packets

const int timeZone = 2; // time zone
const int synkInterval = 14400; // sync interval in seconds
