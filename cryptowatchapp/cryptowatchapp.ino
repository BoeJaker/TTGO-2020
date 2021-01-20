/**
   A framework for the TTGO watch enabling use of wifi, bluetooth, touch, gyro, accelerometer, PEK inturupt key

   Base-level features:
   Clock - updates via USB, WIFI or Bluetooth
   Step Counter
   Webserver
   GUI Generation
   Power manager - charge current adjustment & charge monitoring
   Raise or touch to wake
   
   Apps:
   WatchFace
   Crypto Ticker

   TODO
   Vibrate on low phone batt
   KeyStore
   Docked mode when charging
   NES Emulator
   Figure out user activity via time, movement and APs
   Vibrate if laptop battery low
   Pass PC notifications to watch
 **/

//---------------------------------------//
//Dependancies
#include "config.h" //Configuration for Twatch model

//Wifi 
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <WebServer.h>

//OTA Updates
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//Crpyto App
#include "CoinMarketCapApi.h"

//Images
#include "alert.h"
#include "info.h"
#include "btc_icon.h"
#include "btc_icon2.h"


//---------------------------------------//
//Definitions

//Pin Definitions
#define TOUCH_IRQ 38
#define CHARGE_PIN 35
#define VIBE_PIN 4
#define AXP_IRQ 35
#define BMA_IRQ 39
#define CLOCK_IRQ 37

//TFT Definitions
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240

//TFT Color Definitions
#define TFT_AQUA  0x04FF
#define TFT_GRAY  0x8410
#define ERROR_COLOR 0xF000
#define TEXT_COLOR 0x4F40
#define CLOCK_COLOR 0x4F40


//---------------------------------------//
//Config Variables & Definitions

// Your Coinmarketcap API Key:
#define APIKEY "fe231f8e-6d40-49c8-a22b-231123a7543a"

//Ensure the server has a staatic IP or you will have to change the IP every time the server is assigned a new address
const char* serverName = "https://192.168.1.66:8000/";

// Your Screen Prefrences
const int default_screenOnTime = 30000; // This value is used to reset the screenontime to default if it is changed
int screenOnTime = default_screenOnTime; // time before watch screen times out without user input
const long timeUpdateInterval = 60000; //time before a new request is made to crypto API
long lastTimeUpdate = millis() - timeUpdateInterval;


//---------------------------------------//
//Device Definitions
TFT_eSPI *tft = nullptr;
TTGOClass *ttgo = nullptr;
AXP20X_Class *power;
bool irq = false;

//timekeeping stuff
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -5 * 3600;
const int daylightOffset_sec = 0;
uint8_t hh, mm, ss, mmonth, dday; // H, M, S variables
uint16_t yyear; // Year is 16 bit int

//RTC
RTC_DATA_ATTR time_t now;
RTC_DATA_ATTR uint64_t Mics = 0;
RTC_DATA_ATTR struct tm *timeinfo;

//WIFI
float RSSI = 0.0;

//---------------------------------------//
//Variable Declarations

//Charging
boolean charging = true; //Non Functional

// Touch
unsigned long lastTouchTime = 0;
int rapidTouchCount = 0;
boolean touchDetected = false;

// Menu
boolean return_to_menu= false;
int menu_pos = 1;
//Coinmarketcap API
// CoinMarketCap's limit is "no more than 10 per minute"
// Make sure to factor in if you are requesting more than one coin.
unsigned long api_mtbs = 60000; //mean time between api requests
unsigned long api_due_time = 0;
int16_t x, y;
uint32_t targetTime = 0; //Clock interface update counter


//Crypto
String ticker_list[] {"BTC", "ETH", "1INCH", "UNI", "AAVE", "DOT", "LINK", "COMP", "REN", "GRT", "BAL", "KSM", "SNX", "EGLD", "ADA", "POLS", "CRV", "CHSB", "VET", "BONDLY", "ENJ", "EWT"};
byte ticker_len = 21;
byte ticker_pos = 0;
byte update_ticker = true;


//---------------------------------------//
//Function Pre-Build (for header file)

void wifiConnect();
void initTouch();
void IRAM_ATTR TOUCH_ISR();
void IRAM_ATTR AXP202_VBUS_REMOVED_ISR();
void IRAM_ATTR AXP202_VBUS_CONNECT_ISR();
struct point readTouch();


//---------------------------------------//
//Structures
struct point
{
  int xPos;
  int yPos;
};

//API Client setup
WiFiClientSecure client; // Coin Market Cap Client
CoinMarketCapApi api(client, APIKEY);



//---------------------------------------//
//Begin setup

void setup()
{
  digitalWrite(VIBE_PIN, HIGH);
  digitalWrite(VIBE_PIN, LOW);

  pinMode(AXP202_INT, INPUT_PULLUP);
  pinMode(CHARGE_PIN, INPUT_PULLUP);
  pinMode(VIBE_PIN, OUTPUT);

  Serial.begin(115200);
  Serial.println(F("Boot Ticker"));

  ttgo = TTGOClass::getWatch();
  ttgo->begin();

  //Power Init
  power = ttgo->power;

  //Clock Setup
  setup_clock();

  //Motion Setup
  setup_motion();

  //Touch Setup
  setup_touch();

  //display Setup
  setup_display();
  
  //Wifi Setup
  wifiConnect();
  
  //HTTP Server Setup
  server_setup();
  
  //OTA Updates Setup
  setup_ota();

  lastTouchTime = millis();
}

void loop()
{
  MainLoop(); //Main Menu
  deviceSleep(); //Sleep Function

}
                                                                                                                                                                                                                                                                                                                                            

void MainLoop() {
//  initBLE();
  MenuLoop();
}  
