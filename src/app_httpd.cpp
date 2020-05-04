// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "file.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "arduino.h"
#include "lapse.h"
#include "file.h"
#include <WiFi.h>
#include "flashLED.h"
#include "cfg_eeprom.h"
#include "FS.h"
#include "SD_MMC.h"
#include "servos.h"
#include "trajectory.h"
#include "debug.h"

#include "app_httpd.h"

const char *indexHtml =
#include "index.h"
	;

typedef struct
{
	httpd_req_t *req;
	size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static int joyXpos = 0;
static int joyYpos = 0;
#define JOYSTICK_DIVISEUR 10

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len)
{
	jpg_chunking_t *j = (jpg_chunking_t *)arg;
	if (!index)
	{
		j->len = 0;
	}
	if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK)
	{
		return 0;
	}
	j->len += len;
	return len;
}

static esp_err_t capture_handler(httpd_req_t *req)
{
	camera_fb_t *fb = NULL;
	esp_err_t res = ESP_OK;

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
		httpd_resp_send_500(req);
		return ESP_FAIL;
	}

	httpd_resp_set_type(req, "image/jpeg");
	httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

	size_t fb_len = 0;
	if (fb->format == PIXFORMAT_JPEG)
	{
		fb_len = fb->len;
		res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
	}
	else
	{
		jpg_chunking_t jchunk = {req, 0};
		res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk) ? ESP_OK : ESP_FAIL;
		httpd_resp_send_chunk(req, NULL, 0);
		fb_len = jchunk.len;
	}
	esp_camera_fb_return(fb);
	return res;
}

static esp_err_t capture_save_handler(httpd_req_t *req)
{
	camera_fb_t *fb = NULL;
	esp_err_t res = ESP_OK;
			
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
		httpd_resp_send_500(req);
		return ESP_FAIL;
	}

	char path[20];
	int fileIndex=0;
	sprintf(path, "/snap");
	if (!fileExists(path))
		createDir(path);
	sprintf(path, "/snap/%08d.jpg", fileIndex);
	while(fileExists(path))
	{
		fileIndex++;
		sprintf(path, "/snap/%08d.jpg", fileIndex);
	}
	if(USE_SERIAL) Serial.println(path);
	writeFile(path, (const unsigned char *)fb->buf, fb->len);

	httpd_resp_set_type(req, "image/jpeg");
	httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

	size_t fb_len = 0;
	if (fb->format == PIXFORMAT_JPEG)
	{
		fb_len = fb->len;
		res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
	}
	else
	{
		jpg_chunking_t jchunk = {req, 0};
		res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk) ? ESP_OK : ESP_FAIL;
		httpd_resp_send_chunk(req, NULL, 0);
		fb_len = jchunk.len;
	}
	esp_camera_fb_return(fb);
	
	return res;
}

static esp_err_t streamHandler(httpd_req_t *req)
{
	camera_fb_t *fb = NULL;
	esp_err_t res = ESP_OK;
	char *part_buf[64];

	res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
	if (res != ESP_OK)
		return res;
	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
	
	do
	{
		fb = esp_camera_fb_get();
		if(USE_SERIAL) Serial.print("Frame size ");
		if(USE_SERIAL) Serial.println(fb->len);
		if (!fb)
		{
			if(USE_SERIAL) Serial.println("Camera capture failed");
			continue;
		}
		size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, fb->len);
		res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
		if (res == ESP_OK)
			res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
		if (res == ESP_OK)
			res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
		esp_camera_fb_return(fb);
	} while(res == ESP_OK);
	return res;
}

