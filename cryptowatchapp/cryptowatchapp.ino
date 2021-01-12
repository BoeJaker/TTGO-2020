//---------------------------------------//
//Dependancies
#include "config.h"
//#include "cryptowatch.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
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
#define CLOCK_COLOR Ox4F40

//---------------------------------------//
//Config Variables & Definitions

// Your Coinmarketcap API Key:
#define APIKEY "fe231f8e-6d40-49c8-a22b-231123a7543a"

// Your Wifi Network - Update the integer in the square brackets
#define wifi_ssid1 {"Galaxy Note"}
#define wifi_password1 {"adalovelace"}
#define wifi_ssid2 {"BTHub6-W7TF"}
#define wifi_password2 {"NbyCy4Q3RKhk"}
//#define DEBUG false

#define G_EVENT_VBUS_PLUGIN         _BV(0)
#define G_EVENT_VBUS_REMOVE         _BV(1)
#define G_EVENT_CHARGE_DONE         _BV(2)

#define G_EVENT_WIFI_SCAN_START     _BV(3)
#define G_EVENT_WIFI_SCAN_DONE      _BV(4)
#define G_EVENT_WIFI_CONNECTED      _BV(5)
#define G_EVENT_WIFI_BEGIN          _BV(6)
#define G_EVENT_WIFI_OFF            _BV(7)

enum {
    Q_EVENT_WIFI_SCAN_DONE,
    Q_EVENT_WIFI_CONNECT,
    Q_EVENT_BMA_INT,
    Q_EVENT_AXP_INT,
} ;

#define WATCH_FLAG_SLEEP_MODE   _BV(1)
#define WATCH_FLAG_SLEEP_EXIT   _BV(2)
#define WATCH_FLAG_BMA_IRQ      _BV(3)
#define WATCH_FLAG_AXP_IRQ      _BV(4)

QueueHandle_t g_event_queue_handle = NULL;
EventGroupHandle_t g_event_group = NULL;
EventGroupHandle_t isr_group = NULL;

// Your Screen Prefrences
int screenOnTime =  30000; //time before watch screen times out without user input
long timeUpdateInterval = 60000;
long lastTimeUpdate = millis()-timeUpdateInterval;
//---------------------------------------//
//if ( DEBUG == true ){
//  screenOnTime = 3000000;
//}

//Device Definitions
TFT_eSPI *tft = nullptr;
TTGOClass *ttgo = nullptr;
AXP20X_Class *power;
bool irq = false;

//timekeeping stuff
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -5 * 3600;
const int daylightOffset_sec = 0;

RTC_DATA_ATTR time_t now;
RTC_DATA_ATTR uint64_t Mics = 0;
RTC_DATA_ATTR struct tm *timeinfo;

//---------------------------------------//
//Variable Declarations

//Charging
boolean charging;

// Touch
unsigned long lastTouchTime = 0;
int rapidTouchCount = 0;
boolean touchDetected = false;

//Coinmarketcap API
// CoinMarketCap's limit is "no more than 10 per minute"
// Make sure to factor in if you are requesting more than one coin.
unsigned long api_mtbs = 60000; //mean time between api requests
unsigned long api_due_time = 0;
int16_t x, y;

//Crypto 
String ticker_list[] {"BTC","ETH","1INCH","UNI","AAVE","DOT","LINK","COMP","REN","GRT","BAL","KSM","SNX","EGLD","ADA","POLS","CRV","CHSB","VET","BONDLY","ENJ","EWT"};
byte ticker_len = 21;
byte ticker_pos = 0;
byte update_ticker = true;
//---------------------------------------//
//Function Pre-Build (for header file)

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

struct point readTouch()
{
  struct point p;
  int16_t x, y;
  if (ttgo->getTouch(x, y))
  {
    p.xPos = x;
    p.yPos = y;
  }
  else
  {
    p.xPos = -1;
    p.yPos = -1;
  }
  return p;
}

void mjd_set_timezone_gmt()
{
  ESP_LOGD(TAG, "%s()", __FUNCTION__);
  // @doc https://remotemonitoringsystems.ca/time-zone-abbreviations.php
  // @doc timezone UTC = UTC
  setenv("TZ", "GMT+0BST-1", 1);
  tzset();
}

//---------------------------------------//
//Begin setup

