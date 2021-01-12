
void get_default_charge_current(){
    int cur =  ttgo->power->getChargeControlCur();
    tft->setCursor(0, 100);
    tft->print("Default Current: "); tft->print(cur); tft->println(" mA");
}

void set_charge_current(){
    tft->setCursor(0, 130);
    //axp202 allows maximum charging current of 1800mA, minimum 300mA
    tft->print("Set charge control current 500 mA");
    ttgo->power->setChargeControlCur(500);
}

void get_charge_current(){
    tft->setCursor(0, 160);
    int cur =  ttgo->power->getChargeControlCur();
    tft->print("Setting Current: "); tft->print(cur); tft->println(" mA");

}
