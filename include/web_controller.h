/**
 * Web controller
 */
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <list>
#include "util.h"

AsyncWebServer server(80);
AsyncEventSource events("/events");

/**
 * Processor rewrites specific parts of HTML code (scanner_page).
 * Everything marked with %SCANED_DEVICES% and %TRACKED_DEVICES%. 
 */
String processor(const String& var){
  if(var == "SCANNED_DEVICES"){
    return transferScannedDevicesToWebString();
  }
  else if(var == "TRACKED_DEVICES"){
    return transferTrackedDevicesToWebString();
  }
  return "";
}

class WebController
{
private:

public:
    void Init(){
        /**
         * We have just one page, where we gather list of choosen mac addresses from form.
         * Those mac addresses are tracked devices. 
         */
        server.addHandler(&events);
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){  
            String formInput; // tracked devices mac1,mac2,mac3...

            if (request->hasParam("formMacAddresses")){
                formInput = request->getParam("formMacAddresses")->value();
            }

            TRACKED_MAC_ADDRESSES.clear();

            char buf[(1+formInput.length()) * sizeof(char)];
            formInput.toCharArray(buf, sizeof(buf));
            char *p = buf;
            char *str;
            while ((str = strtok_r(p, ",", &p)) != NULL){
                TRACKED_MAC_ADDRESSES.push_back(String(str));
            }

            // For every chosen file create file in database.
            createDB_files();

            request->send_P(200, "text/html", scanner_page, processor);
        });

        server.begin(); 
    }
};