void setup()
{
    g_event_queue_handle = xQueueCreate(20, sizeof(uint8_t));
    g_event_group = xEventGroupCreate();
    isr_group = xEventGroupCreate();
    
    pinMode(AXP202_INT, INPUT_PULLUP);
    pinMode(CHARGE_PIN, INPUT_PULLUP);
    pinMode(VIBE_PIN, OUTPUT);
    
    Serial.begin(115200);
    Serial.println(F("Boot Ticker"));

    server_setup();
    
    digitalWrite(VIBE_PIN, HIGH);
    
    ttgo = TTGOClass::getWatch();
    
    digitalWrite(VIBE_PIN, LOW);
    
    ttgo->begin();
    ttgo->power->adc1Enable(AXP202_BATT_VOL_ADC1 | AXP202_BATT_CUR_ADC1 | AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1, AXP202_ON);
    ttgo->power->enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ | AXP202_CHARGING_FINISHED_IRQ, AXP202_ON);
    ttgo->power->clearIRQ();
    
    struct point p;
    attachInterrupt(TOUCH_IRQ, TOUCH_ISR, FALLING);
//    attachInterrupt(AXP202_VBUS_REMOVED_IRQ, AXP202_VBUS_REMOVED_ISR, FALLING);
//    attachInterrupt(AXP202_VBUS_CONNECT_IRQ, AXP202_VBUS_CONNECT_ISR, FALLING);
    ttgo->openBL();
    readTouch();
    ttgo->bma->begin();
    ttgo->bma->attachInterrupt();

    //Connection interrupted to the specified pin
    pinMode(BMA423_INT1, INPUT);
    attachInterrupt(BMA423_INT1, [] {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        EventBits_t  bits = xEventGroupGetBitsFromISR(isr_group);
        if (bits & WATCH_FLAG_SLEEP_MODE)
        {
            // Use an XEvent when waking from low energy sleep mode.
            xEventGroupSetBitsFromISR(isr_group, WATCH_FLAG_SLEEP_EXIT | WATCH_FLAG_BMA_IRQ, &xHigherPriorityTaskWoken);
            deviceSleep();
        } else
        {
            // Use the XQueue mechanism when we are already awake.
            uint8_t data = Q_EVENT_BMA_INT;
            xQueueSendFromISR(g_event_queue_handle, &data, &xHigherPriorityTaskWoken);
        }

        if (xHigherPriorityTaskWoken)
        {
            portYIELD_FROM_ISR ();
        }
    }, RISING);

    
    tft = ttgo->tft;
    power = ttgo->power;

    power->adc1Enable(AXP202_BATT_VOL_ADC1 | AXP202_BATT_CUR_ADC1 | AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1, AXP202_ON);
    power->enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ | AXP202_CHARGING_FINISHED_IRQ, AXP202_ON);
    power->clearIRQ();
    
    tft->setTextColor(TFT_GREEN);

//    uint16_t time = millis();
//    time = millis() - time;
//    tft->fillScreen(TFT_BLACK);
//    tft->setSwapBytes(true);
//    tft->pushImage(75, 75, btc_icon_width, btc_icon_height, btc_icon);
//    delay(3000);
    
//    // Startup
//    tft->fillScreen(TFT_BLACK);
//    Serial.print("Connecting to ");
//    tft->drawString("Connecting to ", 15, 10, 2);
//    Serial.println(wifi_ssid);
//    tft->drawString(wifi_ssid, 15, 25, 2);
//    tft->pushImage(200, 2, infoWidth, infoHeight, info);
//    delay(1000);
//
//    Serial.println("");
//    Serial.println("WiFi connected");
//    tft->setTextColor(TFT_GREEN);
//    tft->drawString("WiFi connected", 15, 40, 2);
//    tft->setTextColor(TFT_WHITE);
//    Serial.println("IP address: ");
//    Serial.println(WiFi.localIP());
//    delay(1000);
    tft->fillRect(0, 0, 240, 135, TFT_BLACK);

    tft->pushImage(0, 0, btc_icon2_width, btc_icon2_height, btc_icon2);
       
    lastTouchTime = millis();
}

void deviceSleep() {
  //re-enable touch wakeup
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_38, 0); //1 = High, 0 = Low
  esp_deep_sleep_start();
//  ttgo->powerOff();
}

