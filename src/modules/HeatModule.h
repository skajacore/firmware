#pragma once
#include "SinglePortModule.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into a specific GPIO pin
#define ONE_WIRE_BUS 6 // Change to your connected GPIO pin

#define OUTLETPIN 46
#define HEATPIN 19

/**
 * A module which controls an external heater via a transistor simulating button presses and control of main power via a relay.
 */
class HeatModule : public SinglePortModule, private concurrency::OSThread
{


  public:
    // Setup a oneWire instance to communicate with any OneWire devices
    OneWire _oneWire;

    // Pass our oneWire reference to Dallas Temperature Library
    DallasTemperature _sensors;
    
    /** Constructor
     * name is for debugging output
     */
    HeatModule() : SinglePortModule("heat", meshtastic_PortNum_TEXT_MESSAGE_APP), concurrency::OSThread("HeatController"), _oneWire(ONE_WIRE_BUS), _sensors(&_oneWire) {
      pinMode(HEATPIN, OUTPUT);
      pinMode(OUTLETPIN, OUTPUT);
      digitalWrite(OUTLETPIN, LOW);
      pinMode(7, OUTPUT);
      digitalWrite(7, HIGH);
      powerstate = 0;
      powercycle = 0;
      switchstate = 0;
      clicks = 0;

      pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
      _sensors.begin();
      _sensors.requestTemperatures();  // pre-read
      _sensors.setWaitForConversion(false);

    }
    
    virtual int32_t runOnce() override;



  protected:
    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;

    uint8_t clicks;
    bool powercycle;
    bool powerstate;
    bool switchstate;
    float tempF;
    void heatPower(uint8_t level);
    
};

