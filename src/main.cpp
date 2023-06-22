/**
 * Main progam - WorkTracker 
 */
#include <Arduino.h>
#include "util.h"
#include "web_controller.h"
#include "ble_controller.h"
#include "fdb_controller.h"

const char* SSID      = "ESP";          //ESP
const char* WIFI_PSWD = "esp3295174";   //esp3295174

int test_val = 0;

WebController web_controller;
BLEController ble_controller;
FDBController fdb_controller;

void setup() {
  Serial.begin(115200);
  Serial.println("");

  setup_wifi(SSID, WIFI_PSWD);
  
  web_controller.Init();
  ble_controller.Init();
  fdb_controller.Init();

  Serial.println("");
}

/*  
 * Scan every 5 minutes.
 * Scan duration is 4 minutes and aproximatly one minute is used for DB and Web operations.
 */
void loop() {
  int start_time = getTimeMilis();
  /* ------------------------------------------------------------------------------ */
  if(isWorkTime(start_time, 6, 22, 15)){
    // IS WORK TIME
    SCANNED_DEVICES.clear();
    ble_controller.startup_scan();
    String scanned_devices = transferScannedDevicesToWebString();
    events.send(scanned_devices.c_str(),"scanned_devices",millis());

    fdb_controller.updateDatabase();

    String tracked_devices = transferTrackedDevicesToWebString();
    events.send(tracked_devices.c_str(),"tracked_devices",millis()); 
  } else {
    // END OF WORK
    fdb_controller.flippingDatabase();
    String tracked_devices = transferTrackedDevicesToWebString();
    events.send(tracked_devices.c_str(),"tracked_devices",millis());
  }
  /* ------------------------------------------------------------------------------ */
  int end_time = getTimeMilis();
  delay(300000 - (end_time - start_time)); //300000ms is 5 minutes
}