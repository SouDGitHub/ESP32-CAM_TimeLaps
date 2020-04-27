#include "flashLED.h"

static boolean locked = false;
static boolean actualState = false;

void initFlashLED()
{
	locked = false;
	pinMode(LED_PIN,OUTPUT);
	setFlashLED(actualState);
}

void setFlashLED(boolean state)
{
	if (!locked){		
		actualState = state;
		digitalWrite(LED_PIN,actualState);
	}
	else{
		digitalWrite(LED_PIN,actualState);
	}
}

void lockFlashON(boolean lock, boolean state)
{
	locked = false;
	setFlashLED(state);
	locked = lock;
}