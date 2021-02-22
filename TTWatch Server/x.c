
const int available_lines = 14;
String textarray[available_lines];
int char_per_line = 16;

//
//int RSSI = maclist[listcount][3]; //RSSI
//String payload = maclist[listcount][4]; //Payload
//String MAC = maclist[listcount][0]; //MAC Address
//int TTL = maclist[listcount][1]; //TTL
//
//struct rssi_threshold{
//  int threshold;
//  String tag;
//};
//  
//rssi_threshold threshA {-10, "0.5m"}
//rssi_threshold threshB {-20, "1m"}
//rssi_threshold threshC {-30, "2m"}
//rssi_threshold threshD {-40, "3m"}
//rssi_threshold threshE {-50, "4m"}
//rssi_threshold threshF {-60, "5m"}
//rssi_threshold threshG {-70, "6m"}
//
//void device_detector(){
//  
//}
//Comment out the below to disclude tft functionality 
//TODO add a button to switch screen off to extend battery life


void writemsg(String msg){
  add_msg_to_array(msg);
  update_screen();
}

void add_msg_to_array(String msg){ // msg... the flavour enhancer
  for(int i=1; i <= available_lines ; i++){
    textarray[i-1] = textarray[i];
  }
  textarray[available_lines-1] = msg;   
}

void update_screen(){
  tft->fillRect(0,0,240,240,TFT_BLACK);
  tft->setCursor(0,0);
  for(int i = 1; i <= available_lines ; i++){
    tft->println(textarray[i-1]);
      debug_print(textarray[i-1]); 
  }
}

void debug_print(String feedback){
  //included to speed up loop time scans
  if (debug == true){
      Serial.println(feedback);
  }
}
