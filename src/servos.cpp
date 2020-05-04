#include "servos.h"
#include "esp32-hal-ledc.h"
#include "debug.h"

#define SERVO_X_PIN 3
#define SERVO_Y_PIN 1
#define PWM_X_CHANNEL 4	//CHANNEL 0 & 1 ALREADY USED BY THE CAMERA
#define PWM_Y_CHANNEL 5	//CHANNEL 0 & 1 ALREADY USED BY THE CAMERA
#define PWM_FREQUENCY_HZ 50
#define PWM_WIDTH_BITS 16
#define PWM_WIDTH_MAX_N 65535
#define PWM_WIDTH_SERVO_M90 2258 // VERIFIED WITH OSCILLOSCOPE : TIMER = 3277 => 1ms High pulse : (1ms * [2^16 -1])/ (1/50 *1000) = 1*65535*50/1000 my servo need correction (3.3V logic signal)
#define PWM_WIDTH_SERVO_P90 8336 // VERIFIED WITH OSCILLOSCOPE : TIMER = 6553 => 2ms High pulse : (2ms * [2^16 -1])/ (1/50 *1000) = 2*65535*50/1000 my servo need correction (3.3V logic signal)
#define COEF_ACCURACY 100

static long xPos = 0;
static long yPos = 0;

long mapfloat(float x, long inMin, long inMax, long outMin, long outMax,boolean clip);

void initServos()
{
	if(!USE_SERIAL) {
	ledcSetup(PWM_X_CHANNEL, PWM_FREQUENCY_HZ, PWM_WIDTH_BITS);
	ledcAttachPin(SERVO_X_PIN, PWM_X_CHANNEL);
	
	ledcSetup(PWM_Y_CHANNEL, PWM_FREQUENCY_HZ, PWM_WIDTH_BITS);
	ledcAttachPin(SERVO_Y_PIN, PWM_Y_CHANNEL);
	}
}

void setServosXPosInt(int pos)
{
	
	xPos = (long)pos;
	if(xPos>=SERVO_POS_MAX)
		xPos = SERVO_POS_MAX;
	else if (xPos<=SERVO_POS_MIN)
		xPos = SERVO_POS_MIN;

	xPos = map(xPos*COEF_ACCURACY,SERVO_POS_MIN*COEF_ACCURACY,SERVO_POS_MAX*COEF_ACCURACY,PWM_WIDTH_SERVO_M90,PWM_WIDTH_SERVO_P90);
	if(USE_SERIAL) Serial.printf("Servos int X Target %d, PWM Cmd %lu\n",pos, xPos);
	if(!USE_SERIAL) {
	ledcWrite(PWM_X_CHANNEL, xPos);
	}
}

void setServosYPosInt(int pos)
{
	yPos = (long)pos;
	if(yPos>=SERVO_POS_MAX)
		yPos = SERVO_POS_MAX;
	else if (yPos<=SERVO_POS_MIN)
		yPos = SERVO_POS_MIN;

	yPos = map(yPos*COEF_ACCURACY,SERVO_POS_MIN*COEF_ACCURACY,SERVO_POS_MAX*COEF_ACCURACY,PWM_WIDTH_SERVO_M90,PWM_WIDTH_SERVO_P90);
	if(USE_SERIAL) Serial.printf("Servos int Y Target %d, PWM Cmd %lu\n",pos, yPos);
	if(!USE_SERIAL) {
	ledcWrite(PWM_Y_CHANNEL, yPos);
	}
}

void setServosXPosFloat(float pos)
{
	xPos = mapfloat(pos,SERVO_POS_MIN,SERVO_POS_MAX,PWM_WIDTH_SERVO_M90,PWM_WIDTH_SERVO_P90,true);
	if(USE_SERIAL) Serial.printf("Servos float X Target %.2f, PWM Cmd %lu\n",pos, xPos);
	if(!USE_SERIAL) {
	ledcWrite(PWM_X_CHANNEL, xPos);
	}
}

void setServosYPosFloat(float pos)
{
	yPos = mapfloat(pos,SERVO_POS_MIN,SERVO_POS_MAX,PWM_WIDTH_SERVO_M90,PWM_WIDTH_SERVO_P90,true);
	if(USE_SERIAL) Serial.printf("Servos float Y Target %.2f, PWM Cmd %lu\n",pos, yPos);
	if(!USE_SERIAL) {
	ledcWrite(PWM_Y_CHANNEL, yPos);
	}
}

void stopServosCMD()
{
	if(!USE_SERIAL) {
	ledcWrite(PWM_Y_CHANNEL, 0);
	ledcWrite(PWM_X_CHANNEL, 0);
	}
	if(USE_SERIAL) Serial.printf("Servos XY stop\n");
}

long mapfloat(float x, long inMin, long inMax, long outMin, long outMax,boolean clip){
	float tmp = (x - (float)inMin)  *  ((float)outMax-(float)outMin)  /  ((float)inMax-(float)inMin) + (float)outMin ;
	if(clip){
		if(tmp>(float)outMax)
			tmp = (float)outMax;
		else if(tmp<(float)outMin)
			tmp = (float)outMin;
	}
	return (long)tmp;
}