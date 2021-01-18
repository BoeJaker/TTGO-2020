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
#include "config.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <WebServer.h>

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

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

// Your Wifi Network - Update the integer in the square brackets
#define wifi_ssid1 {"Galaxy Note"}
#define wifi_password1 {"adalovelace"}
#define wifi_ssid2 {"BTHub6-W7TF"}
#define wifi_password2 {"NbyCy4Q3RKhk"}

const char* serverName = "https://192.168.1.66:8000/";

// Your Screen Prefrences
int screenOnTime =  30000; //time before watch screen times out without user input
long timeUpdateInterval = 60000;
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
boolean charging = true;

// Touch
unsigned long lastTouchTime = 0;
int rapidTouchCount = 0;
boolean touchDetected = false;

// Menu
boolean return_to_menu= false;

//Coinmarketcap API
// CoinMarketCap's limit is "no more than 10 per minute"
// Make sure to factor in if you are requesting more than one coin.
unsigned long api_mtbs = 60000; //mean time between api requests
unsigned long api_due_time = 0;
int16_t x, y;

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

  //Check if the RTC clock matches, if not, use compile time
  ttgo->rtc->check();

  //Synchronize time to system time
  ttgo->rtc->syncToSystem();

  //Backlight Init
  ttgo->openBL(); // Turn on backlight

  //Touch Init
  struct point p; //Create dummy touch object
  attachInterrupt(TOUCH_IRQ, TOUCH_ISR, FALLING);
  readTouch(); //Initialise touch

  //Motion Init
  ttgo->bma->begin();

  //Motion Interrupt
  ttgo->bma->attachInterrupt();
  pinMode(BMA423_INT1, INPUT);
  attachInterrupt(BMA423_INT1, [] { }, RISING);

  //TFT Init
  tft = ttgo->tft;
  tft->setTextColor(TFT_GREEN);
  tft->fillRect(0, 0, 240, 135, TFT_BLACK);
  tft->pushImage(0, 0, btc_icon2_width, btc_icon2_height, btc_icon2);

  wifiConnect();
  server_setup();
  setup_ota();

  lastTouchTime = millis();
}

void loop()
{
  MainLoop();
  deviceSleep();

}
void IRAM_ATTR AXP202_VBUS_REMOVED_ISR(){
  charging = false;
}
void IRAM_ATTR AXP202_VBUS_CONNECT_ISR(){
  charging = true;
}


                                                                                                                                                                                                                                                                                                                                            
uint32_t targetTime = 0;

void MainLoop() {
//  initBLE();
boolean exit_main_menu = false;
while (exit_main_menu == false){
  switch (modeMenu()) { // Call modeMenu. The return is the desired app number
    case 0: // CLOCK
      while (lastTouchTime + screenOnTime > millis() & return_to_menu != true) {
        server_loop();
        if (targetTime < millis()) {
          targetTime = millis() + 1000;
          displayTime(ss == 0); // Call every second but only update time every minute
          signal_indicator();
        }
        home_button();
      }
      exit_main_menu = true;
      break;

      
    case 1: // CRYPTO TICKER
      tft->pushImage(0, 0, btc_icon2_width, btc_icon2_height, btc_icon2);
      while (lastTouchTime + screenOnTime > millis() & return_to_menu != true) {
        server_loop();
        crypto_ticker_app();
        home_button();
      }
      exit_main_menu = true;
      break;

      
    case 2: //WIFI SCANNER
      WiFi.mode(WIFI_STA);
      while (lastTouchTime + screenOnTime > millis() & return_to_menu != true) {
        server_loop();
        wifi_scanner();
        home_button();
      }
      exit_main_menu = true;
      break;

    case 3: //SETTINGS
//      while (lastTouchTime + screenOnTime > millis() & return_to_menu != true) {
        server_loop();
        request("Settings");
        home_button();
        delay(1000);
//      }
      break;

    case 4: //BATTERY HURETECS
      while (lastTouchTime + screenOnTime > millis() & return_to_menu != true) {
        server_loop();
        battery_app();
        home_button();
        delay(1000);
      }
      exit_main_menu = true;
      break;
      
    case 5: //PERIPHERAL DIAGNOSTICS
      while (lastTouchTime + screenOnTime > millis() & return_to_menu != true) {
        server_loop();
        diagnostics();
        home_button();
      }
      exit_main_menu = true;
      break;
      
    case 6: //SLEEP
        exit_main_menu = true;
        break;
      
    case 7: //BATCHELOR
        server_loop();
        digitalWrite(VIBE_PIN, HIGH);
        delay(5000);
        digitalWrite(VIBE_PIN, LOW);
        break;

     case 8: //LOCK
        server_loop();
        request("Lock");
        break;
        
     case 9: //LOCK
        server_loop();
        request("Git Upload");
        break;
     
     case 10: 
        server_loop();
        request("Auto Lock");
        break;
        
     case 11: 
        server_loop();
        request("Shutdown");
        break;
        
     case 12: 
        server_loop();
        request("Trade");
        break;
   }  
}
}
