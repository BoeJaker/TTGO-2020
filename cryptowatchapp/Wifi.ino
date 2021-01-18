#include <WiFi.h>

  String ssidList[] = {"BTHub6-W7TF","Access Point","Galaxy Note"};
  String passwordList[] = {"NbyCy4Q3RKhk", "NbyCy4Q3RKhk", "adalovelace"};
  int limit = sizeof(ssidList);
  
void wifiConnect(){
  int s = 0;
  int counter = 10000;
  if (WiFi.status() != WL_CONNECTED) {
    while (WiFi.status() != WL_CONNECTED) {
      if (counter >= 10000){
        char ssid0[60];
        char password0[60];
        ssidList[s].toCharArray(ssid0, ssidList[s].length() + 1);
        passwordList[s].toCharArray(password0, passwordList[s].length() + 1);
        
        WiFi.begin(ssid0, password0);
        
        tft->fillRect(150, 25, 100, 40, TFT_BLACK);
        tft->drawString("Connecting to ", 150, 10, 2);
        tft->drawString(ssid0, 150, 25, 2);
        
        s += 1;
        if (s >= limit){
          s = 0;
        }
        counter=0;
      }
      delay(1);
      counter+=1;
   }   
  }
  if (WiFi.status() == WL_CONNECTED) {
    tft->fillRect(150, 0, 100, 80, TFT_BLACK);
    getInternetTime(); //needs to be moved to mail loop
  }
}
void signal_indicator(){
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
}

void wifi_scanner()
{
    if (millis() - lastTimeUpdate > timeUpdateInterval) {
    ttgo->tft->setTextSize(1);
    tft->setTextColor(TFT_GREEN);
    Serial.println("scan start");

    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        tft->drawString("Networks Found", 60, 1, 2);
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
            if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN){
              tft->setTextColor(TFT_RED);
            }
            tft->fillRect(0, 20*(i+1), 240, 18, TFT_BLACK);
            tft->drawString(WiFi.SSID(i)+" "+String(WiFi.RSSI(i))+" "+String((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*"), 10, 20*(i+1), 2);
            tft->drawLine(0,20*(i+1)+18,240+int(WiFi.RSSI(i)),20*(i+1)+18,TFT_WHITE);
            tft->setTextColor(TFT_GREEN);
            delay(10);
            
        }
    }
    Serial.println("");

    // Wait a bit before scanning again
    }
}
