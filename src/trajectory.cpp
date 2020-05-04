// XP1 YP1 = START COORDINATES
// XP2 YP2 = MIDDLE COORDINATES
// XP3 YP3 = END COORDINATES

#include "trajectory.h"
#include "servos.h"
#include "debug.h"

#define DISABLE_SERVO_DURING_PICS false
#define DELAY_LNG_STEP 500
#define DELAY_SHORT_STEP 100

static float X2_A = 0;
static float X2_B = 0;
static float X2_C = 0;
static bool trajectoryRunning = false;

static float xPos = 0;
static float yPos = 0;
static float xStep = 0;

static float xStop = 0;

bool computeX2Trajectory(int XP1,int YP1,int XP2,int YP2,int XP3,int YP3, long nbSteps){
	if(USE_SERIAL) Serial.printf("computetrajectory \n");
	if (XP1==XP2)
		return false;
	
	float K1 = (float)(YP1-YP2)/(XP1*XP1-XP2*XP2);
	float K2 = (float)(XP1-XP2)/(XP1*XP1-XP2*XP2);

	float test = (float)XP3-XP3*XP3*K2-XP1+XP1*XP1*K2;
	if(test==0)
		return false;

	X2_B = (float)(YP3-XP3*XP3*K1-YP1+XP1*XP1*K1)/(XP3-XP3*XP3*K2-XP1+XP1*XP1*K2);
	X2_A = (float)K1-K2*X2_B;
	X2_C = (float)YP1-XP1*XP1*K1-X2_B*(XP1-XP1*XP1*K2);

	xStep = (float)(XP3-XP1)/(nbSteps-1);
	xStop = (float)XP3;

	if(USE_SERIAL) Serial.printf("XP1 %d \tYP1 %d \tXP2 %d \tYP2 %d \tXP3 %d \tYP3 %d \tnbSteps %ld\t X2_A %.2f\t X2_B %.2f\t X2_C %.2f \txStep %.2f\txStop %.2f\t \n",XP1, YP1, XP2, YP2, XP3, YP3, nbSteps,X2_A,X2_B,X2_C,xStep,xStop);
	return true;
}

float getYtargetX2Curve(float xTarget){
	float yTarget = xTarget*xTarget*X2_A + xTarget*X2_B + X2_C; 
	return yTarget;
}

bool processX2Trajcetory(){
	if (trajectoryRunning){
		xPos = xPos+xStep;
		yPos = getYtargetX2Curve(xPos);
		setServosXPosFloat(xPos);
		setServosYPosFloat(yPos);
		if(DISABLE_SERVO_DURING_PICS){
			delay(DELAY_SHORT_STEP);
			stopServosCMD();
		}
		if(USE_SERIAL) Serial.printf("processTrajectory xPos %.2f\t, yPos %.2f\n",xPos,yPos);
		if(xStep>0){
			if(xPos>=xStop){
				trajectoryRunning = false;
				if(USE_SERIAL) Serial.println("processTrajectory xPos>=xStop");
			}
		}
		else{
			if(xPos<=xStop){
				trajectoryRunning = false;
				if(USE_SERIAL) Serial.println("processTrajectory xPos<=xStop");
			}
		}
	}
	return trajectoryRunning;
}

bool startTrajectory(int XP1,int YP1,int XP2,int YP2,int XP3,int YP3, long nbSteps){
	if(!computeX2Trajectory(XP1, YP1, XP2, YP2, XP3, YP3, nbSteps)){
			trajectoryRunning = false;
	}
	else{
		xPos = XP1;
		yPos = getYtargetX2Curve(xPos);
		setServosXPosFloat(xPos);
		setServosYPosFloat(yPos);
		if(DISABLE_SERVO_DURING_PICS){
			delay(DELAY_LNG_STEP);
			stopServosCMD();
		}
		trajectoryRunning = true;
	}
	return trajectoryRunning;
}

bool stopTrajectory(){
	trajectoryRunning=false;
	stopServosCMD();
	return trajectoryRunning;
}