#include "arduino.h"

extern void setTimeLapsInterval(uint16_t delta);
extern bool startLapse();
extern void stopLapse();
extern bool processLapse(unsigned long dt);