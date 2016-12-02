/*

 Udp NTP Client

 Get the time from a Network Time Protocol (NTP) time server
 Demonstrates use of UDP sendPacket and ReceivePacket
 For more on NTP time servers and the messages needed to communicate with them,
 see http://en.wikipedia.org/wiki/Network_Time_Protocol

 created 4 Sep 2010
 by Michael Margolis
 modified 9 Apr 2012
 by Tom Igoe
 updated for the ESP8266 12 Apr 2015 
 by Ivan Grokhotkov

 This code is in the public domain.

 */

extern "C" {
#include "user_interface.h"
}

#include "debug.h"
#include "settings.h"
#include "clock.h"

#include "TM1637.h"
#include <Time.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include "FS.h"
#include <RF433.h>

#define CAYENNE_DEBUG
#define CAYENNE_PRINT Serial    // Comment this out to disable prints and save space
#include "CayenneDefines.h"
#include "BlynkSimpleEsp8266.h"
#include "CayenneWiFiClient.h"

#include "DHT.h"

/*
 * mysettigs.h should contain your WiFi Settings.
 * It should be named "mysettings.h", and placed in your project directory.
 * The contents of the file are:

char ssid[] = "xxx";  //  your network SSID (name)
char pass[] = "xxx";       // your network password
char token[] = "xxx";  // Cayenne authentication token.

 * I do it this way so I can exclude my WiFi settings in my .gitignore file.
 * It also means it's less likely someone will accedently commit their WiFi settings.
 */
#include "mysettings.h"

#define LUX_VIRTUAL_PIN V0
#define HUM_VIRTUAL_PIN V1
#define TEMP_VIRTUAL_PIN V2

TM1637 display(D5, D6); // Clk, DIO
RF433 rf433(-1, D7);

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTPIN D4     // what digital pin we're connected to
DHT dht(DHTPIN, DHTTYPE);

float lux = -1;
float temperature = -100;
float humidity = -100;
unsigned long nextenvread = 0;

bool syncok = false;

unsigned long epoch = 0;

char buffer[500];

void setup()
{
  DebugStart();
  DebugLn("");
  DebugLn("Starting.");
  
  SPIFFS.begin();
  settings.Load();
  setBrightness(4);
  writeInteger(now());
  
  DebugLn("Cayenne.begin...");
  Cayenne.begin(token, ssid, pass);
  
  DebugLn("setupTime()");
  setupTime();
  ntpActive = 1;

  setupOTA();
  setupWeb();
  setupLux();
  rf433.setup();
  dht.begin();
  
  applySettings();
}

int timestate = 0;
unsigned long nextcheck = millis();
bool colon = false;
int setbrightness;
  
void loop()
{  
  
  // print the hour, minute and second:
  time_t t = now();
  int hour12;
  
  if (millis() > nextcheck) {
    nextcheck += 500;
        
    if (t > 1000000000 && timestate == 0) {
      timestate = 1;
      setSyncInterval(settings.syncinterval);
    }
    if (timestate == 0) {
      writeInteger(t);
    }
    else {
      hour12 = hour(t)%12;
      if (hour12 == 0) {
        hour12 = 12;
      }
      if (syncok) {
        colon = true;
      } else {
        colon = !colon;
      }
      setColon(colon);
      //display.writeTime(hour12, minute(t), colon);
      writeInteger((hour12 * 100) + minute(t));
    }
    loopLux();
    adjustBrightness();

    if (millis() > nextenvread) {
      readEnvironment();
      nextenvread = millis() + 5000;
    }
  }

/*
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (3.2 / 1023.0);
  Debug("Voltage: ");
  DebugLn(voltage);
  */
  
  loopOTA();
  loopWeb();
  Cayenne.run();

  yield();
}

// This function is called when the Cayenne widget requests data for the Virtual Pin.
CAYENNE_OUT(LUX_VIRTUAL_PIN)
{
  Debug("Read lux: ");
  DebugLn(lux);
  if (lux < 0.001) {
    lux = 0.001; // Another bug in Cayenne, it does not appear to record when the value is zero
  }
  Cayenne.virtualWrite(LUX_VIRTUAL_PIN, lux);
}

CAYENNE_OUT(TEMP_VIRTUAL_PIN)
{
  Debug("Read Temp: ");
  DebugLn(temperature);
  Cayenne.virtualWrite(TEMP_VIRTUAL_PIN, temperature);
  //Cayenne.virtualWrite(TEMP_VIRTUAL_PIN, t, TEMPERATURE, CELSIUS);
}
CAYENNE_OUT(HUM_VIRTUAL_PIN)
{
  Debug("Read Hum: ");
  DebugLn(humidity);
  Cayenne.virtualWrite(HUM_VIRTUAL_PIN, humidity);
}

void readEnvironment() {
  float h = dht.readHumidity();
  float t = dht.readTemperature(true); // Get in F because it's buggy!
  Debug("Update environment: Temp: ");
  Debug(t);
  Debug(" Hum: ");
  DebugLn(h);
  if (h > 0) humidity = h;
  if (t > -100) temperature = t;
}

void applySettings() {
  setBrightness(settings.brightness);
  setSyncInterval(10);
  timestate = 0;
}

void setBrightness(int brightness) {
  display.set(brightness);
  setbrightness = brightness;
}

void writeInteger(int16_t val) {
  display.display(val);
}

void setColon(bool state) {
  display.point(state);
}

void adjustBrightness() {
  int targetbrightness;
  if (lux < settings.lvl0lux) {
    targetbrightness = 0;
  }
  else if  (lux < settings.lvl1lux) {
    targetbrightness = 1;
  }
  else if  (lux < settings.lvl2lux) {
    targetbrightness = 2;
  }
  else {
    targetbrightness = 3;
  }
  setBrightness(targetbrightness);
}

int16_t prev_pcg = 101;

