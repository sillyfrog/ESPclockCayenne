#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

// SCL: D1
// SDA: D2
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

void setupLux()
{
  /* Initialise the sensor */
  if(!tsl.begin())
  {
    DebugLn("No TSL2561 detected");
  } else {
    DebugLn("TSL2561 detected!");
  }
 
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
}


void loopLux()
{
  /* Get a new sensor event */ 
  sensors_event_t event;
  tsl.getEvent(&event);
 
  /* Display the results (light is measured in lux) */
  
  if (event.light)
  {
    lux = event.light;
  }
  else
  {
    lux = 0;
  }
}

