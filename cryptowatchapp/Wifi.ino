#include <WiFi.h>

  String ssidList[] = {"BTHub6-W7TF","Access Point","Galaxy Note"};
  String passwordList[] = {"NbyCy4Q3RKhk", "NbyCy4Q3RKhk", "adalovelace"};

  String ssid = "BTHub6-W7TF";
  String password = "NbyCy4Q3RKhk";

void wifiConnect(){

  char ssid0[60];
  char password0[60];
  
  ssidList[0].toCharArray(ssid0, ssidList[0].length() + 1);
  passwordList[0].toCharArray(password0, passwordList[0].length() + 1);
  
  if (WiFi.status() != WL_CONNECTED) {
    tft->drawString("Connecting to ", 150, 10, 2);
    tft->drawString(ssid0, 150, 25, 2);

    WiFi.begin(ssid0, password0 );
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED) {
      if (counter >= 5000){
        WiFi.mode(WIFI_OFF);
        WiFi.begin(wifi_ssid2, wifi_ssid2);
        tft->fillRect(150, 25, 100, 40, TFT_BLACK);
        tft->drawString("Connecting to ", 150, 10, 2);
        tft->drawString(wifi_ssid2, 150, 25, 2);
        counter=-1;
      }
   delay(1);
   counter+=1;
   }
   if (WiFi.status() == WL_CONNECTED) {
     tft->fillRect(150, 10, 100, 40, TFT_BLACK);
     getInternetTime();
  }
}  
}
