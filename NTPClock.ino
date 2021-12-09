//#define USE_WIFI_MANAGER
#ifdef ARDUINO_M5Stick_C
//#define ARDUINO_M5Stick_C_Plus // just in case for M5Stick C Plus
#endif

#define SEG7

#ifdef ST7789
#ifdef ARDUINO_M5Stick_C_Plus
#include <M5StickCPlus.h>
#else // !ARDUINO_M5Stick_C_Plus
#ifdef ARDUINO_M5Stick_C
#include <M5StickC.h>
#else // !ARDUINO_M5Stick_C
#ifdef ARDUINO_M5STACK_Core2
#include <M5Core2.h>
#else // !ARDUINO_M5STACK_Core2
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#endif // !ARDUINO_M5STACK_Core2
#endif // !ARDUINO_M5Stick_C
#endif // !ARDUINO_M5Stick_C_Plus
#endif // ST7789

#if defined(ARDUINO_M5Stick_C) || defined(ARDUINO_M5Stick_C_Plus) || defined(ARDUINO_M5STACK_Core2)
#define LCD M5.Lcd
#else
#undef LCD
#define LCD tft
#endif

#include <WiFi.h>
#include <Ticker.h>
#include "auth.h"

Ticker ticker;

#ifdef SEG7
const int sck = 12;
const int sdi = 14;
const int latch = 15;
const int oe = 13;
const int cds = 36; // VP=36, VN=39
const int ledOE = 0;
#endif
#ifdef ST7789
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
#define PADX 5
#endif

bool NTPSyncFailed = false;

#ifdef SEG7
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
#endif

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
#ifdef SEG7
  ticker.attach(0.2, tick);
#endif
}

void connectWiFi()
{
  //set led pin as output
  //  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
#ifdef SEG7
  ticker.attach(0.6, tick);
#endif
#ifdef ST7789
  LCD.fillScreen(TFT_BLUE);
  LCD.drawString("Connect to:", PADX, 0, 4);
  LCD.drawString("AutoConnectAP", PADX, LCD.fontHeight(4), 4);
#endif

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

#ifdef SEG7
  ticker.detach();
#endif
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
#ifdef SEG7
    ticker.attach(0.6, tick);
#endif
#ifdef ST7789
    LCD.fillScreen(TFT_BLUE);
    LCD.setTextPadding(PADX); // seems no effect by this line.
    LCD.drawString("Connecting ...", PADX, LCD.height() / 2 - LCD.fontHeight(4) / 2, 4);
#endif
  }
  for (i = 0 ; WiFi.status() != WL_CONNECTED && i < waitTimeOut ; i += waitTime) {
    delay(waitTime);
  }
  // if you are connected, print your MAC address:
  {
    byte mac[6];

    WiFi.macAddress(mac);
    Serial.print("MAC: ");
    Serial.print(mac[0],HEX);
    Serial.print(":");
    Serial.print(mac[1],HEX);
    Serial.print(":");
    Serial.print(mac[2],HEX);
    Serial.print(":");
    Serial.print(mac[3],HEX);
    Serial.print(":");
    Serial.print(mac[4],HEX);
    Serial.print(":");
    Serial.println(mac[5],HEX);
  }
  if (firstTime) {
#ifdef SEG7
    ticker.detach();
#endif
#ifdef ST7789
    LCD.fillScreen(TFT_BLACK);
#endif
  }
  firstTime = false;
}
#endif

#ifdef SEG7
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
#endif

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
#ifdef SEG7
  Serial.print("Brightness = ");
  Serial.print(b);
  Serial.print(" (");
  Serial.print(map(b, 0, 4095, 5, 255));
  Serial.println(")");
#endif
}

void setup() {

  configTime(9 * 3600, 0, nullptr); // set time zone as JST-9
  // The above is performed without network connection.

#ifdef SEG7
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
#endif

#ifdef ST7789
#if defined(ARDUINO_M5Stick_C) || defined(ARDUINO_M5Stick_C_Plus) || defined(ARDUINO_M5STACK_Core2)
  // initialize the M5StickC object
  M5.begin();
  delay(500);
#else
  // initialize TFT screen
  tft.init(); // equivalent to tft.begin();
  Serial.begin(115200);
  Serial.println("");
#endif
  LCD.setRotation(1); // set it to 1 or 3 for landscape resolution
  LCD.fillScreen(TFT_BLUE);
  LCD.setTextColor(TFT_WHITE);
  LCD.setTextPadding(PADX); // seems no effect by this line.
  LCD.drawString("Connecting ...",
		 PADX, LCD.height() / 2 - LCD.fontHeight(4) / 2, 4);
#endif
  
  connectWiFi();

#ifdef SEG7
  // Display "Cnd"
  digitalWrite(latch, 0);
  shiftOut(sdi, sck, LSBFIRST, B01111010);
  shiftOut(sdi, sck, LSBFIRST, 1);
  shiftOut(sdi, sck, LSBFIRST, B10011101);
  digitalWrite(latch, 1);
  delay(1000);
#endif

  NTPSync();
}

void loop() {
#ifdef SEG7
  unsigned br;
#endif
  time_t t = 0;
  struct tm timeInfo;
  static int prevMin = -1;

  NTPSync();
  t = time(NULL);
  localtime_r(&t, &timeInfo);

#ifdef SEG7
  // control the 7seg brightness
  br = analogRead(cds);
  // Serial.println(String(br) + " " + map(br, 0, 4095, 255, 0));

  ledcWrite(ledOE, map(br, 0, 4095, 256, 0)); // 256 to black out.
#endif

  if (timeInfo.tm_min != prevMin) {
    prevMin = timeInfo.tm_min;

#ifdef SEG7
    digitalWrite(latch, 0);
    shiftOut(sdi, sck, LSBFIRST, digits[prevMin % 10] | (NTPSyncFailed ? 1 : 0));
    shiftOut(sdi, sck, LSBFIRST, digits[prevMin / 10]); 
    shiftOut(sdi, sck, LSBFIRST, digits[timeInfo.tm_hour % 12]);
    digitalWrite(latch, 1);
#endif
#ifdef ST7789
#define TFTFONT 7
    char buf[6]; // 6 for "xx:xx"
    LCD.fillScreen(TFT_BLACK);
    snprintf(buf, 6, "%d:%02d", timeInfo.tm_hour, timeInfo.tm_min);
    LCD.drawString(buf, LCD.width() / 2 - LCD.textWidth(buf, TFTFONT) / 2,
		   LCD.height() / 2 - LCD.fontHeight(TFTFONT) / 2, TFTFONT);
#endif
  }

  delay(1000);
}