static esp_err_t cmd_handler(httpd_req_t *req)
{
	char *buf;
	size_t buf_len;
	char variable[32] = {0,};
	char value[32] = {0,};
	char value2[32] = {0,};

	buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1)
	{
		buf = (char *)malloc(buf_len);
		if (!buf)
		{
			httpd_resp_send_500(req);
			return ESP_FAIL;
		}
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
		{
			if(USE_SERIAL) Serial.println(buf);
			if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
				httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK )
			{
				if (!strcmp(variable, "joyXYVal"))
				{
					httpd_query_key_value(buf, "val2", value2, sizeof(value2));
				}
			}
			else
			{
				free(buf);
				httpd_resp_send_404(req);
				return ESP_FAIL;
			}
		}
		else
		{
			free(buf);
			httpd_resp_send_404(req);
			return ESP_FAIL;
		}
		free(buf);
	}
	else
	{
		httpd_resp_send_404(req);
		return ESP_FAIL;
	}

	cfg_struct* current_cfg = get_current_cfg();
	int val=0;
	int val2=0; 
			//////COMMAND JOYSTICK//////	
	if (!strcmp(variable, "joyXYVal"))
	{
		val = atoi(value);
		val2 = atoi(value2);
		if(current_cfg->servoXInv)
			joyXpos = joyXpos - val/JOYSTICK_DIVISEUR;
		else
			joyXpos = joyXpos + val/JOYSTICK_DIVISEUR;
			
		if(current_cfg->servoYInv)
			joyYpos = joyYpos - val2/JOYSTICK_DIVISEUR;
		else
			joyYpos = joyYpos + val2/JOYSTICK_DIVISEUR;

		if(joyXpos>=SERVO_POS_MAX)
			joyXpos = SERVO_POS_MAX;
		else if (joyXpos<=SERVO_POS_MIN)
			joyXpos = SERVO_POS_MIN;

		if(joyYpos>=SERVO_POS_MAX)
			joyYpos = SERVO_POS_MAX;
		else if (joyYpos<=SERVO_POS_MIN)
			joyYpos = SERVO_POS_MIN;

		setServosXPosInt(joyXpos);
		setServosYPosInt(joyYpos);

		static char json_response[1024];
		char myLabel[35]="";
		char *p = json_response;
		*p++ = '{';
		sprintf(myLabel,"X pos = %d°, Y pos = %d°",joyXpos,joyYpos);
		p += sprintf(p, "\"ServoPos\":\"%s\"", myLabel);
		*p++ = '}';
		*p++ = 0;
		httpd_resp_set_type(req, "application/json");
		httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
		if(USE_SERIAL) Serial.println(json_response);
		return httpd_resp_send(req, json_response, strlen(json_response));
	}

	int res = 0;
	sensor_t *s = esp_camera_sensor_get();
	
			//////COMMAND BUTTON//////
	if (!strcmp(variable, "ButtonCtrl"))
	{
		if (!strcmp(value, "savejoystart")){
			current_cfg->servoCoord[START_POS].xPos = joyXpos;
			current_cfg->servoCoord[START_POS].yPos = joyYpos;
		}
		else if (!strcmp(value, "savejoymid")){
			current_cfg->servoCoord[MID_POS].xPos = joyXpos;
			current_cfg->servoCoord[MID_POS].yPos = joyYpos;
		}
		else if (!strcmp(value, "savejoyend")){
			current_cfg->servoCoord[END_POS].xPos = joyXpos;
			current_cfg->servoCoord[END_POS].yPos = joyYpos;
		}
		else if (!strcmp(value, "gojoystart")){	
			joyXpos = current_cfg->servoCoord[START_POS].xPos;
			joyYpos = current_cfg->servoCoord[START_POS].yPos;
			setServosXPosInt(joyXpos);
			setServosYPosInt(joyYpos);
		}
		else if (!strcmp(value, "gojoymid")){	
			joyXpos = current_cfg->servoCoord[MID_POS].xPos;
			joyYpos = current_cfg->servoCoord[MID_POS].yPos;
			setServosXPosInt(joyXpos);
			setServosYPosInt(joyYpos);
		}
		else if (!strcmp(value, "gojoyend")){	
			joyXpos = current_cfg->servoCoord[END_POS].xPos;
			joyYpos = current_cfg->servoCoord[END_POS].yPos;
			setServosXPosInt(joyXpos);
			setServosYPosInt(joyYpos);
		}
		else if (!strcmp(value, "savePowerOnPos")){
			current_cfg->servoCoord[PWON_POS].xPos = joyXpos;
			current_cfg->servoCoord[PWON_POS].yPos = joyYpos;
		}
		else if (!strcmp(value, "simultrajectory")){
			boolean test = startTrajectory(current_cfg->servoCoord[START_POS].xPos,current_cfg->servoCoord[START_POS].yPos,current_cfg->servoCoord[MID_POS].xPos,current_cfg->servoCoord[MID_POS].yPos,current_cfg->servoCoord[END_POS].xPos,current_cfg->servoCoord[END_POS].yPos,current_cfg->nbPicTrajectory);
			if (test){
				long tempo_simul = 30000/current_cfg->nbPicTrajectory/2;
				if(tempo_simul<20)
					tempo_simul = 20;

				for (long i=0; i<=current_cfg->nbPicTrajectory; i++){
					setFlashLED(20);
					processX2Trajcetory();
					delay(tempo_simul);
					setFlashLED(0);
					delay(tempo_simul);
				}
			}
		}
		else if (!strcmp(value, "savecfg1"))
			storeCAMX(1);
		else if (!strcmp(value, "savecfg2"))
			storeCAMX(2);
		else if (!strcmp(value, "savecfg3"))
			storeCAMX(3);
		else if (!strcmp(value, "loadcfg1"))
			loadCAMX(1);
		else if (!strcmp(value, "loadcfg2"))
			loadCAMX(2);
		else if (!strcmp(value, "loadcfg3"))
			loadCAMX(3);
		else if (!strcmp(value, "loadcfgdefault"))
			loadCAMX(0);
		else if (!strcmp(value, "ESPreset")){
			//Reset - respond OK for the browser to not reset loop because it re-ask the reset at each next connection...
			httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
			httpd_resp_send(req, NULL, 0);
			if(USE_SERIAL) Serial.println("\nRESET ESP32");
			delay(500);
			ESP.restart();
		}
		else if  (!strcmp(value, "toggle-lapsetrue")){
			stopTrajectory();
			stopLapse();
		}
		else if  (!strcmp(value, "toggle-lapsefalse")){
			if(current_cfg->useTrajectory){
				if(startTrajectory(current_cfg->servoCoord[START_POS].xPos,current_cfg->servoCoord[START_POS].yPos,current_cfg->servoCoord[MID_POS].xPos,current_cfg->servoCoord[MID_POS].yPos,current_cfg->servoCoord[END_POS].xPos,current_cfg->servoCoord[END_POS].yPos,current_cfg->nbPicTrajectory)){
					startLapse();
				}
			}
			else{
				startLapse();
			}
		}
		else if  (!strcmp(value, "WiFiStop")) 
		{
			//StopWifi - respond OK for the browser to not turn off wifi in loop because it re-ask at each next connection...
			httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
			httpd_resp_send(req, NULL, 0);
			delay(500);
			if(USE_SERIAL) Serial.printf("FreeHeap WiFi ON: %u octets\n",ESP.getFreeHeap());
			WiFi.disconnect(true);
  			WiFi.mode(WIFI_OFF);
			btStop();
			delay(1000);
			if(USE_SERIAL) Serial.printf("FreeHeap WiFi OFF: %u octets\n",ESP.getFreeHeap());
			if(USE_SERIAL) Serial.println("WiFi is OFF. Reset ESP32 tu turn ON WiFi again");
			//return;
		}
		else if  (!strcmp(value, "SdCardInit")){
			initFileSystem();
			initFlashLED();
			initServos();
		}
		else if  (!strcmp(value, "refreshInfo"))
			res=0;
		else if  (!strcmp(value, "EepromRestoreToDefault")){
			storeDefaultCfgToEEPROM();
			loadCAMX(0);
		}
		else
			res = 1;
	}
			//////STRING VALUE//////
	else if (!strcmp(variable, "WiFiAPSSID"))
		current_cfg->AP_WiFi_ssid = value;
	else if(!strcmp(variable, "WiFiAPPW"))
		current_cfg->AP_WiFi_pw = value;
	else if(!strcmp(variable, "WiFiSSID"))
		current_cfg->WiFi_ssid = value;
	else if(!strcmp(variable, "WiFiPW"))
		current_cfg->WiFi_pw = value;
	else
	{
		val = atoi(value);
			//////BOOL OR INTEGER VALUE//////
		if (!strcmp(variable, "framesize"))
		{
			if (s->pixformat == PIXFORMAT_JPEG)
				res = s->set_framesize(s, (framesize_t)val);
		}
		else if (!strcmp(variable, "quality"))
			res = s->set_quality(s, val);
		else if (!strcmp(variable, "contrast"))
			res = s->set_contrast(s, val);
		else if (!strcmp(variable, "brightness"))
			res = s->set_brightness(s, val);
		else if (!strcmp(variable, "saturation"))
			res = s->set_saturation(s, val);
		else if (!strcmp(variable, "gainceiling"))
			res = s->set_gainceiling(s, (gainceiling_t)val);
		else if (!strcmp(variable, "colorbar"))
			res = s->set_colorbar(s, val);
		else if (!strcmp(variable, "awb"))
			res = s->set_whitebal(s, val);
		else if (!strcmp(variable, "agc"))
			res = s->set_gain_ctrl(s, val);
		else if (!strcmp(variable, "aec"))
			res = s->set_exposure_ctrl(s, val);
		else if (!strcmp(variable, "hmirror"))
			res = s->set_hmirror(s, val);
		else if (!strcmp(variable, "vflip"))
			res = s->set_vflip(s, val);
		else if (!strcmp(variable, "agc_gain"))
			res = s->set_agc_gain(s, val);
		else if (!strcmp(variable, "aec2"))
			res = s->set_aec2(s, val);
		else if (!strcmp(variable, "aec_value"))
			res = s->set_aec_value(s, val);
		else if (!strcmp(variable, "dcw"))
			res = s->set_dcw(s, val);
		else if (!strcmp(variable, "bpc"))
			res = s->set_bpc(s, val);
		else if (!strcmp(variable, "wpc"))
			res = s->set_wpc(s, val);
		else if (!strcmp(variable, "raw_gma"))
			res = s->set_raw_gma(s, val);
		else if (!strcmp(variable, "lenc"))
			res = s->set_lenc(s, val);
		else if (!strcmp(variable, "special_effect"))
			res = s->set_special_effect(s, val);
		else if (!strcmp(variable, "awb_gain"))
			res = s->set_awb_gain(s, val);
		else if (!strcmp(variable, "wb_mode"))
			res = s->set_wb_mode(s, val);
		else if (!strcmp(variable, "ae_level"))
			res = s->set_ae_level(s, val);
		else if (!strcmp(variable, "interval"))
		{
			current_cfg->TimeLapsDelta = val;
			setTimeLapsInterval(val);
		}
		else if (!strcmp(variable, "nbPicTraj"))
		{
			current_cfg->nbPicTrajectory = val;
		}
		else if (!strcmp(variable,"WiFiAPOnly"))
		{
			current_cfg->StartAPOnly = val;
		}
		else if (!strcmp(variable,"autostart"))
		{
			current_cfg->AutoStartTimeLapsStartUp = val;
		}
		else if (!strcmp(variable,"autostartcfg"))
		{
			current_cfg->AutoStartCFG = val;
		}
		else if (!strcmp(variable,"forceFlash")){
			current_cfg->forceFlash = val;
			lockFlashON(current_cfg->forceFlash, current_cfg->forceFlashThresh);
		}
		else if (!strcmp(variable,"forceFlashThresh")){
			current_cfg->forceFlashThresh = (uint8_t)val;
			lockFlashON(current_cfg->forceFlash, current_cfg->forceFlashThresh);
		}
		else if (!strcmp(variable,"flash")){
			current_cfg->flash = val;
		}
		else if (!strcmp(variable,"flashThresh")){
			current_cfg->flashThresh = val;
		}
		else if (!strcmp(variable,"joyinvX")){
			current_cfg->servoXInv = val;
		}
		else if (!strcmp(variable,"joyinvY")){
			current_cfg->servoYInv = val;
		}
		else if (!strcmp(variable,"useTrajectory")){
			current_cfg->useTrajectory = val;
		}
		
		else
		{
			res = 1;
		}
	}
	
	if(USE_SERIAL) Serial.printf("RES = %d\n", res);
	if (res==1){
		return httpd_resp_send_500(req);
	}
	else if(res==0){
		storeCfgToEEPROM();
	}

	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
	return httpd_resp_send(req, NULL, 0);
}

