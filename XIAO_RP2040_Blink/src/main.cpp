#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
//inlcude the stack
#include <knx.h>
//include hardware file generated by the knxprod tool
#include "hardware.h"

//here we get our ComObjects
//The Number MUST start at 1
//there are no spaces allowed (like 1,2,4 is wrong)
#define goLedSwitch knx.getGroupObject(1)
#define goLedState knx.getGroupObject(2)

//define knx instance since we added the build flag "KNX_NO_AUTOMATIC_GLOBAL_INSTANCE"
//because we don't have a button for the prog-mode
//see https://github.com/thewhobox/KNX_Test/wiki/Prog_Mode
KnxFacade<RP2040ArduinoPlatform, Bau07B0> knx;

//use the builtin NeoPixel on the XIAO RP2040
int Power = 11;
int PIN = 12;
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


//custom handlers to show the prog state with the NeoPixel
void progLedOff()
{
    Serial.println("ProgMode OFF");
    pixels.clear();
    pixels.show();
}
void progLedOn()
{
    Serial.println("ProgMode ON");
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
    pixels.show();
}


int counter = 0;
int lastmillis = 0;
bool ledState = false;
bool ledActive = false;
int interval = 0;
int color = 0;

//easy function to set led to a state
void setLed(bool state)
{
    ledState = state;

    if(ledState)
    {
        //turn led on
        pixels.setPixelColor(0, pixels.Color(color == 0 ? 255:0, color == 1 ? 255:0, color == 2 ? 255:0));
        pixels.show();
    } else {
        //turn led off
        pixels.clear();
        pixels.show();
    }
    
    //send state to ComObject 2
    goLedState.value(ledState);
}

//this will be called if a knx telegram is received for GroupObject Number 1
//Our ComObject Numer 1 is 
void switchCallback(GroupObject& go)
{
    ledActive = go.value();
    Serial.print("Switch Callback ");
    Serial.println(go.value() ? "True" : "False");
}

void setup()
{
    //Start Serial so we can print things in the monitor at platformio
    Serial.begin(115200);

    //we set the DebugSerial so we can see output from the stack in platformio
    //we could also do this by setting the Build Flag "KNX_DEBUG_SERIAL"
    //see https://github.com/thewhobox/KNX_Test/wiki/Build_Flags
    ArduinoPlatform::SerialDebug = &Serial;

    //set our custom handler for prog-led on/off
    knx.setProgLedOffCallback(progLedOff);
    knx.setProgLedOnCallback(progLedOn);

    //read memory and load knx stuff like "physical address", "grouptable", ...
    //so the stack can check if the device is configured
    knx.readMemory();

    //check if your device is configured by the ETS
    if (knx.configured())
    {
        //yes it is, so now we can setup our device depending on the parameters

        //read the color of the led
        //Size 2 bit; Offset 0; BitOffset 0
        byte col = (knx.paramByte(PARAM_color) & PARAM_color_Mask) >> PARAM_color_Shift;
        Serial.print("Parameter Color: ");
        Serial.println(col, HEX);
        color = col;

        //read if the led should blink
        //Size 1 bit; Offset 0; BitOffset 2
        bool blink = knx.paramBit(PARAM_blink, PARAM_blink_Shift);
        Serial.print("Parameter Blink: ");
        Serial.println(blink, HEX);

        //read the interval the led should blink
        //Size 8 bit; Offset 1; BitOffset 0
        uint8_t uinterval = knx.paramByte(PARAM_interval);
        Serial.print("Parameter Interval: ");
        Serial.println(uinterval);
        interval = blink ? (uinterval * 10) : -1;
        lastmillis = 0 - interval;

        //register callback if we receive a knx-telegram on ComObject 1
        goLedSwitch.callback(switchCallback);
        //set DatapointType for ComObject 1: 1.001 Switch
        goLedSwitch.dataPointType(DPT_Switch);
        //set DatapointType for ComObject 2: 1.001 Switch
        goLedState.dataPointType(DPT_Switch);
        Serial.println("Callbacks linked");
    } else {
        Serial.println("Nicht konfiguriert!");
    }
        
    //magically start the knx stuff^^
    knx.start();

    //knx.progMode(true);
    Serial.println("Start");
}

void loop()
{
    //kxn should do stuff
    knx.loop();

    //do nothing if not configured
    if (!knx.configured())
        return;

    //do your stuff but be careful!
    //knx.loop should be called at least every 50ms

    //if channel(led) is activated by a knx-telegram
    if(ledActive)
    {
        //-1 if led should not blink
        //>=0 when the led should blink
        if(interval >= 0)
        {
            //check if interval is over
            if(lastmillis + interval < millis())
            {
                lastmillis = millis();
                //toggle led
                setLed(!ledState);
            }
        } else {
            //so the led should not blink
            //then jist turn it on if its off
            if(!ledState)
            {
                //turn led on
                setLed(true);
            }
        }
    } else if(ledState)
    {
        //if it is deactivated but the led is still on: turn it off
        setLed(false);
        Serial.print("halll");
    }
    
}