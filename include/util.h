#ifndef UTIL 
#define UTIL
#include <Arduino.h>
#include <WiFi.h>
#include "SPIFFS.h"
#include <list>
#include "time.h"

/**
 * Structure for bluetooth devices. 
 */
struct ScannedDev{      
    String name;
    String mac;
};

std::list<ScannedDev> SCANNED_DEVICES = {};  // list of scanned devices
std::list<String> TRACKED_MAC_ADDRESSES = {};  // list of scanned devices

/**
 * Transfer list od ScannedDev devices into String valid to web javascript.  
 */
String transferScannedDevicesToWebString(){
    String result = "";
    for(auto device : SCANNED_DEVICES){
        result += device.mac;
        result += "=";
        if(device.name.isEmpty()){
            result += "undef";
        } else {
            result += device.name;
        }
        result += ",";
    }

    result.remove(result.length() - 1);
    return result;
}

/**
 * Transfer tracked devices into String from with delimeter =, to parse them later
 * in javasript on web and creating table. 
 */
String transferTrackedDevicesToWebString(){
    String result = "";
    
    for(auto device : TRACKED_MAC_ADDRESSES){
        // Arduino spliting String by delimenter.
        String mac_name[2];
        char buf[(1+device.length()) * sizeof(char)];
        device.toCharArray(buf, sizeof(buf));
        char *p = buf;
        char *str;
        int idx = 0;
        while ((str = strtok_r(p, "=", &p)) != NULL){
            mac_name[idx] = String(str);
            idx +=1;
        }
        String device_mac = mac_name[0];

        result += mac_name[0] + "=" + mac_name[1] + "=";

        File file = SPIFFS.open("/" + device_mac + ".txt");

        if(!file){
            Serial.println("Failed to open file for reading");
            return "";
        }
        String file_content = file.readString();
        file.close();

        String file_content_values[6];
        char buf2[(1 + file_content.length()) * sizeof(char)];
        file_content.toCharArray(buf2, sizeof(buf2));
        char *p2 = buf2;
        char *str2;
        int idx2 = 0;
        while ((str2 = strtok_r(p2, "=", &p2)) != NULL){
            file_content_values[idx2] = String(str2);
            idx2 +=1;
        }
        
        result += file_content_values[0] + "=" + file_content_values[1] + "=" + file_content_values[2] + "=" + file_content_values[3] + "=" + file_content_values[4] + "=" + file_content_values[5];
        result += ",";
    }

    result.remove(result.length() - 1);
    return result;
}

/**
 * Creates MAC.txt file ater commiting form (button Track). 
 */
void createDB_files(){
    for(auto device : TRACKED_MAC_ADDRESSES){

        // Arduino spliting String by delimenter.
        String mac_name[2];
        char buf[(1+device.length()) * sizeof(char)];
        device.toCharArray(buf, sizeof(buf));
        char *p = buf;
        char *str;
        int idx = 0;
        while ((str = strtok_r(p, "=", &p)) != NULL){
            mac_name[idx] = String(str);
            idx +=1;
        }

        String device_mac = mac_name[0];
        
        // Work with database itself.
        if((device_mac != "") && (!SPIFFS.exists("/" + device_mac + ".txt"))){
            File file = SPIFFS.open("/" + device_mac + ".txt", FILE_WRITE);
            if(!file){
                Serial.println("Failed to open file for reading");
                return;
            }

            //time-start, work-minutes, pause-minutes, time-end, work-min-total, work-pause-total
            file.print("-=0=0=-=0=0");

            file.close();
            Serial.println("File " + device_mac + " was created.");
        } else {
            Serial.println("File " + device_mac + " exists.");
        }
    }
}

/**
 * Connects to Wifi and sets configuration for NTP.
 * 
 * @param SSID WiFi name.
 * @param WIFI_PSWD WiFi password.
 */
void setup_wifi(const char* SSID, const char* WIFI_PSWD){ 
    const char* ntpServer = "pool.ntp.org";
    const long  gmtOffset_sec = 3600;
    const int   daylightOffset_sec = 3600;

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(1000);

    WiFi.begin(SSID, WIFI_PSWD);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("WiFi Failed!");
        ESP.restart();
    }
    Serial.print("Server address is: ");
    Serial.println(WiFi.localIP());

    //NTP configuration
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

/**
 * Get current time in miliseconds (Hours + Minutes + Seconds).
 * 
 * @return time in miliseconds (precission on seconds). 
 */
int getTimeMilis(){
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("ERROR: WebController: Failed to obtain time -> RESTARTING ESP32");
        ESP.restart();
    }
    int h_ms = timeinfo.tm_hour * 60 * 60 * 1000;
    int m_ms = timeinfo.tm_min * 60 * 1000;
    int s_ms = timeinfo.tm_sec * 1000;

    return (h_ms + m_ms + s_ms);
}

/**
 * Return time in form of String Hours:Minutes.
 * 
 * @return Hours:Minutes time 
 */
