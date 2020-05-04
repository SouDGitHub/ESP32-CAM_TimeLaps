#include "arduino.h"

#define EEPROM_SIZE 512
#define NB_CAMERA_STORE 4   //First location reserved for defaults settings
#define NB_COORD_STORE 4
#define PWON_POS 0
#define START_POS 1
#define MID_POS 2
#define END_POS 3




struct cfg_camera_struct{
    uint8_t framesize;//0 - 10
    uint8_t quality;//0 - 63
    int8_t brightness;//-2 - 2
    int8_t contrast;//-2 - 2
    int8_t saturation;//-2 - 2
    int8_t sharpness;//-2 - 2
    uint8_t denoise;
    uint8_t special_effect;//0 - 6
    uint8_t wb_mode;//0 - 4
    uint8_t awb;
    uint8_t awb_gain;
    uint8_t aec;
    uint8_t aec2;
    int8_t ae_level;//-2 - 2
    uint16_t aec_value;//0 - 1200
    uint8_t agc;
    uint8_t agc_gain;//0 - 30
    uint8_t gainceiling;//0 - 6
    uint8_t bpc;
    uint8_t wpc;
    uint8_t raw_gma;
    uint8_t lenc;
    uint8_t hmirror;
    uint8_t vflip;
    uint8_t dcw;
    uint8_t colorbar;
    int xclk_freq_hz;
    uint8_t flash;
    uint8_t flashThresh;
} ;

struct coordinate{
    int xPos = 0;
    int yPos = 0;
};

struct cfg_struct{
    uint8_t RestoreDefaultFalg = 0;
    int eepromUsed = 0;
    String AP_WiFi_ssid = "";
    String AP_WiFi_pw = "";
    String WiFi_ssid = "";
    String WiFi_pw = "";
    boolean StartAPOnly = 0;
    boolean AutoStartTimeLapsStartUp = 0;
    uint8_t AutoStartCFG = 0;
    uint16_t TimeLapsDelta = 0;
    uint8_t forceFlash = 0;
    uint8_t forceFlashThresh = 0;
    uint8_t flash = 0;
    uint8_t flashThresh = 0;
    cfg_camera_struct camera[NB_CAMERA_STORE];
    coordinate servoCoord[NB_COORD_STORE];
    boolean servoXInv = 0;
    boolean servoYInv = 0;
    boolean useTrajectory = 0;
    int nbPicTrajectory = 0;
};

extern struct cfg_struct* get_current_cfg();
extern boolean initCfgFromEEPROM();
extern void storeCfgToEEPROM();
extern void storeDefaultCfgToEEPROM();
extern void storeCAMX(uint8_t n);
extern void loadCAMX(uint8_t n);