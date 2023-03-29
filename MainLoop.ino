/*  everything that happens in the normal operation of the smartwatch happens here. this is kind of the user-interface part of the code.
*/
bool newNotificationData = false;
bool dontSleep = false;
int previousPage = 0;


//void updateStepCounter(uint32_t counter)
//{
//    bar.setStepCounter(counter);
//}

void MainLoop()
{
  bool  rlst;
  uint8_t data;
  initBLE();
  
  //! Fast response wake-up interrupt
    EventBits_t  bits = xEventGroupGetBits(isr_group);
    if (bits & WATCH_FLAG_SLEEP_EXIT) {
        if (lenergy) {
            lenergy = false;
            // rtc_clk_cpu_freq_set(RTC_CPU_FREQ_160M);
            setCpuFrequencyMhz(160);
        }

        low_energy();

        if (bits & WATCH_FLAG_BMA_IRQ) {
            do {
                rlst =  ttgo->bma->readInterrupt();
            } while (!rlst);
            xEventGroupClearBits(isr_group, WATCH_FLAG_BMA_IRQ);
        }
        if (bits & WATCH_FLAG_AXP_IRQ) {
            ttgo->power->readIRQ();
            ttgo->power->clearIRQ();
            //TODO: Only accept axp power pek key short press
            xEventGroupClearBits(isr_group, WATCH_FLAG_AXP_IRQ);
        }
        xEventGroupClearBits(isr_group, WATCH_FLAG_SLEEP_EXIT);
        xEventGroupClearBits(isr_group, WATCH_FLAG_SLEEP_MODE);
    }
    if ((bits & WATCH_FLAG_SLEEP_MODE)) {
        //! No event processing after entering the information screen
        return;
    }

    //! Normal polling
    if (xQueueReceive(g_event_queue_handle, &data, 5 / portTICK_RATE_MS) == pdPASS) {
        switch (data) {
        case Q_EVENT_BMA_INT:
            do {
                rlst =  ttgo->bma->readInterrupt();
            } while (!rlst);

            //! setp counter
//            if (ttgo->bma->isStepCounter()) {
//                updateStepCounter(ttgo->bma->getCounter());
//            }
            break;
        case Q_EVENT_AXP_INT:
            ttgo->power->readIRQ();
//            if (ttgo->power->isVbusPlugInIRQ()) {
//                updateBatteryIcon(LV_ICON_CHARGE);
//            }
//            if (ttgo->power->isVbusRemoveIRQ()) {
//                updateBatteryIcon(LV_ICON_CALCULATION);
//            }
//            if (ttgo->power->isChargingDoneIRQ()) {
//                updateBatteryIcon(LV_ICON_CALCULATION);
//            }
            if (ttgo->power->isPEKShortPressIRQ()) {
                ttgo->power->clearIRQ();
                low_energy();
                return;
            }
            ttgo->power->clearIRQ();
            break;
//        case Q_EVENT_WIFI_SCAN_DONE: {
//            int16_t len =  WiFi.scanComplete();
//            for (int i = 0; i < len; ++i) {
//                wifi_list_add(WiFi.SSID(i).c_str());
//            }
//            break;
//        }
        default:
            break;
        }

    }
    if (lv_disp_get_inactive_time(NULL) < DEFAULT_SCREEN_TIMEOUT) {
        while (lastTouchTime + screenOnTime > millis()) {
    if (!newNotificationData && pRemoteCharacteristic) {
      updateNotificationData();
      newNotificationData = true;
      drawHome();
    }
    if (touchDetected) {
      //handleTouch() operates in a similar manner to the MainLoop()
      //and simply switches to the correct touch handler
      handleTouch();
    } else {

      switch (currentPage)
      {
        case HOME:
          dontSleep = false;
          if (previousPage != currentPage) {
            drawHome();
          }
          break;
        case NOTIFICATIONS:
          dontSleep = false;
          if (previousPage != currentPage) {
            drawNotifications();
          }

          break;
        default:
          currentPage = HOME;
      }
      previousPage = currentPage;
    }
  }
    } else {
        low_energy();
    }
}