static esp_err_t status_handler(httpd_req_t *req)
{
	static char json_response[1024];

	sensor_t *s = esp_camera_sensor_get();
	cfg_struct* current_cfg = get_current_cfg();

	char myLabel[50]="";

	char *p = json_response;
	*p++ = '{';

	p += sprintf(p, "\"framesize\":%u,", s->status.framesize);
	p += sprintf(p, "\"quality\":%u,", s->status.quality);
	p += sprintf(p, "\"brightness\":%d,", s->status.brightness);
	p += sprintf(p, "\"contrast\":%d,", s->status.contrast);
	p += sprintf(p, "\"saturation\":%d,", s->status.saturation);
	p += sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
	p += sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
	p += sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
	p += sprintf(p, "\"awb\":%u,", s->status.awb);
	p += sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
	p += sprintf(p, "\"aec\":%u,", s->status.aec);
	p += sprintf(p, "\"aec2\":%u,", s->status.aec2);
	p += sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
	p += sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
	p += sprintf(p, "\"agc\":%u,", s->status.agc);
	p += sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
	p += sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
	p += sprintf(p, "\"bpc\":%u,", s->status.bpc);
	p += sprintf(p, "\"wpc\":%u,", s->status.wpc);
	p += sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
	p += sprintf(p, "\"lenc\":%u,", s->status.lenc);
	p += sprintf(p, "\"vflip\":%u,", s->status.vflip);
	p += sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
	p += sprintf(p, "\"dcw\":%u,", s->status.dcw);
	p += sprintf(p, "\"colorbar\":%u,", s->status.colorbar);
	p += sprintf(p, "\"interval\":%u,", current_cfg->TimeLapsDelta);
	p += sprintf(p, "\"nbPicTraj\":%u,", current_cfg->nbPicTrajectory);
	p += sprintf(p, "\"autostart\":%u,", current_cfg->AutoStartTimeLapsStartUp);
	p += sprintf(p, "\"autostartcfg\":%u,", current_cfg->AutoStartCFG);
	p += sprintf(p, "\"WiFiAPSSID\":\"%s\",", current_cfg->AP_WiFi_ssid.c_str());
	p += sprintf(p, "\"WiFiAPPW\":\"%s\",", current_cfg->AP_WiFi_pw.c_str());
	p += sprintf(p, "\"WiFiSSID\":\"%s\",", current_cfg->WiFi_ssid.c_str());
	p += sprintf(p, "\"WiFiPW\":\"%s\",", current_cfg->WiFi_pw.c_str());
	p += sprintf(p, "\"WiFiAPOnly\":%u,", current_cfg->StartAPOnly);
	p += sprintf(p, "\"forceFlash\":%u,", current_cfg->forceFlash);
	p += sprintf(p, "\"forceFlashThresh\":%u,", current_cfg->forceFlashThresh);
	p += sprintf(p, "\"flash\":%u,", current_cfg->flash);
	p += sprintf(p, "\"flashThresh\":%u,", current_cfg->flashThresh);
	p += sprintf(p, "\"joyinvX\":%u,", current_cfg->servoXInv);
	p += sprintf(p, "\"joyinvY\":%u,", current_cfg->servoYInv);
	p += sprintf(p, "\"useTrajectory\":%u,", current_cfg->useTrajectory);
	
		uint8_t cardType = SD_MMC.cardType();
		if (cardType == CARD_NONE)
			sprintf(myLabel,"No SD card");
		if (cardType == CARD_MMC)
			sprintf(myLabel,"SD Card type: MMC");
		else if (cardType == CARD_SD)
			sprintf(myLabel,"SD Card type: SDSC");
		else if (cardType == CARD_SDHC)
			sprintf(myLabel,"SD Card type: SDHC");
		else
			sprintf(myLabel,"SD CARD UNKNOWN");
	p += sprintf(p, "\"SdCardType\":\"%s\",", myLabel);
		
		uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
		sprintf(myLabel,"SD Size: %lluMB",cardSize);
	p += sprintf(p, "\"SdCardSize\":\"%s\",", myLabel);
	
		sprintf(myLabel,"SD Total space: %lluMB",SD_MMC.totalBytes() / (1024 * 1024));
	p += sprintf(p, "\"SdCardTotalSpace\":\"%s\",", myLabel);

		sprintf(myLabel,"SD Used space: %lluMB",SD_MMC.usedBytes() / (1024 * 1024));
	p += sprintf(p, "\"SdCardUsedSpace\":\"%s\",", myLabel);

		sprintf(myLabel,"SD Free space: %lluMB",(SD_MMC.totalBytes()-SD_MMC.usedBytes()) / (1024 * 1024));
	p += sprintf(p, "\"SdCardFreeSpace\":\"%s\",", myLabel);

		sprintf(myLabel,"EEPROM uses %u/%u octets",current_cfg->eepromUsed,EEPROM_SIZE);
	p += sprintf(p, "\"EepromUsage\":\"%s\",", myLabel);
		
		sprintf(myLabel,"FreeHeap: %u octets",ESP.getFreeHeap());
	p += sprintf(p, "\"FreeHeap\":\"%s\",", myLabel);
	
		sprintf(myLabel,"X pos = %d°, Y pos = %d°",joyXpos,joyYpos);
	p += sprintf(p, "\"ServoPos\":\"%s\"", myLabel);
	*p++ = '}';
	*p++ = 0;
	httpd_resp_set_type(req, "application/json");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
	if(USE_SERIAL) Serial.println(json_response);
	return httpd_resp_send(req, json_response, strlen(json_response));
}

