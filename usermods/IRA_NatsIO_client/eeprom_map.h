// TODO should be replaced by config 
#include <EEPROM.h>

/// ALL EEPROM ADDRESS LOCATIONS ///
#define EEPROM_MAJ_VERSION  1   // List the major eeprom layout version
#define EEPROM_MINOR_VERION 2   // List the minor eeprom layout version
#define NATS_MODE           3   // the remote set operation mode (over NATS)
#define PIXEL_LENGTH        4   // the number of pixels in the pixeltape

#define FX_SELECT           5
#define FX_SPEED            6
#define FX_XFADE            7
#define FX_FGND_R           8
#define FX_FGND_G           9
#define FX_FGND_B           10
#define FX_BGND_R           11
#define FX_BGND_G           12
#define FX_BGND_B           13

#define HW_BOARD_VERSION    14      // 00 is original, 01 is first production batch
#define HW_BOARD_SERIAL_NR  15      // The board sequence nr

#define DEV_NAME_LENGTH     16
#define DEV_NAME            17      // 32 bytes long max

/// EEPROM SIZE
#define EEPROM_SIZE         64    // the number of bytes we want to read/store, rounded up

uint8_t eeprom_maj;
uint8_t eeprom_min;

/// ALL 'LIVE' Veriables that work with EEPROM non-volatile storage
uint8_t nats_mode;             // this variable keeps the operation mode set from NATS
bool    nats_mode_changed;     // indicate to the main loop that the mode changed
uint8_t pixel_length;

uint8_t fx_select;
uint8_t fx_speed;
uint8_t fx_xfade;
uint8_t fx_fgnd_r;
uint8_t fx_fgnd_g;
uint8_t fx_fgnd_b;
uint8_t fx_bgnd_r;
uint8_t fx_bgnd_g;
uint8_t fx_bgnd_b;

uint8_t board_version;
uint8_t board_serial_nr;

uint8_t dev_name_length;

void eeprom_restate ()
{
    eeprom_maj = EEPROM.read(EEPROM_MAJ_VERSION);       // 1
    eeprom_min = EEPROM.read(EEPROM_MINOR_VERION);      // 2

    nats_mode   = EEPROM.read(NATS_MODE);               // 3
// handled by wled    
    pixel_length = EEPROM.read(PIXEL_LENGTH);           // 4
// handled by wled 
    fx_select   = EEPROM.read(FX_SELECT);               // 5
    fx_speed    = EEPROM.read(FX_SPEED);
    fx_xfade    = EEPROM.read(FX_XFADE);
    fx_fgnd_r   = EEPROM.read(FX_FGND_R);
    fx_fgnd_g   = EEPROM.read(FX_FGND_G);
    fx_fgnd_b   = EEPROM.read(FX_FGND_B);
    fx_bgnd_r   = EEPROM.read(FX_BGND_R);
    fx_bgnd_g   = EEPROM.read(FX_BGND_G);
    fx_bgnd_b   = EEPROM.read(FX_BGND_B);               // 13

    board_version = EEPROM.read(HW_BOARD_VERSION);      // 14
    board_serial_nr = EEPROM.read(HW_BOARD_SERIAL_NR);  // 15

    dev_name_length = EEPROM.read(DEV_NAME_LENGTH);
}

void eeprom_variables_print() 
{
    Serial.print("[VAR] eeprom_maj: "); Serial.println(eeprom_maj);
    Serial.print("[VAR] eeprom_min: "); Serial.println(eeprom_min);

    Serial.print("[VAR] nats_mode: "); Serial.println(nats_mode);
    Serial.print("[VAR] pixel_length: "); Serial.println(pixel_length);

    Serial.print("[VAR] fx_select: "); Serial.println(fx_select);
    Serial.print("[VAR] fx_speed: "); Serial.println(fx_speed);
    Serial.print("[VAR] fx_xfade: "); Serial.println(fx_xfade);
    Serial.print("[VAR] fx_fgnd_r: "); Serial.println(fx_fgnd_r);
    Serial.print("[VAR] fx_fgnd_g: "); Serial.println(fx_fgnd_g);
    Serial.print("[VAR] fx_fgnd_b: "); Serial.println(fx_fgnd_b);
    Serial.print("[VAR] fx_bgnd_r: "); Serial.println(fx_bgnd_r);
    Serial.print("[VAR] fx_bgnd_g: "); Serial.println(fx_bgnd_g);
    Serial.print("[VAR] fx_bgnd_rb: "); Serial.println(fx_bgnd_b);

    Serial.print("[VAR] board_version: "); Serial.println(board_version);
    Serial.print("[VAR] board_serial_nr: "); Serial.println(board_serial_nr);
    Serial.print("[VAR] dev_name_length: "); Serial.println(dev_name_length);
}