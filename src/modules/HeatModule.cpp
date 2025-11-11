#include "HeatModule.h"
#include "MeshService.h"
#include "configuration.h"
#include "main.h"

#include <assert.h>

ProcessMessage HeatModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    auto &p = mp.decoded;
    // The incoming message is in p.payload
    LOG_INFO("HH: Received message from=0x%0x, id=%d, chan=%d, msg=%.*s", mp.from, mp.id, (uint32_t)mp.channel, p.payload.size, p.payload.bytes);

    // Only process on non default channel
    if (channels.isDefaultChannel(mp.channel)){
        LOG_INFO("HH: Packet ignored");
        return ProcessMessage::CONTINUE;
    }

    bool actionTaken = 0;
    auto incomingMessage = reinterpret_cast<const char *>(p.payload.bytes);
    if (strncasecmp(incomingMessage, "heat ", 5) == 0){
        if (strncasecmp(incomingMessage+5, "high", 4) == 0){
            heatPower(3);
            actionTaken = 1;
        } else 
        if (strncasecmp(incomingMessage+5, "med", 3) == 0){
            heatPower(2);
            actionTaken = 1;
        } else
        if (strncasecmp(incomingMessage+5, "low", 3) == 0){
            heatPower(1);
            actionTaken = 1;
        } else
        if (strncasecmp(incomingMessage+5, "off", 3) == 0){
            heatPower(0);
            actionTaken = 1;
        }
    }
    else if (strncasecmp(incomingMessage, "outlet off", 10) == 0){
        digitalWrite(OUTLETPIN,LOW);
        powerstate = 0;
        actionTaken = 1;
    }
    else if (strncasecmp(incomingMessage, "outlet on", 9) == 0){
        digitalWrite(OUTLETPIN,HIGH);
        powerstate = 1;
        actionTaken = 1;
    }

    if (actionTaken){
        auto reply = allocDataPacket();
        reply->channel = mp.channel;
        reply->decoded.reply_id = mp.id;
        reply->decoded.payload.size = p.payload.size;
        memcpy(reply->decoded.payload.bytes, p.payload.bytes, reply->decoded.payload.size);

        service->sendToMesh(reply);
        
        return ProcessMessage::CONTINUE;
    }

    return ProcessMessage::CONTINUE;
}

int32_t HeatModule::runOnce(){
    if (powercycle){
        if (powerstate){
            digitalWrite(OUTLETPIN, LOW);
            powerstate = 0;
            ESP_LOGI("HH","outlet off");
            return 500;
        } else {
            digitalWrite(OUTLETPIN, HIGH);
            powerstate = 1;
            powercycle = 0;
            ESP_LOGI("HH","outlet on");
            return 1500;
        }
    } else {
        if (clicks > 0){
            if (!switchstate){
                digitalWrite(HEATPIN, HIGH);
                switchstate = 1;
            } else {
                digitalWrite(HEATPIN, LOW);
                switchstate = 0;
                clicks--;
            }
            ESP_LOGI("HH","clicks: %d heatpin: %d", clicks, switchstate);
        }
        return 500;
    }
    return 1000;
}

void HeatModule::heatPower(uint8_t level){
    powercycle = 1;
    switch (level){
        case 0:
            clicks = 0; break;
        case 1:
            clicks = 3; break;
        case 2:
            clicks = 2; break;
        case 3:
            clicks = 1; break;
    }
    LOG_INFO("HH: heatPower set to %d", level);
}