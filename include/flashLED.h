#include "arduino.h"

#define LED_PIN 4
#define LED_PWM_CHANNEL 1
#define LED_PWM_FREQ 10000
#define LED_PWM_RES 8

extern void initFlashLED();
extern void setFlashLED(boolean state);
extern void lockFlashON(boolean lock, boolean state);