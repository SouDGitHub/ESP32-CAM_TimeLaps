# ESP32-Timelaps
ESP32-Cam TimeLaps project with servos control, settings storage, and many functions. Adjust your camera settings, prepare your trajectory, set your timelaps preference and start it. Exemple of trajectory on youtube:

[![Alt text](https://img.youtube.com/vi/_iji2RYj_Jw/0.jpg)](https://www.youtube.com/watch?v=_iji2RYj_Jw)

I used the code from bitluni to learn the web interface and prepare the base for future projects: [bitluni/ESP32CamTimeLapse](https://github.com/bitluni/ESP32CamTimeLapse).
Web interface joystick code from [bobboteck/JoyStick](https://github.com/bobboteck/JoyStick).
Servos kit can be found on [aliexpress](https://fr.aliexpress.com/item/2048998846.html?spm=a2g0o.productlist.0.0.2f0645b2A2zxhA&algo_pvid=f6b010f3-41ee-44b2-82ba-8f12ce782c5f&algo_expid=f6b010f3-41ee-44b2-82ba-8f12ce782c5f-47&btsid=0b0a3f8115886184756704716e37b9&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_) for cheap!



## Features
* Fully controlled through the web interface
* All settings in EEPROM
* 3 sets of settings available to store/load on demand
* AutoStart
* WiFi Station & AP WiFi 
* WiFi can be disable while running
* Memory information and management
* SDCard can be initialize while running
* LED Flash fully configurable (dimmable)
* Two axis servos arm can be controlled with the joystick from the web interface
* Configurable trajectory to track object during time laps (preview available embedded)

## Configuration
* All parameters controllable from web interface
* Select/comment camera board in `src\camera.cpp` according to your board
* According to your camera behavior, set the camera frequency in camera.h "config.xclk_freq_hz" to get good images. Mine works at 5MHz.
* Serial pins are used for Serial or Servos. Set your configuration in "debug.h"

## Create video from img using W10 Photo
* In Windows Explorer, select all files you want to include in the video
* Right click on selection -> Property -> Details
* Select a random date on the "Origin -> Date taken" property, then apply
* Use native W10 Photo to create video

## Create video from img using fffmpeg
[JackGruber/ESP32-Timelaps-Webcam](https://github.com/JackGruber/ESP32-Timelaps-Webcam)

## To Do
* Add embedded picture rotation
* Add camera frequency in settings
* Add deep sleep function during time laps
* Add settings to maintain command or to power off servo when not moving
* Add servo speed control loop somewhere (use of ESP fade timer hardware function can be great)

## Limitation
* Fast movements can reset the ESP32-CAM : power supply glitches
* Servos might generate steps because of servos specifications. PWM SW resolution is much better than my servos resolution.
* Trajectory path follow the function Y=AX2+BX+C. X is servos X (ROTATION) Y is servo Y (UP/DOWN). Set your Start/Mid/End points accordingly: Mid point should be SOMEWHERE IN BETWEEN the others.

## Images
<img src="img/ESP32-CAM_SERVOS.jpg">
<img src="img/webmenu.jpg">

## Links
* [Original GitHub repro ESP32CamTimeLapse](https://github.com/bitluni/ESP32CamTimeLapse)
* [Project page from the ESP32CamTimeLapse](https://bitluni.net/esp32camtimelapse)
* [FFmpeg download](https://www.ffmpeg.org/download.html)
* [An other cool TimeLaps project from JackGruber that could better fit your needs](https://github.com/JackGruber/ESP32-Timelaps-Webcam)
