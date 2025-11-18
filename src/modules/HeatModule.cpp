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
    
    Action action = ACT_NONE;

    char * str = new char[p.payload.size];
    for(int i = 0; p.payload.bytes[i] && (i < p.payload.size); i++){
      str[i] = tolower(p.payload.bytes[i]);
    }

    char* a = strstr(str, "heat ");
    if (a){
        heatLevel = (
            strncasecmp(a+5, "high", 4) == 0 ? HT_HIGH :
            strncasecmp(a+5, "med", 3) == 0 ? HT_MED :
            strncasecmp(a+5, "low", 3) == 0 ? HT_LOW :
            HT_OFF
        );
        action = ACT_HEAT;
    }
    a = strstr(str, "outlet ");
    if (a){
        if (strncasecmp(a+7, "off", 3) == 0){
            digitalWrite(OUTLETPIN,LOW);
            powerstate = 0;
        } else
        if (strncasecmp(a+7, "on", 2) == 0){
            digitalWrite(OUTLETPIN,HIGH);
            powerstate = 1;
        }
        action = (action ? ACT_STATUS : ACT_OUTLET);
    }

    a = strstr(str, "status");
    if (a){
        action = ACT_STATUS;
    }
    
    a = strstr(str, "set ");
    if (a){
        tempSetpoint = atof(a+4);
        action = (action ? ACT_STATUS : ACT_SETPOINT);
    }
    
    a = strstr(str, "hyst ");
    if (a){
        tempHysteresis = atof(a+5);
        action = (action ? ACT_STATUS : ACT_HYSTERESIS);
    }

    if (action > ACT_NONE){
        auto reply = allocDataPacket();
        reply->channel = mp.channel;
        reply->decoded.reply_id = mp.id;

        switch(action){
            case ACT_HEAT:
            case ACT_OUTLET:
                reply->decoded.payload.size = p.payload.size;
                memcpy(reply->decoded.payload.bytes, p.payload.bytes, reply->decoded.payload.size);
                break;
            case ACT_TEMP:
                reply->decoded.payload.size = snprintf((char*)(reply->decoded.payload.bytes), 12, "Temp: %0.1f", tempF);
                break;
            case ACT_SETPOINT:
                reply->decoded.payload.size = snprintf((char*)(reply->decoded.payload.bytes), 12, "Set: %0.1f", tempSetpoint);
                break;
            case ACT_HYSTERESIS:
                reply->decoded.payload.size = snprintf((char*)(reply->decoded.payload.bytes), 12, "Hyst: %0.1f", tempSetpoint);
                break;
            case ACT_STATUS:
                reply->decoded.payload.size = snprintf((char*)(reply->decoded.payload.bytes), 233,
                    "PWR: %s\nHTL: %s\nT: %0.1fF\nS: %0.1fF\nH: %0.1fF", 
                    (powerstate ? "on" : "off"),
                    (
                        heatLevel == HT_LOW ? "low" :
                        heatLevel == HT_MED ? "med" :
                        heatLevel == HT_HIGH ? "high" :
                        "off"
                    ),
                    tempF,
                    tempSetpoint,
                    tempHysteresis);
                break;
            default:
                break;
        }

        service->sendToMesh(reply);
    }


    return ProcessMessage::CONTINUE;
}

int32_t HeatModule::runOnce(){
    tempF = _sensors.getTempCByIndex(0)*1.8+32;
    
    if (tempF < -100.0f){
        digitalWrite(OUTLETPIN, LOW);
        powerstate = 0;
        return 1000;
    }

    if (heatLevel > HT_OFF && tempSetpoint > 1.0f){
        if (powerstate && tempF > (tempSetpoint + tempHysteresis)){
            clicks = HT_NVAL-heatLevel;
            digitalWrite(OUTLETPIN, LOW);
            powerstate = 0;
        } else
        if ((!powerstate || (heatLevel != lastHeatLevel)) && tempF < (tempSetpoint - tempHysteresis)){
            heatPower(heatLevel);
        }
    }

    //ESP_LOGI("HH","temp: %f", tempF);
    // print
    _sensors.requestTemperatures();  // async update

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

void HeatModule::heatPower(HtLevel level){
    powercycle = 1;
    heatLevel = level;
    lastHeatLevel = level;
    clicks = (uint8_t)level;
    LOG_INFO("HH: heatPower set to %d", (level == HT_HIGH ? "high" : level == HT_MED ? "med" : level == HT_LOW ? "low" : "off"));
}