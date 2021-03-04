//#define USE_WIFI_MANAGER

#include <WiFi.h>
#include <Ticker.h>
Ticker ticker;

const int sck = 12;
const int sdi = 14;
const int latch = 15;
const int oe = 13;
const int cds = 36; // VP=36, VN=39
const int ledOE = 0;
bool NTPSyncFailed = false;

void tick()
{
  static bool f = false;

  digitalWrite(latch, 0);
  if (f) {
    shiftOut(sdi, sck, LSBFIRST, B10010010);
    shiftOut(sdi, sck, LSBFIRST, B10010010);
    shiftOut(sdi, sck, LSBFIRST, B10010010);
    f = false;
  }
  else {
    shiftOut(sdi, sck, LSBFIRST, 0);
    shiftOut(sdi, sck, LSBFIRST, 0);
    shiftOut(sdi, sck, LSBFIRST, 0);
    f = true;
  }
  digitalWrite(latch, 1);
}

static const time_t recentPastTime = 1500000000UL; // 2017/7/14 2:40:00 JST

#ifdef USE_WIFI_MANAGER
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#endif

#ifdef USE_WIFI_MANAGER
//WiFiManager
//Local intialization. Once its business is done, there is no need to keep it around
WiFiManager wifiManager;


//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

void connectWiFi()
{
  //set led pin as output
  //  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect()) {
    // do something here.
  }

  ticker.detach();
  //keep LED on -> off
  //digitalWrite(BUILTIN_LED, HIGH);
//  pinMode(BUILTIN_LED, INPUT);
}
#else //!USE_WIFI_MANAGER
void connectWiFi()
{
  const unsigned waitTime = 500, waitTimeOut = 60000;
  unsigned i;
  static bool firstTime = true;
  
  WiFi.begin(WIFIAP, WIFIPW);
  if (firstTime) {
    // start ticker with 0.5 because we start in AP mode and try to connect
    ticker.attach(0.6, tick);
  }
  for (i = 0 ; WiFi.status() != WL_CONNECTED && i < waitTimeOut ; i += waitTime) {
    delay(waitTime);
  }
  if (firstTime) {
    ticker.detach();
  }
  firstTime = false;
}
#endif


const byte digits[] = {
  B11111100, // 0
  B01100000, // 1
  B11011010, // 2
  B11110010, // 3
  B01100110, // 4
  B10110110, // 5
  B10111110, // 6
  B11100000, // 7
  B11111110, // 8
  B11110110, // 9
  B11101110, // A
//B10111010, // 10
  B01101100, // 11
  B11101110, // A
  B00111110, // b
  B10011100, // C
  B01111010, // d
  B10011110, // E
  B10001110, // F
};

void NTPSync()
{
  const unsigned long NTP_INTERVAL = (24 * 60 * 60 * 1000);
  static unsigned long lastNTPconfiguration = 0;

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (WiFi.status() == WL_CONNECTED) {
    // check NTP
    if (!lastNTPconfiguration || NTP_INTERVAL < millis() - lastNTPconfiguration) {
      configTime(9 * 3600, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");
      lastNTPconfiguration = millis();
      for (time_t t = 0 ; t == 0 ; t = time(NULL)); // wait for NTP to synchronize
      NTPSyncFailed = false;
    }
  }
  else {
    NTPSyncFailed = true;
  }
}

void printBrightness(unsigned b)
{
  Serial.print("Brightness = ");
  Serial.print(b);
  Serial.print(" (");
  Serial.print(map(b, 0, 4095, 5, 255));
  Serial.println(")");
}

void setup() {
  Serial.begin(115200);
  Serial.println("");

  configTime(9 * 3600, 0, nullptr); // set time zone as JST-9
  // The above is performed without network connection.

  pinMode(latch, OUTPUT);
  pinMode(sck, OUTPUT);
  pinMode(sdi, OUTPUT);
  pinMode(oe, OUTPUT);
  pinMode(cds, INPUT);

  // turn 7segs all on
  digitalWrite(latch, 0);
  shiftOut(sdi, sck, LSBFIRST, B11111111);
  shiftOut(sdi, sck, LSBFIRST, B11111111);
  shiftOut(sdi, sck, LSBFIRST, B11111111);
  digitalWrite(latch, 1);
  delay(1000);
  
  ledcSetup(ledOE, 12800, 8); // chan 0, freq = 12800, bitlength = 8
  ledcAttachPin(oe, ledOE); // attach the channel to LED chan 0, set above

  // auto connection

  // Display "C..."
  digitalWrite(latch, 0);
  shiftOut(sdi, sck, LSBFIRST, 1);
  shiftOut(sdi, sck, LSBFIRST, 1);
  shiftOut(sdi, sck, LSBFIRST, B10011101);
  digitalWrite(latch, 1);
  
  connectWiFi();

  // Display "Cnd"
  digitalWrite(latch, 0);
  shiftOut(sdi, sck, LSBFIRST, B01111010);
  shiftOut(sdi, sck, LSBFIRST, 1);
  shiftOut(sdi, sck, LSBFIRST, B10011101);
  digitalWrite(latch, 1);
  delay(1000);

  NTPSync();
}

void loop() {
  unsigned br;
  time_t t = 0;
  struct tm timeInfo;
  static int prevMin = -1;

  NTPSync();
  t = time(NULL);
  localtime_r(&t, &timeInfo);

  // control the 7seg brightness
  br = analogRead(cds);
  // Serial.println(String(br) + " " + map(br, 0, 4095, 255, 0));

  ledcWrite(ledOE, map(br, 0, 4095, 256, 0)); // 256 to black out.
  // change 256 to 255 if it should display time in darkness.

  if (timeInfo.tm_min != prevMin) {
    prevMin = timeInfo.tm_min;

    digitalWrite(latch, 0);
    shiftOut(sdi, sck, LSBFIRST, digits[prevMin % 10] | (NTPSyncFailed ? 1 : 0));
    shiftOut(sdi, sck, LSBFIRST, digits[prevMin / 10]); 
    shiftOut(sdi, sck, LSBFIRST, digits[timeInfo.tm_hour % 12]);
    digitalWrite(latch, 1);
  }

  delay(1000);
}
