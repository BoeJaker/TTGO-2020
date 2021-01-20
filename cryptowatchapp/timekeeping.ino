
//  RTC_Date tnow = ttgo->rtc->getDateTime();
//
//  hh = tnow.hour;
//  mm = tnow.minute;
//  ss = tnow.second;
//  dday = tnow.day;
//  mmonth = tnow.month;
//  yyear = tnow.year;

//keep time, this code is kind of a hot mess and could use a good refactor
//also handles drawing time and date to the display

void setup_clock(){
  //Check if the RTC clock matches, if not, use compile time
  ttgo->rtc->check();

  //Synchronize time to system time
  ttgo->rtc->syncToSystem();
}

boolean correctTime = true;
//header file for the time.h library https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-timer.h (closest thing to documentation I've found so far)
//
//
//struct
void mjd_set_timezone_gmt()
{
  ESP_LOGD(TAG, "%s()", __FUNCTION__);
  // @doc https://remotemonitoringsystems.ca/time-zone-abbreviations.php
  // @doc timezone UTC = UTC
  setenv("TZ", "GMT+0BST-1", 1);
  tzset();
}


String getInternetTime()
{
  mjd_set_timezone_gmt();
  
  //connect to WiFi
//  wifiConnect();
  
  WiFi.mode(WIFI_STA);
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(1000);
  printLocalTime();
  mjd_set_timezone_gmt();
  time(&now);
  timeinfo = localtime(&now);

  byte Hour = timeinfo->tm_hour;
  byte Minute = timeinfo->tm_min;
  byte Second = timeinfo->tm_sec;
  byte Day = timeinfo->tm_mday;
  byte Month = timeinfo->tm_mon;
  byte Year = timeinfo->tm_year;

  ttgo->rtc->setDateTime(Year, Month, Day, Hour, Minute, Second);
  return "00:00:00AM";
}

void printLocalTime()
{
  time(&now);
  timeinfo = localtime(&now);
#ifdef DEBUG
  Serial.printf("%s\n", asctime(timeinfo));
  delay(2); // 26 bytes @ 115200 baud is less than 2 ms
#endif
}

void drawDate(int x, int y, int textSize)
{
  //configure current timezone (this information gets lost in deep sleep)
  mjd_set_timezone_gmt();
  time(&now);
  timeinfo = localtime(&now);

  String weekday;

  switch (timeinfo->tm_wday)
  {
    case 0:
      weekday = "Sunday";
      break;
    case 1:
      weekday = "Monday";
      break;
    case 2:
      weekday = "Tuesday";
      break;
    case 3:
      weekday = "Wednesday";
      break;
    case 4:
      weekday = "Thursday";
      break;
    case 5:
      weekday = "Friday";
      break;
    case 6:
      weekday = "Saturday";
      break;
    default:
      weekday = "error";
      break;
  }

  String Date = weekday + ", " + String(timeinfo->tm_mon + 1) + "/" + String(timeinfo->tm_mday);

  ttgo->tft->setTextSize(textSize);
  ttgo->tft->setTextColor(CLOCK_COLOR);
  for (int a = 0; a < Date.length(); a++)
  {
    ttgo->tft->setCursor(x + a * 6 * textSize, y);
    ttgo->tft->fillRect(x + a * 6 * textSize, y, 6 * textSize, 10 * textSize, TFT_BLACK); //Ticker 
    ttgo->tft->print(Date[a]);
  }
}

void drawDateCentered(int y, int textSize)
{
  //configure current timezone (this information gets lost in deep sleep)
  mjd_set_timezone_gmt();
  time(&now);
  timeinfo = localtime(&now);

  String weekday;

  switch (timeinfo->tm_wday)
  {
    case 0:
      weekday = "Sunday";
      break;
    case 1:
      weekday = "Monday";
      break;
    case 2:
      weekday = "Tuesday";
      break;
    case 3:
      weekday = "Wednesday";
      break;
    case 4:
      weekday = "Thursday";
      break;
    case 5:
      weekday = "Friday";
      break;
    case 6:
      weekday = "Saturday";
      break;
    default:
      weekday = "error";
      break;
  }

  String Date = weekday + ", " + String(timeinfo->tm_mon + 1) + "/" + String(timeinfo->tm_mday);

  int x = (SCREEN_WIDTH - (Date.length() * 6 * textSize)) / 2;

  ttgo->tft->setTextSize(textSize);
  ttgo->tft->setTextColor(CLOCK_COLOR);
  for (int a = 0; a < Date.length(); a++)
  {
    ttgo->tft->setCursor(x + a * 6 * textSize, y);
    ttgo->tft->fillRect(x + a * 6 * textSize, y, 6 * textSize, 10 * textSize, TFT_BLACK); //Ticker pair
    ttgo->tft->print(Date[a]);
  }
}


void drawTime(int x, int y, int textSize)
{
  lastTimeUpdate = millis();
  //configure current timezone (this information gets lost in deep sleep)
  mjd_set_timezone_gmt();
  time(&now);
  timeinfo = localtime(&now);

  String Hour = String(timeinfo->tm_hour, DEC);
  String Minute = String(timeinfo->tm_min, DEC);
  String Second = String(timeinfo->tm_sec, DEC);

  byte hour, minute, second = 0;
  hour = timeinfo->tm_hour;
  minute = (timeinfo->tm_min);
  second = timeinfo->tm_sec;

  char timestr[12] = "00:00 XM";
  if (timeinfo->tm_hour > 12)
  {
    timestr[0] = '0' + ((hour - 12) / 10);
    timestr[1] = '0' + ((hour - 12) % 10);
    timestr[6] = 'P';
  }
  else if (timeinfo->tm_hour == 12)
  {
    timestr[0] = '1';
    timestr[1] = '2';
    timestr[6] = 'P';
  }
  else if (timeinfo->tm_hour == 0)
  {
    timestr[0] = '1';
    timestr[1] = '2';
    timestr[6] = 'A';
  }
  else
  {
    timestr[0] = '0' + (timeinfo->tm_hour / 10);
    timestr[1] = '0' + (timeinfo->tm_hour % 10);
    timestr[6] = 'A';
  }

  timestr[3] = '0' + (timeinfo->tm_min / 10);
  timestr[4] = '0' + (timeinfo->tm_min % 10);

  /*  when writing the time we assume that we're writing over something, so for each character
       we fill in a black box behind it exactly the required size. we do this to try and prevent character "flashing"
       as much as possible.  */
  ttgo->tft->setTextSize(textSize);
  if (correctTime)
  {
    ttgo->tft->setTextColor(CLOCK_COLOR);
  }
  else
  {
    ttgo->tft->setTextColor(ERROR_COLOR);
  }
  for (int a = 0; a < 11; a++)
  {
    ttgo->tft->setCursor(x + a * 6 * textSize, y);
    ttgo->tft->fillRect(x + a * 6 * textSize, y, 6 * textSize, 8 * textSize, TFT_BLACK); //Ticker pair
    ttgo->tft->print(timestr[a]);
  }
}
