#include "arduino.h"
#define FLASH_PRESHOOT_DURATION_MS 300

extern void initFlashLED();
extern void setFlashLED(uint8_t percent);
extern void lockFlashON(boolean lock, uint8_t percent);