void printTickerData(String ticker)
{
    Serial.println("---------------------------------");
    Serial.println("Getting ticker data for " + ticker);


    //For the new API, you can use the currency ID or abbreviated name, such as
    //Bitcoin, you can view the letter after Circulating Supply at https://coinmarketcap.com/, it is BTC

    CMCTickerResponse response = api.GetTickerInfo(ticker, "USD");
    if (response.error == "") {
//        Serial.print("ID: ");
//        Serial.println(response.id);
//        Serial.print("Name: ");
//        Serial.println(response.name);
//        Serial.print("Symbol: ");
//        Serial.println(response.symbol);

//        Serial.print("Rank: ");
//        Serial.println(response.cmc_rank);
//
//        Serial.print("Price: ");
//        Serial.println(response.price);
//
//        Serial.print("24h Volume: ");
//        Serial.println(response.volume_24h);
//        Serial.print("Market Cap: ");
//        Serial.println(response.market_cap);
//
//        Serial.print("Circulating Supply: ");
//        Serial.println(response.circulating_supply);
//        Serial.print("Total Supply: ");
//        Serial.println(response.total_supply);
//
//        Serial.print("Percent Change 1h: ");
//        Serial.println(response.percent_change_1h);
//        Serial.print("Percent Change 24h: ");
//        Serial.println(response.percent_change_24h);
//        Serial.print("Percent Change 7d: ");
//        Serial.println(response.percent_change_7d);
//        Serial.print("Last Updated: ");
//        Serial.println(response.last_updated);
        
        tft->fillRect(100, 11, 100, 20, TFT_BLACK); //Ticker pair
        tft->setTextColor(TFT_GRAY);
        tft->drawString(response.symbol, 100, 11, 2);
        String symbol = response.symbol;
        int len = symbol.length();
        int space = 100+(len*9);
        tft->drawString("/ USD", space, 11, 2);

        tft->setTextColor(TFT_WHITE);

        

        tft->fillRect(0, 120, 300, 38, TFT_BLACK); //price
        tft->fillRect(175, 30, 55, 20, TFT_BLACK); //rank

        tft->fillRect(105, 50, 130, 20, TFT_BLACK); //Volume
        tft->fillRect( 100, 65, 130, 20, TFT_BLACK); //Marketcap
        tft->fillRect( 95, 80, 130, 20, TFT_BLACK); //Delta
        tft->fillRect( 90, 95, 130, 20, TFT_BLACK); //Delta

        tft->setTextColor(TFT_YELLOW);

        if (response.percent_change_1h < 0) {
            tft->setTextColor(TFT_RED);
        }
        if (response.percent_change_1h > 0) {
            tft->setTextColor(TFT_GREEN);
        }


        String price = String(response.price);
        len = price.length()-1;
        space = 160-(len*18);
        tft->drawString(String(response.price).c_str(), space, 120, 6);

        tft->setTextColor(TFT_GRAY);
        tft->drawString("Rank:", 105, 30, 4);
        tft->drawString(String(response.cmc_rank).c_str(), 175, 30, 4);
        tft->drawString(String(response.volume_24h).c_str(), 105, 50, 2);
        tft->drawString(String(response.market_cap).c_str(), 100, 65, 2);

        tft->drawLine(11, 166, 229, 166, TFT_GRAY);
          
//        // hours change
//        tft->fillRect(10, 200, 210, 25, TFT_BLACK);
//        tft->setTextColor(TFT_YELLOW);
//
//        if (response.percent_change_1h < 0) {
//            tft->setTextColor(TFT_RED);
//        }
//        if (response.percent_change_1h > 0) {
//            tft->setTextColor(TFT_GREEN);
//        }
//        tft->drawString("% Delta 1h:", 11, 200, 4);
//        tft->drawString(String(response.percent_change_1h).c_str(), 156, 200, 4);
//        delay(1000);
//
//        // 24 hours change
//        tft->fillRect(95, 65, 145, 25, TFT_BLACK);
        tft->setTextColor(TFT_YELLOW);

        if (response.percent_change_24h < 0) {
            tft->setTextColor(TFT_RED);
        }
        if (response.percent_change_24h > 0) {
            tft->setTextColor(TFT_GREEN);
        }
        tft->drawString("% Delta 24h:", 95, 80, 2);
        tft->drawString(String(response.percent_change_24h).c_str(), 180, 80, 2);


        // 7d hours change
//        tft->fillRect(10, 200, 210, 25, TFT_BLACK);
        tft->setTextColor(TFT_YELLOW);

        if (response.percent_change_7d < 0) {
            tft->setTextColor(TFT_RED);
        }
        if (response.percent_change_7d > 0) {
            tft->setTextColor(TFT_GREEN);
        }
        tft->drawString("% Delta 7d:", 90, 95, 2);
        tft->drawString(String(response.percent_change_7d).c_str(), 165, 95, 2);

    } else {
        Serial.print("Error getting data: ");
        Serial.println(response.error);
        tft->fillRect(200, 2, 40, 32, TFT_BLACK); //wifi RSSI and alert
        tft->pushImage(203, 2, alertWidth, alertHeight, alert);
    }
    Serial.println("---------------------------------");


  update_ticker = false;
}

