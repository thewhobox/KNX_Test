#include <Arduino.h>
#include "OpenKNX.h"
#include "BlinkModule.h"

void setup()
{
	SERIAL_DEBUG.begin(115200);
	delay(10000);
	SERIAL_DEBUG.println("Starte");

	const uint8_t firmwareRevision = 0;
    openknx.init(firmwareRevision);
    openknx.addModule(1, new BlinkModule());
    openknx.setup();
}

void loop()
{
	openknx.loop();
}