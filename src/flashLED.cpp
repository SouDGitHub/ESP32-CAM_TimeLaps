#include "flashLED.h"
#include "esp32-hal-ledc.h"
#include "debug.h"

#define LED_PIN 4
#define PWM_CHANNEL 2	//CHANNEL 0 & 1 ALREADY USED BY THE CAMERA
#define PWM_FREQUENCY_HZ 5000 //LOWER FREQUENCY MIGHT GENERATES HIGHLIGHTED LINES ON THE PICTURE
#define PWM_WIDTH_BITS 10
#define PWM_WIDTH_MAX_N 1023

static boolean locked = false;
static uint8_t actualState = 0;

void initFlashLED()
{
	locked = false;
	ledcSetup(PWM_CHANNEL, PWM_FREQUENCY_HZ, PWM_WIDTH_BITS);
	ledcAttachPin(LED_PIN, PWM_CHANNEL);
	lockFlashON(locked,actualState);
}

void setFlashLED(uint8_t percent)
{
	if (!locked){		
		actualState = percent;
	}
	ledcWrite(PWM_CHANNEL, map(actualState,0,100,0,PWM_WIDTH_MAX_N));
}

void lockFlashON(boolean lock, uint8_t percent)
{
	locked = false;
	if(lock){
		setFlashLED(percent);
	}
	else{
		setFlashLED(0);
	}
	locked = lock;
}