float RSSI = 0.0;
unsigned long la;

void loop()
{
    MainLoop();
    deviceSleep();
  
}
//void IRAM_ATTR AXP202_VBUS_REMOVED_ISR(){
//  charging = false;
//}
//void IRAM_ATTR AXP202_VBUS_CONNECT_ISR(){
//  charging = true;
//}
void IRAM_ATTR TOUCH_ISR()
{
  digitalWrite(VIBE_PIN, HIGH);
  digitalWrite(VIBE_PIN, LOW);
  Serial.println("Touch detected from interrupt");
  if (millis() - lastTouchTime < 200) {
    rapidTouchCount++;
    Serial.println("rapidTouchCount: " + String(rapidTouchCount));

    if (rapidTouchCount > 500) {
      #ifdef DEBUG
        Serial.println("**** Rapid touch shutdown registered ****");
        Serial.flush();
      #endif
      deviceSleep();
    }
  } else {
    rapidTouchCount = 0;
  }
  if(millis() - lastTouchTime > 200){
      touchDetected = true;

      lastTouchTime = millis();
  
      update_ticker = true;
      //Ticker Incrementing
      if (ticker_pos < ticker_len){
        ticker_pos +=  1;
      } else {
        ticker_pos = 0;
      }
   }
}
void MainLoop(){
  wifiConnect();  
  server_loop();
  while (lastTouchTime + screenOnTime > millis()) {
    // Update time
    if (millis()-lastTimeUpdate > timeUpdateInterval){
      drawTime(11,178,3);
      drawDate(11,208,2);
      ttgo->tft->setCursor(0, 0);
      ttgo->tft->setTextSize(0);
    }
    unsigned long timeNow = millis();
      if ((timeNow > api_due_time) | update_ticker)  {
        // int signal bars

        Serial.print(WiFi.RSSI());
        tft->fillRect(200, 0, 40, 30, TFT_BLACK); //wifi RSSI and alert


        int bars;
        RSSI = WiFi.RSSI();

        if (RSSI >= -55) {
            bars = 5;
            Serial.println(" 5 bars");  
        } else if (RSSI < -55 & RSSI >= -65) {
            bars = 4;
            Serial.println(" 4 bars");
        } else if (RSSI < -65 & RSSI >= -70) {
            bars = 3;
            Serial.println(" 3 bars");
        } else if (RSSI < -70 & RSSI >= -78) {
            bars = 2;
            Serial.println(" 2 bars");
        } else if (RSSI < -78 & RSSI >= -82) {
            bars = 1;
            Serial.println(" 1 bars");
        } else {
            bars = 0;
            Serial.println(" 0 bars");
        }

        // signal bars
        for (int b = 0; b <= bars; b++) {
            tft->fillRect(202 + (b * 6), 23 - (b * 4), 5, b * 4, TFT_GRAY);
        }

      
        Serial.println(ticker_list[ticker_pos]);
        printTickerData(ticker_list[ticker_pos]);
        api_due_time = timeNow + api_mtbs;

        if (power->isChargeing()){
           digitalWrite(VIBE_PIN, HIGH);
           tft->drawString(String("Charging"), 170, 168, 2);
           digitalWrite(VIBE_PIN, LOW);
        }else{
          digitalWrite(VIBE_PIN, HIGH);
          tft->fillRect( 170, 168, 120, 20, TFT_BLACK);
          digitalWrite(VIBE_PIN, LOW);
          tft->setTextColor(TFT_GREEN);
          tft->drawString(String(power->getBattPercentage())+"%", 170, 10, 2);
        }
    }
}
}