String getTimeString(){
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("ERROR: WebController: Failed to obtain time -> RESTARTING ESP32");
        return "-";
    }
    String result = "";
    result = String(timeinfo.tm_hour) + ":" + String(timeinfo.tm_min);
    return result;
}

/**
 * Decide whether is work time or not.
 * 
 * @param time_ms time in miliseconds.
 * @param start_h start of work in hours.
 * @param end_h end of wotk in hours.
 * @param offset_m offset in minutes, which creates more time for employes to come or leav.
 * @return true for worktime, false for end of work.
 */
bool isWorkTime(int time_ms, int start_h, int end_h, int offset_m){
  int start_ms = (start_h * 60 * 60 * 1000) - (offset_m * 60 * 1000);
  int end_ms = (end_h * 60 * 60 * 1000) + (offset_m * 60 * 1000);

  if((time_ms > start_ms) && (time_ms < end_ms)){
    return true;
  }  

  return false;
}

/**
 * Scanner page 
 */
const char scanner_page[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
    <title>WorkTracker</title>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <link rel='stylesheet' href='https://use.fontawesome.com/releases/v5.7.2/css/all.css' integrity='sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr' crossorigin='anonymous'>
    <link rel='icon' href='data:,'>
    <style>
    * {box-sizing: border-box;}
    .row {margin-left:-5px; margin-right:-5px;}
    .column {float: left; width: 50%; padding: 5px; margin-right:20px;}
    .row::after {content: ""; clear: both; display: table;}
    table {border-collapse: collapse; border-spacing: 0; width: 100%; border: 1px solid #ddd;}
    th, td { text-align: left; padding: 16px;}
    tr:nth-child(odd) {background-color: #FFFFFF}
    tr:nth-child(even) {background-color: #f2f2f2;}
    body {  margin: 2em; background-color: #2E4756;}
    .button-13 {background-color: #fff;border: 1px solid #d5d9d9;border-radius: 8px;box-shadow: rgba(213, 217, 217, .5) 0 2px 5px 0;box-sizing: border-box;color: #0f1111;cursor: pointer;display: inline-block;font-family: "Amazon Ember",sans-serif;font-size: 13px;line-height: 29px;
    padding: 0 10px 0 11px;position: relative;text-align: center;text-decoration: none;user-select: none;-webkit-user-select: none;touch-action: manipulation;vertical-align: middle;width: 100px;
    }

    .button-13:hover {
    background-color: #f7fafa;
    }

    .button-13:focus {
    border-color: #008296;
    box-shadow: rgba(213, 217, 217, .5) 0 2px 5px 0;
    outline: 0;
    }
    </style>
</head>

<body>
<h1 style="color:white; font-family:verdana;">Work Tracker</h1>

<form style='display: none' action='/' id='mac_form'>
<input style='height: 5px;' type='text' name='formMacAddresses' id='formMac'>
</form>

<p style='display: none' id="scanned_devices">%SCANNED_DEVICES%</p>
<p style='display: none' id="tracked_devices">%TRACKED_DEVICES%</p>
<button class="button-13" role="button" onclick='startTracking()' style='width:10em; height: 2.5em;'>Track</button>

<button class="button-13" role="button" style='width:11em; margin-left: 10em; margin-left: 3em; height: 2.5em;'>Clean DB</button>
<br></br>

<div class="row" id="row_id">

<div class="column" id="left_col" style='height:50em; overflow:auto; border: 1px solid #2E4756; font-size: 12px;'>
<table id="table_scan">
    <tbody></tbody>
</table>
</div>

<div style="height:50em; margin-left: 5em; overflow:auto; border: 1px solid #2E4756; font-size: 12px;" class="column" id="right_col">
<table style='display: none' id="table_track">
    <tbody></tbody>
</table>
</div>

</div>

<script type='text/javascript'>
    function startTracking(){
        var checkboxes = document.getElementsByTagName('input');
        var selected_mac_addresses = [];
        for (var i = 0; i < checkboxes.length; i++){
            var checkbox = checkboxes[i];
            if (checkbox.checked == true) {
                var checkbox_row = checkbox.parentNode.parentNode;
                var checkbox_row_fst_cell = checkbox_row.getElementsByTagName('td')[0];
                var checkbox_row_scd_cell = checkbox_row.getElementsByTagName('td')[1];
                var device = checkbox_row_fst_cell.innerHTML + "=" + checkbox_row_scd_cell.innerHTML;
                selected_mac_addresses.push(device);
            }
        }
        console.log("selected mac", selected_mac_addresses);
        document.getElementById('formMac').value = selected_mac_addresses;
        alert(document.getElementById('formMac').value);
        document.getElementById('mac_form').submit();
    }

    function deleteScanTable(){
        const element1 = document.getElementById("table_scan");
        element1.remove();
    }

    function deleteTrackTable(){
        const element2 = document.getElementById("table_track");
        element2.remove();
    }


    function create_scan_table(){
        deleteScanTable();
        
        const scanned_devices = document.getElementById("scanned_devices").innerHTML;
        const devices = scanned_devices.split(",");

        const left_col_element = document.getElementById("left_col");
        const tbl = document.createElement("table");
        const tblBody = document.createElement("tbody");

        for (let i = 0; i < devices.length; i++) {
            const device_parts = devices[i].split("=")
            const row = document.createElement("tr");
                
            const cell1 = document.createElement("td");
            const cellMac = document.createTextNode(device_parts[0]);
            cell1.appendChild(cellMac);
            row.appendChild(cell1);

            const cell2 = document.createElement("td");
            const cellName = document.createTextNode(device_parts[1]);
            cell2.appendChild(cellName);
            row.appendChild(cell2);                

            const cell3 = document.createElement("td");
            const checkbox = document.createElement("INPUT");
            checkbox.setAttribute("type", "checkbox");
            cell3.appendChild(checkbox);
            row.appendChild(cell3);     

            tblBody.appendChild(row);
        }

        tbl.appendChild(tblBody);
        tbl.id = "table_scan";
        left_col_element.appendChild(tbl);
    }

    function create_track_table(){
        const tracked_devices = document.getElementById("tracked_devices").innerHTML;
        
        if(tracked_devices != ""){

            deleteTrackTable();

            const devices = tracked_devices.split(",");

            const right_col_element = document.getElementById("right_col");
            right_col_element.style.marginLeft = "5em";
            const tbl = document.createElement("table");
            const tblBody = document.createElement("tbody");

            const headrow = document.createElement("tr");
                const h1 = document.createElement("td");
                const h1_mac = document.createTextNode("MAC address");
                h1.appendChild(h1_mac);
                headrow.appendChild(h1);

                const h2 = document.createElement("td");
                const h2_name = document.createTextNode("Name");
                h2.appendChild(h2_name);
                headrow.appendChild(h2);

                const h3 = document.createElement("td");
                const h3_time = document.createTextNode("Start");
                h3.appendChild(h3_time);
                headrow.appendChild(h3);

                const h4 = document.createElement("td");
                const h4_work = document.createTextNode("W[min]");
                h4.appendChild(h4_work);
                headrow.appendChild(h4);

                const h5 = document.createElement("td");
                const h5_pause = document.createTextNode("P[min]");
                h5.appendChild(h5_pause);
                headrow.appendChild(h5);

                const h6 = document.createElement("td");
                const h6_time = document.createTextNode("Stop");
                h6.appendChild(h6_time);
                headrow.appendChild(h6);

                const h7 = document.createElement("td");
                const h7_totW = document.createTextNode("Total W[min]");
                h7.appendChild(h7_totW);
                headrow.appendChild(h7);

                const h8 = document.createElement("td");
                const h8_totP = document.createTextNode("Total P[min]");
                h8.appendChild(h8_totP);
                headrow.appendChild(h8);
            tblBody.appendChild(headrow);

            for (let i = 0; i < devices.length; i++) {
                const device_values = devices[i].split("=");

                const row = document.createElement("tr");

                const cell1 = document.createElement("td");
                const cellMac = document.createTextNode(device_values[0]);
                cell1.appendChild(cellMac);
                row.appendChild(cell1);
                
                const cell2 = document.createElement("td");
                const cellName = document.createTextNode(device_values[1]);
                cell2.appendChild(cellName);
                row.appendChild(cell2);

                const cell25 = document.createElement("td");
                const cellTimeSt = document.createTextNode(device_values[2]);
                cell25.appendChild(cellTimeSt);
                row.appendChild(cell25);

                const cell3 = document.createElement("td");
                const cellWork = document.createTextNode(device_values[3]);
                cell3.appendChild(cellWork);
                row.appendChild(cell3);

                const cell4 = document.createElement("td");
                const cellPause = document.createTextNode(device_values[4]);
                cell4.appendChild(cellPause);
                row.appendChild(cell4);

                const cell45 = document.createElement("td");
                const cellTimeEnd = document.createTextNode(device_values[5]);
                cell45.appendChild(cellTimeEnd);
                row.appendChild(cell45);

                const cell5 = document.createElement("td");
                const cellTotWork = document.createTextNode(device_values[6]);
                cell5.appendChild(cellTotWork);
                row.appendChild(cell5);

                const cell6 = document.createElement("td");
                const cellTotPause = document.createTextNode(device_values[7]);
                cell6.appendChild(cellTotPause);
                row.appendChild(cell6);

                tblBody.appendChild(row);
            }

            tbl.appendChild(tblBody);
            tbl.id = "table_track";
            right_col_element.appendChild(tbl);
        }
    }

    if (!!window.EventSource) {
        var source = new EventSource('/events');
        source.addEventListener('scanned_devices', function(e) {
            console.log("Devices:", e.data);
            document.getElementById("scanned_devices").innerHTML = e.data;
            create_scan_table();
        }, false);

        source.addEventListener('tracked_devices', function(e) {
            console.log("Tracked:", e.data);
            document.getElementById("tracked_devices").innerHTML = e.data;
            create_track_table();
        }, false);
    }

    create_scan_table();
    create_track_table();
</script>

</body>
</html>)rawliteral";

#endif