/**
 * Bluetooth low energy controller. 
 */
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <list>
#include "util.h"

/**
 * Callback class determins behavior on scanning specific device.
 * Device is inserted into SCANNED_DEVICES. 
 */
class ScanResultCallback: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        ScannedDev device;
        device.name = String(advertisedDevice.getName().c_str());
        device.mac = String(advertisedDevice.getAddress().toString().c_str());
        SCANNED_DEVICES.push_back(device);
    }
};

class BLEController
{
private:
    BLEScan* pBLEScan;
    int scanTime = 15; //In seconds

public:
    void Init(){
        BLEDevice::init("");
        pBLEScan = BLEDevice::getScan();
        pBLEScan->setAdvertisedDeviceCallbacks(new ScanResultCallback());
        pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
        pBLEScan->setInterval(100);
        pBLEScan->setWindow(99);  // less or equal setInterval value
    }

    void startup_scan(){
        /**
         * Startup scan called on start of ESP to obtain some devices. 
         */
        BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
        pBLEScan->clearResults();
    }

    void window_scan(){
        /**
         * Performs scan in 3 sessions when one session lasts scanTime seconds.
         * First session starts immediately and waits 120000ms.
         * Seccond session start after that and continue scan (first session holds result), wait another 120000ms.
         * Third session also continue in scan and after that all scanned devices are removed.
         * Whole process lasts aproximatly 4 minutes.
         */
        BLEScanResults foundDevices = pBLEScan->start(scanTime, true);
        delay(120000 - (scanTime * 1000));
    
        foundDevices = pBLEScan->start(scanTime, true);
        delay(120000 - (scanTime * 1000));
    
        foundDevices = pBLEScan->start(scanTime, true);
        pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
    }
};


