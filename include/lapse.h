//#pragma once
#include "flashLED.h"
#include "Arduino.h"
#include "camera.h"
#include <stdio.h>
#include "file.h"

extern void setInterval(unsigned long delta);
extern bool startLapse();
extern bool stopLapse();
extern bool processLapse(unsigned long dt);
