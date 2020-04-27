#include "lapse.h"
#include "cfg_eeprom.h"

static unsigned long fileIndex = 0;
static unsigned long lapseIndex = 0;
static unsigned long frameInterval = 2000;
static bool mjpeg = true;
static bool lapseRunning = false;
static unsigned long lastFrameDelta = 0;

void setInterval(unsigned long delta)
{
    frameInterval = delta;
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

bool stopLapse()
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
        esp_err_t res = ESP_OK;
        cfg_struct* current_cfg = get_current_cfg();
        setFlashLED(current_cfg->flash);
        delay(300);
        fb = esp_camera_fb_get();
        setFlashLED(0);
        if (!fb)
        {
	        Serial.println("Camera capture failed");
	        return false;
        }

        char path[32];
        sprintf(path, "/lapse%03d/pic%05d.jpg", lapseIndex, fileIndex);
        Serial.println(path);
        if(!writeFile(path, (const unsigned char *)fb->buf, fb->len))
        {
            lapseRunning = false;
            return false;
        }
	    setFlashLED(0);
        fileIndex++;
        esp_camera_fb_return(fb);
    }
    return true;
}
