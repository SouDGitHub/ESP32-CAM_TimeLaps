#include "lapse.h"
#include "cfg_eeprom.h"
#include "flashLED.h"
#include "Arduino.h"
#include "camera.h"
#include <stdio.h>
#include "file.h"
#include "debug.h"
#include "trajectory.h"
static uint32_t fileIndex = 0;
static uint32_t lapseIndex = 0;
static unsigned long frameInterval = 2000;
static bool mjpeg = true;
static bool lapseRunning = false;
static unsigned long lastFrameDelta = 0;

void setTimeLapsInterval(uint16_t delta)
{
    frameInterval = (unsigned long)delta*1000;
}

bool startLapse()
{
    if(lapseRunning) return true;
    fileIndex = 0;
    char path[32];
    for(; lapseIndex < 10000; lapseIndex++)
    {
        sprintf(path, "/lapse%03d", lapseIndex);
        if (!fileExists(path))
        {
            createDir(path);
            lastFrameDelta = 0;
            lapseRunning = true;
            return true;
        }
    }
	return false;
}

void stopLapse()
{
    lapseRunning = false;
}

bool processLapse(unsigned long dt)
{
    if(!lapseRunning) return false;

    lastFrameDelta += dt;
    if(lastFrameDelta >= frameInterval)
    {
        lastFrameDelta -= frameInterval;
        camera_fb_t *fb = NULL;
        cfg_struct* current_cfg = get_current_cfg();
        if(current_cfg->flash){
            setFlashLED(current_cfg->flashThresh);
            delay(FLASH_PRESHOOT_DURATION_MS);
        }
        fb = esp_camera_fb_get();
        setFlashLED(0);
        if (!fb)
        {
	        if(USE_SERIAL) Serial.println("Camera capture failed");
	        return false;
        }

        char path[32];
        sprintf(path, "/lapse%03d/pic%05d.jpg", lapseIndex, fileIndex);
        if(USE_SERIAL) Serial.println(path);
        if(!writeFile(path, (const unsigned char *)fb->buf, fb->len))
        {
            lapseRunning = false;
            return false;
        }
        fileIndex++;
        processX2Trajcetory();
        esp_camera_fb_return(fb);
    }
    return true;
}