static esp_err_t index_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "text/html");
	unsigned long l = strlen(indexHtml);
	return httpd_resp_send(req, (const char *)indexHtml, l);
}

void startCameraServer()
{
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	httpd_uri_t index_uri = {
		.uri = "/",
		.method = HTTP_GET,
		.handler = index_handler,
		.user_ctx = NULL};

	httpd_uri_t status_uri = {
		.uri = "/status",
		.method = HTTP_GET,
		.handler = status_handler,
		.user_ctx = NULL};

	httpd_uri_t cmd_uri = {
		.uri = "/control",
		.method = HTTP_GET,
		.handler = cmd_handler,
		.user_ctx = NULL};

	httpd_uri_t capture_uri = {
		.uri = "/capture",
		.method = HTTP_GET,
		.handler = capture_handler,
		.user_ctx = NULL};

	httpd_uri_t capture_save_uri = {
		.uri = "/capture_save",
		.method = HTTP_GET,
		.handler = capture_save_handler,
		.user_ctx = NULL};

	httpd_uri_t stream_uri = {
		.uri = "/stream",
		.method = HTTP_GET,
		.handler = streamHandler,
		.user_ctx = NULL};		

	if(USE_SERIAL) Serial.printf("Starting web server on port: '%d'\n", config.server_port);
	if (httpd_start(&camera_httpd, &config) == ESP_OK)
	{
		httpd_register_uri_handler(camera_httpd, &index_uri);
		httpd_register_uri_handler(camera_httpd, &cmd_uri);
		httpd_register_uri_handler(camera_httpd, &status_uri);
		httpd_register_uri_handler(camera_httpd, &capture_uri);
		httpd_register_uri_handler(camera_httpd, &capture_save_uri);
		httpd_register_uri_handler(camera_httpd, &stream_uri);
	}

	config.server_port += 1;
	config.ctrl_port += 1;
	if(USE_SERIAL) Serial.printf("Starting stream server on port: '%d'\n", config.server_port);

	cfg_struct* current_cfg = get_current_cfg();
	joyXpos = current_cfg->servoCoord[PWON_POS].xPos;
	joyYpos = current_cfg->servoCoord[PWON_POS].yPos;

	if (httpd_start(&stream_httpd, &config) == ESP_OK)
	{
		httpd_register_uri_handler(stream_httpd, &stream_uri);
	}
}
