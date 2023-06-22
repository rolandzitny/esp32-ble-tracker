/**
 * File database controller. 
 */
#include <Arduino.h>
#include "SPIFFS.h"
#include "util.h"

class FDBController
{
private:

public:
    void Init(){
        if(!SPIFFS.begin(true)){
            Serial.println("An Error has occurred while mounting SPIFFS");
            ESP.restart();
            return;
        }
    }
    
    void updateDatabase(){
        /**
         * Update whole database by specific rules.
         * Those rules are in documentation.
         */
        String all_db_files_string = "";
        int number_of_files = 0;

        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        while(file){
            all_db_files_string += String(file.name()) + "=";
            number_of_files += 1;
            file = root.openNextFile();
        }

        String all_db_files_array[number_of_files];
        char buf[(all_db_files_string.length()) * sizeof(char)];
        all_db_files_string.toCharArray(buf, sizeof(buf));
        char *p = buf;
        char *str;
        int idx_get = 0;
        while ((str = strtok_r(p, "=", &p)) != NULL){
            all_db_files_array[idx_get] = String(str);
            idx_get +=1;
        }

        for(int i=0; i < number_of_files; i++){
            String file_mac_address = all_db_files_array[i];
            file_mac_address.remove(file_mac_address.length()-1);
            file_mac_address.remove(file_mac_address.length()-1);
            file_mac_address.remove(file_mac_address.length()-1);
            file_mac_address.remove(file_mac_address.length()-1);

            File file2 = SPIFFS.open("/" + all_db_files_array[i]);  
            String file_content = file2.readString();
            file2.close();
            String device_parameters[6];
            char buf[(1+file_content.length()) * sizeof(char)];
            file_content.toCharArray(buf, sizeof(buf));
            char *p = buf;
            char *str;
            int idx = 0;
            while ((str = strtok_r(p, "=", &p)) != NULL){
                device_parameters[idx] = String(str);
                idx +=1;
            }

            int is_scanned_flag = 0;
            for(auto device : SCANNED_DEVICES){
                if(file_mac_address == device.mac){
                    is_scanned_flag = 1;
                }
            }

            if(is_scanned_flag == 1){
                // Device was scanned
                String update_text = "";
                if(device_parameters[3] == "-"){
                    if(device_parameters[0] == "-"){
                        update_text += getTimeString() + "=";
                    } else {
                        update_text += device_parameters[0] + "=";
                    }

                    update_text += String(device_parameters[1].toInt() + 5) + "=";

                    if(device_parameters[2].toInt() != 0){
                        update_text += "0=";  
                        update_text += device_parameters[3] + "=" + device_parameters[4] + "=" + String(device_parameters[5].toInt() + device_parameters[2].toInt());                  
                    } else {
                        update_text += device_parameters[2] + "=" + device_parameters[3] + "=" + device_parameters[4] + "=" + device_parameters[5];                        
                    }

                    //UPDATE FILE WRITE...
                    File updated_file = SPIFFS.open("/" + all_db_files_array[i], FILE_WRITE);
                    updated_file.print(update_text);
                    updated_file.close();
                }
            } else {
                // Device was not found
                String update_text = "";
                if(device_parameters[0] != "-"){
                    if(device_parameters[3] == "-"){
                        update_text += device_parameters[0] + "=" + device_parameters[1] + "=";

                        if((device_parameters[2].toInt() + 5) == 40){
                            update_text += "0=";
                            update_text += getTimeString() + "=";
                        } else {
                            update_text += String(device_parameters[2].toInt() + 5) + "=";
                            update_text += device_parameters[3] + "=";
                        }

                        update_text += device_parameters[4] + "=" + device_parameters[5];

                        //UPDATE FILE WRITE...
                        File updated_file1 = SPIFFS.open("/" + all_db_files_array[i], FILE_WRITE);
                        updated_file1.print(update_text);
                        updated_file1.close();  
                    }
                }
            }
        }
    }

    void flippingDatabase(){
        /**
        * At the end of the day (work time) set all parameters of files
        * to base values and count total work time and total pause time. 
        */
        String all_db_files_string = "";
        int number_of_files = 0;

        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        while(file){
            all_db_files_string += String(file.name()) + "=";
            number_of_files += 1;
            file = root.openNextFile();
        }

        String all_db_files_array[number_of_files];
        char buf[(all_db_files_string.length()) * sizeof(char)];
        all_db_files_string.toCharArray(buf, sizeof(buf));
        char *p = buf;
        char *str;
        int idx_get = 0;
        while ((str = strtok_r(p, "=", &p)) != NULL){
            all_db_files_array[idx_get] = String(str);
            idx_get +=1;
        }

        for(int i=0; i < number_of_files; i++){
            String file_mac_address = all_db_files_array[i];
            file_mac_address.remove(file_mac_address.length()-1);
            file_mac_address.remove(file_mac_address.length()-1);
            file_mac_address.remove(file_mac_address.length()-1);
            file_mac_address.remove(file_mac_address.length()-1);

            File file2 = SPIFFS.open("/" + all_db_files_array[i]);  
            String file_content = file2.readString();
            file2.close();
            String device_parameters[6];
            char buf[(1+file_content.length()) * sizeof(char)];
            file_content.toCharArray(buf, sizeof(buf));
            char *p = buf;
            char *str;
            int idx = 0;
            while ((str = strtok_r(p, "=", &p)) != NULL){
                device_parameters[idx] = String(str);
                idx +=1;
            }

            String update_text = "";
            update_text += "-=0=0=-=" + String(device_parameters[4].toInt() + device_parameters[1].toInt()) + "=" + String(device_parameters[5].toInt() + device_parameters[2].toInt());  

            File updated_file = SPIFFS.open("/" + all_db_files_array[i], FILE_WRITE);
            updated_file.print(update_text);
            updated_file.close();
        }
    }

    void cleanSpiffs(){
        /**
         * Remove all Spiffs files. 
         */
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        Serial.println("Removing files:");
        while(file){
            Serial.println("/" + String(file.name()));
            SPIFFS.remove("/" + String(file.name()));
            file = root.openNextFile();
        }

        //Some file needs to be in database, else it cause some error.
        File updated_file = SPIFFS.open("/base.txt", FILE_WRITE);
        updated_file.print("-=0=0=-=0=0");
        updated_file.close();
    }

    void checkSpiffs(){
        /**
         * Print all spiffs files.
         */
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        while(file){
            File file2 = SPIFFS.open("/" + String(file.name()));
            Serial.print(file.name());
            Serial.print(": ");
            while(file2.available()){
                Serial.write(file2.read());
            }
            Serial.println("");
            file = root.openNextFile();
        }
    }

    void printAllSpiffsFilenames(){
        /**
         * Prints all spiffs filenames.
        */
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        while(file){
            Serial.println(file.name());
            file = root.openNextFile();
        }
    }
};

