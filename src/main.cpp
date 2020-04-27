#include "arduino.h"
#include <WiFi.h>
#include "file.h"
#include "camera.h"
#include "lapse.h"
#include "cfg_eeprom.h"
#include "flashLED.h"
#include "app_httpd.h"

#define CONNECT_TIME_OUT_SECOND 10

void setup()
{
	Serial.begin(115200);
	Serial.setDebugOutput(true);
	Serial.println();

	initFileSystem();
	initFlashLED();
	initCfgFromEEPROM();
	initCamera();

	cfg_struct* current_cfg = get_current_cfg();
	setInterval(current_cfg->TimeLapsDelta);
	loadCAMX(current_cfg->AutoStartCFG);
	lockFlashON(current_cfg->forceFlash,current_cfg->forceFlash);

	int counter_s = 0;
	if(!current_cfg->StartAPOnly)
	{
		Serial.print("\nTrying to connected to SSID = ");
		Serial.println(current_cfg->WiFi_ssid);			
		WiFi.begin(current_cfg->WiFi_ssid.c_str(), current_cfg->WiFi_pw.c_str());
		while (WiFi.status() != WL_CONNECTED && counter_s < CONNECT_TIME_OUT_SECOND)
		{
			delay(1000);
			counter_s += 1;
			Serial.print(".");
		}
		if (WiFi.status() == WL_CONNECTED)
		{
			Serial.print("\nWiFi ready ! Connected to SSID = ");
			Serial.println(current_cfg->WiFi_ssid);			
			Serial.print("Use 'http://");
			Serial.print(WiFi.localIP());
			Serial.println("' to connect");
		}
		else{
			WiFi.disconnect();
			Serial.print("\nImpossible to connect to SSID = ");
			Serial.println(current_cfg->WiFi_ssid);			
		}
	}
	if(current_cfg->StartAPOnly || counter_s == CONNECT_TIME_OUT_SECOND)
	{		
		Serial.println("\nStarting AP Mode");
		WiFi.softAP(current_cfg->AP_WiFi_ssid.c_str(), current_cfg->AP_WiFi_pw.c_str());	
		Serial.print("WiFi ready ! SSID = ");
		Serial.print(current_cfg->AP_WiFi_ssid);
		Serial.print("\t Password = ");
		Serial.println(current_cfg->AP_WiFi_pw);
		Serial.print("Use 'http://");
		Serial.print(WiFi.softAPIP());
		Serial.println("' to connect");
	}

	startCameraServer();
	Serial.println("Camera Ready!");
	
	if(current_cfg->AutoStartTimeLapsStartUp)
	{
		if(startLapse())
			Serial.println("Time-Laps strated");
		else
			Serial.println("Failed to start Time-Laps");
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
