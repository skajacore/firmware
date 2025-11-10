#pragma once
#include "SinglePortModule.h"

#define OUTLETPIN 46
#define HEATPIN 19

/**
 * A module which controls an external heater via a transistor simulating button presses and control of main power via a relay.
 */
class HeatModule : public SinglePortModule, private concurrency::OSThread
{
  public:
    /** Constructor
     * name is for debugging output
     */
    HeatModule() : SinglePortModule("heat", meshtastic_PortNum_TEXT_MESSAGE_APP), concurrency::OSThread("HeatController") {
      pinMode(HEATPIN, OUTPUT);
      pinMode(OUTLETPIN, OUTPUT);
      digitalWrite(OUTLETPIN, LOW);
      powerstate = 0;
      powercycle = 0;
      switchstate = 0;
      clicks = 0;
    }
    
    virtual int32_t runOnce() override;

  protected:
    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;

    uint8_t clicks;
    bool powercycle;
    bool powerstate;
    bool switchstate;

    void heatPower(uint8_t level);
    
};

