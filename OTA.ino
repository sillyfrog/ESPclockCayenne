#include <ArduinoOTA.h>

void setupOTA()
{
  ArduinoOTA.onStart([]() {
    DebugLn("Start");
    display.display((int16_t)0);
  });
  ArduinoOTA.onEnd([]() {
    DebugLn("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int16_t pcg;
    pcg = progress / (total / 100);
    if (pcg != prev_pcg) {
      prev_pcg = pcg;
      Debug("Update progress: ");
      Debug(pcg);
      DebugLn("%");
      //Debug("%\r");
      //DebugLn("%%");
      display.display(pcg);
    }
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Debug("Error[" + error);
    Debug("]: ");
    if (error == OTA_AUTH_ERROR) DebugLn("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) DebugLn("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) DebugLn("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) DebugLn("Receive Failed");
    else if (error == OTA_END_ERROR) DebugLn("End Failed");
  });
  ArduinoOTA.begin();
}

void loopOTA() {
  ArduinoOTA.handle();
}

