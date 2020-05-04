#include "arduino.h"
#include <WiFi.h>
#include "file.h"
#include "camera.h"
#include "lapse.h"
#include "cfg_eeprom.h"
#include "flashLED.h"
#include "app_httpd.h"
#include "servos.h"
#include "trajectory.h"
#include "debug.h"

#define CONNECT_TIME_OUT_SECOND 10

void setup()
{
	if(USE_SERIAL) Serial.begin(115200);
	if(USE_SERIAL) Serial.setDebugOutput(true);
	if(USE_SERIAL) Serial.println();

	initFileSystem();
	initCfgFromEEPROM();
	initCamera();
	initFlashLED();
	initServos();


	cfg_struct* current_cfg = get_current_cfg();
	setTimeLapsInterval(current_cfg->TimeLapsDelta);
	loadCAMX(current_cfg->AutoStartCFG);
	lockFlashON(current_cfg->forceFlash,current_cfg->forceFlashThresh);
	setServosXPosInt(current_cfg->servoCoord[PWON_POS].xPos);
	setServosYPosInt(current_cfg->servoCoord[PWON_POS].yPos);

	int counter_s = 0;
	if(!current_cfg->StartAPOnly)
	{
		if(USE_SERIAL) Serial.print("\nTrying to connected to SSID = ");
		if(USE_SERIAL) Serial.println(current_cfg->WiFi_ssid);			
		WiFi.begin(current_cfg->WiFi_ssid.c_str(), current_cfg->WiFi_pw.c_str());
		while (WiFi.status() != WL_CONNECTED && counter_s < CONNECT_TIME_OUT_SECOND)
		{
			delay(1000);
			counter_s += 1;
			if(USE_SERIAL) Serial.print(".");
		}
		if (WiFi.status() == WL_CONNECTED)
		{
			if(USE_SERIAL) Serial.print("\nWiFi ready ! Connected to SSID = ");
			if(USE_SERIAL) Serial.println(current_cfg->WiFi_ssid);			
			if(USE_SERIAL) Serial.print("Use 'http://");
			if(USE_SERIAL) Serial.print(WiFi.localIP());
			if(USE_SERIAL) Serial.println("' to connect");
		}
		else{
			WiFi.disconnect();
			if(USE_SERIAL) Serial.print("\nImpossible to connect to SSID = ");
			if(USE_SERIAL) Serial.println(current_cfg->WiFi_ssid);			
		}
	}
	if(current_cfg->StartAPOnly || counter_s == CONNECT_TIME_OUT_SECOND)
	{		
		if(USE_SERIAL) Serial.println("\nStarting AP Mode");
		WiFi.softAP(current_cfg->AP_WiFi_ssid.c_str(), current_cfg->AP_WiFi_pw.c_str());	
		if(USE_SERIAL) Serial.print("WiFi ready ! SSID = ");
		if(USE_SERIAL) Serial.print(current_cfg->AP_WiFi_ssid);
		if(USE_SERIAL) Serial.print("\t Password = ");
		if(USE_SERIAL) Serial.println(current_cfg->AP_WiFi_pw);
		if(USE_SERIAL) Serial.print("Use 'http://");
		if(USE_SERIAL) Serial.print(WiFi.softAPIP());
		if(USE_SERIAL) Serial.println("' to connect");
	}

	startCameraServer();
	if(USE_SERIAL) Serial.println("Camera Ready!");
	
	if(current_cfg->AutoStartTimeLapsStartUp){
		if(current_cfg->useTrajectory){
			if(startTrajectory(current_cfg->servoCoord[START_POS].xPos,current_cfg->servoCoord[START_POS].yPos,current_cfg->servoCoord[MID_POS].xPos,current_cfg->servoCoord[MID_POS].yPos,current_cfg->servoCoord[END_POS].xPos,current_cfg->servoCoord[END_POS].yPos,current_cfg->nbPicTrajectory)){
				if(USE_SERIAL) Serial.println("Trajectory strated");
			}
			else{
				if(USE_SERIAL) Serial.println("Failed to compute trajectory");
			}
		}
		
		if(startLapse()){
			if(USE_SERIAL) Serial.println("Time-Laps strated");
		}
		else{
			if(USE_SERIAL) Serial.println("Failed to start Time-Laps");
		}
	}

}

void loop()
{
	unsigned long t = millis();
	static unsigned long ot = 0;
	unsigned long dt = t - ot;
	ot = t;
	processLapse(dt);
}
