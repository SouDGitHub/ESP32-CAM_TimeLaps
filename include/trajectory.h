#include "arduino.h"

extern bool computeX2Trajectory(int XP1,int YP1,int XP2,int YP2,int XP3,int YP3, long nbSteps);
extern bool startTrajectory(int XP1,int YP1,int XP2,int YP2,int XP3,int YP3, long nbSteps);
extern bool stopTrajectory();
extern bool processX2Trajcetory();