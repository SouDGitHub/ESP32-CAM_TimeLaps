#include "arduino.h"

#define SERVO_POS_MIN -90
#define SERVO_POS_MAX +90

extern void initServos();

extern void setServosXPosInt(int pos);
extern void setServosYPosInt(int pos);
extern void setServosXPosFloat(float pos);
extern void setServosYPosFloat(float pos);
extern void stopServosCMD();