/*  Copyright (C) 2016 Buxtronix and Alexander Pruss

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include "FS.h"
#include "clock.h"

#define FIELD_LENGTH 64


class Settings {
  private:

    char tmpbuf[50];
    
    String getStrSetting(String vname, String def)
    {
      File configFile = SPIFFS.open("/settings/" + vname + ".txt", "r");
      if (!configFile) {
        DebugLn("Failed to open config file");
        return def;
      }
      return configFile.readString();
    }

    String getStrSetting(String vname)
    {
      return getStrSetting(vname, "");
    }
    
    long getIntSetting(String vname, long def)
    {
      File configFile = SPIFFS.open("/settings/" + vname + ".txt", "r");
      if (!configFile) {
        DebugLn("Failed to open config file");
        return def;
      }
      return configFile.parseInt();
    }
    
    float getFloatSetting(String vname, float def)
    {
      File configFile = SPIFFS.open("/settings/" + vname + ".txt", "r");
      if (!configFile) {
        DebugLn("Failed to open config file");
        return def;
      }
      return configFile.parseFloat();
    }
    
    void saveSetting(String vname, String value) {
      File f = SPIFFS.open("/settings/" + vname + ".txt", "w");
      f.print(value);
      f.close();
    }

    void saveSetting(String vname, long value) {
      saveSetting(vname, String(value));
    }
    
    void saveSetting(String vname, float value) {
      saveSetting(vname, String(value));
    }
    void saveSetting(String vname, int value) {
      saveSetting(vname, String(value));
    }

  public:

    void Load() {
      timezone = getIntSetting("timezone", 0);
      syncinterval = getIntSetting("syncinterval", 3600);
      brightness = getIntSetting("brightness", 1);
      getStrSetting("timeserver", "not.yet.set").toCharArray(timeserver, FIELD_LENGTH);
      devname = getStrSetting("devname", "New Device");
      getStrSetting("ssid", "your ssid").toCharArray(ssid, FIELD_LENGTH);
      getStrSetting("psk", "aPSK").toCharArray(psk, FIELD_LENGTH);
      
      lvl0lux = getFloatSetting("lvl0lux", 5);
      lvl1lux = getFloatSetting("lvl1lux", 10);
      lvl2lux = getFloatSetting("lvl2lux", 20);
      //lvl3lux = getFloatSetting("lvl3lux", 40);
      luxmargin = getFloatSetting("luxmargin", 5);
      
      uuid = getStrSetting("uuid", "");
      token = getStrSetting("token", "");
    }

    void Save() {
      saveSetting("timezone", timezone);
      saveSetting("syncinterval", syncinterval);
      saveSetting("brightness", brightness);
      saveSetting("timeserver", timeserver);
      saveSetting("devname", devname);
      saveSetting("ssid", ssid);
      saveSetting("psk", psk);
      
      saveSetting("lvl0lux", lvl0lux);      
      saveSetting("lvl1lux", lvl1lux);
      saveSetting("lvl2lux", lvl2lux);
      //saveSetting("lvl3lux", lvl3lux);
      saveSetting("luxmargin", luxmargin);

      saveSetting("uuid", uuid);
      saveSetting("token", token);
    }
    
    int16_t timezone = 0;
    char usdst = 0;
    char timeserver[FIELD_LENGTH] = "";
    int syncinterval = 60;
    int brightness = 4;
    String devname = "";
    char ssid[FIELD_LENGTH] = "";
    char psk[FIELD_LENGTH] = "";
    float lvl0lux;
    float lvl1lux;
    float lvl2lux;
    //float lvl3lux;
    float luxmargin;
    
    String uuid = "";
    String token = "";
};

Settings settings;



