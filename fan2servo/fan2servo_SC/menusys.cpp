/**
*   Menyu system -
*
*
*
*/

#include <Arduino.h>

// LiquidCrystal library
#include <LiquidCrystal.h>

// Library for using single pin analog button for 5 LCD buttons
// include the AnalogMultiButton library
#include <AnalogMultiButton.h>

// Include the EEPROM library
#include <EEPROM.h>

// Menu system specifics
#include "menusys.h"

// local app.h header
#include "app.h"

// Button configuration - 5 on a analog ladder
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5
#define btnTotal  btnNONE

static const int buttonLimitValues[] = {0, 100, 250, 410, 640};

static uint32_t   freq = 0;       // Holds measured frequency

// make an AnalogMultiButton object, pass in the pin, total and values array
static AnalogMultiButton btns(A0, btnTotal, buttonLimitValues);

// select the pins used on the LCD panel
static LiquidCrystal lcd(LCD_REG_SELECT, LCD_ENABLE, LCDDATA_0, LCDDATA_1, LCDDATA_2, LCDDATA_3);

// local functions
static int        getKey(void);                   // LCD module buttons
static uint32_t   eeprom_crc(byte *, uint16_t); // CRC calculation for EEPROM

// CRC polynomial table 
static const uint32_t crc_table[16] =
  {
  0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
  0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };
static uint32_t   crcCopy;

// Load this content into menu data structure when EEPROM empty or invalid
static const fanToServoSetup_t defaultSetup = 
  {
  0x55,                       // header
  false,                      // Full duty defult low
  5,                          // Ignore the first n PWM transitions
  0,                          // Normal
  0,                          // Linear move
  { 0, 20, 40, 60, 80, 100 }, // Curve points
  250,                        // Servo trim - low end
  -300,                       // Servo trim - high end
  100,                        // Rate - full range default
  1500,                       // Default midpoint for servo (1500 uS)
  0,                          // CRC 32
  };

// Global menu system data
fanToServoSetup_t     f2sSetup;   // saved application setup data
MD_Menu::value_t      vBuf;       // interface buffer for values

// Menu Headers --------
const PROGMEM MD_Menu::mnuHeader_t mnuHdr[] =
  {
  // id |  label |              idItmStart| idItmEnd |idItmCurr
    { 10,  "F2S main menu",     10,         12,       0 },
    { 11,  "PWM inp. menu",     20,         21,       0 },
    { 12,  "Servo menu",        30,         40,       0 },
  };

// Menu Items ----------
const PROGMEM MD_Menu::mnuItem_t mnuItm[] =
  {
  // Starting (Root) menu
  // id |label           action |           actionId

    { 10, "PWM Input",       MD_Menu::MNU_MENU, 11 },
    { 11, "Damper servo",    MD_Menu::MNU_MENU, 12 },

  // PWM input submenu
    { 20, "PWM 100% lvl",   MD_Menu::MNU_INPUT, 10 },
    { 21, "Ignore first",   MD_Menu::MNU_INPUT, 11 },

  // Servo props setup submenu
    { 30, "Servo move",     MD_Menu::MNU_INPUT, 30 },
    { 31, "Move type",      MD_Menu::MNU_INPUT, 31 },
      { 32, "Point 1/4",      MD_Menu::MNU_INPUT, 32 },   // Valid menu set if move type is Curve
      { 33, "Point 2/4",      MD_Menu::MNU_INPUT, 33 },
      { 34, "Point 3/4",      MD_Menu::MNU_INPUT, 34 },
      { 35, "Point 4/4",      MD_Menu::MNU_INPUT, 35 },
    { 37, "End trim LO",    MD_Menu::MNU_INPUT, 37 },
    { 38, "End trim HI",    MD_Menu::MNU_INPUT, 38 },
    { 39, "Move rate",      MD_Menu::MNU_INPUT, 39 },
    { 40, "Mid position",   MD_Menu::MNU_INPUT, 40 },
  };

// Input list Items ---------
const PROGMEM char    duty100[]       = "Low|High";
const PROGMEM char    servoDirList[]  = "Norm|Reverse";
const PROGMEM char    movementType[]  = "Linear|Curve";

const PROGMEM MD_Menu::mnuInput_t mnuInp[] =
  {
  // Input PWM
  //  id |label |         action |          cbValueRequest cbVR| fieldWid|  rMin  |     |rMax   |    base| *pList
    { 10, "Input at" ,    MD_Menu::INP_LIST,  mnuDutyRqst,        4,        0,    0,      0,    0,    0,    duty100 },
    { 11, "Skip",         MD_Menu::INP_INT,   mnuSkipRqst,        3,        0,    0,      20,   0,    10,   nullptr },

  // Servo
    { 30, "Dir",          MD_Menu::INP_LIST,  mnuSDirRqst,          7,        0,    0,      0,    0,    0,    servoDirList },
    { 31, "As",           MD_Menu::INP_LIST,  mnuSMovTypeRqst,      6,        0,    0,      0,    0,    10,   movementType },
      { 32, "C.Pt1",        MD_Menu::INP_INT,   mnuCurvePtRqst,     3,        0,    0,    100,    0,    10,   nullptr },
      { 33, "C.Pt2",        MD_Menu::INP_INT,   mnuCurvePtRqst,     3,        0,    0,    100,    0,    10,   nullptr },
      { 34, "C.Pt3",        MD_Menu::INP_INT,   mnuCurvePtRqst,     3,        0,    0,    100,    0,    10,   nullptr },
      { 35, "C.Pt4",        MD_Menu::INP_INT,   mnuCurvePtRqst,     3,        0,    0,    100,    0,    10,   nullptr },
    { 37, "uS+",          MD_Menu::INP_INT,   mnuTrimRqst,          4,        0,    0,    500,    0,   10,    nullptr },
    { 38, "uS-",          MD_Menu::INP_INT,   mnuTrimRqst,          4,     -500,    0,      0,    0,   10,    nullptr },
    { 39, "% Full",       MD_Menu::INP_INT,   mnuRateRqst,          4,       50,    0,    100,    0,   10,    nullptr },
    { 40, "Mid uS",       MD_Menu::INP_INT,   mnuMidPointRqst,      4,     1000,    0,   2000,    0,   10,    nullptr }
  };

// bring it all together in the global menu object
MD_Menu Mnu(menuNavigation, LCDdisplay,           // user navigation and display
          mnuHdr, ARRAY_SIZE(mnuHdr),       // menu header data
          mnuItm, ARRAY_SIZE(mnuItm),       // menu item data
          mnuInp, ARRAY_SIZE(mnuInp));      // menu input data



/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	Callback code for menu set/get input values
 *
 *	param:
 *	returns:	Value request callback duty level
 ***/
MD_Menu::value_t *mnuDutyRqst(MD_Menu::mnuId_t id, bool bGet)
  {
  MD_Menu::value_t *r = &vBuf;

  if (id == 10)
    {
    if (bGet)
      vBuf.value = f2sSetup.fullDutyAtLevel;
    else
      f2sSetup.fullDutyAtLevel = vBuf.value;
    }
  else
    r = nullptr;
  return (r);
  }

/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	Callback code for menu set/get input values
 *
 *	param:
 *	returns:	Value request callback for transitions to skip at start
 ***/
MD_Menu::value_t *mnuSkipRqst(MD_Menu::mnuId_t id, bool bGet)
  {
  MD_Menu::value_t *r = &vBuf;

  if (id == 11)
    {
    if (bGet)
      vBuf.value = f2sSetup.ignorefirst;
    else
      f2sSetup.ignorefirst = vBuf.value;
    }
  else
    r = nullptr;
  return(r);
  }

 /*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	Callback code for menu set/get input values
 *
 *	param:
 *	returns:	Value request callback for Servo direction
 ***/
MD_Menu::value_t *mnuSDirRqst(MD_Menu::mnuId_t id, bool bGet)
  {
  MD_Menu::value_t *r = &vBuf;

  if (id == 30)
    {
    if (bGet)
      vBuf.value = f2sSetup.servoDir;
    else
      f2sSetup.servoDir = vBuf.value;
    }
  else
    r = nullptr;
  return(r);
  }

 /*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	Callback code for menu set/get input values
 *
 *	param:
 *	returns:	Value request callback for Servo movement type
 *            Linear or curve points
 ***/
MD_Menu::value_t *mnuSMovTypeRqst(MD_Menu::mnuId_t id, bool bGet)
  {
  MD_Menu::value_t *r = &vBuf;

  if (id == 31)
    {
    if (bGet)
      vBuf.value = f2sSetup.linOrCurve;
    else
      f2sSetup.linOrCurve = vBuf.value;
    }
  else
    r = nullptr;
  return(r);
  }

/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	Callback code for menu set/get input values
 *
 *	param:
 *	returns:	Value request callback for Curve points
 ***/
MD_Menu::value_t *mnuCurvePtRqst(MD_Menu::mnuId_t id, bool bGet)
  {
  MD_Menu::value_t *r = &vBuf;

  // The endpoint should always be 0 and 100
  // Make sure they are
  f2sSetup.cp[0] = 0;
  f2sSetup.cp[SERVO_CURVE_PTS-1] = 100;

  if (f2sSetup.linOrCurve == 0)
    return (nullptr);   // Not in curve mode - skip this menu

  switch (id)
    {
    case 32:
      if (bGet)
        vBuf.value = (int32_t)f2sSetup.cp[1];
      else
        f2sSetup.cp[1] = (int16_t)vBuf.value;
      break;

    case 33:
      if (bGet)
        vBuf.value = (int32_t)f2sSetup.cp[2];
      else
        f2sSetup.cp[2] = (int16_t)vBuf.value;
      break;

    case 34:
      if (bGet)
        vBuf.value = (int32_t)f2sSetup.cp[3];
      else
        f2sSetup.cp[3] = (int16_t)vBuf.value;
      break;

    case 35:
      if (bGet)
        vBuf.value = (int32_t)f2sSetup.cp[4];
      else
        f2sSetup.cp[4] = (int16_t)vBuf.value;
      break;

    default:
      r = nullptr;
      break;
    }
  return (r);
  }

/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	Callback code for menu set/get input values
 *
 *	param:
 *	returns:	Value request callback for LO/HI Servo endpoint
              trim data
 ***/
MD_Menu::value_t *mnuTrimRqst(MD_Menu::mnuId_t id, bool bGet)
  {
  MD_Menu::value_t *r = &vBuf;

  switch (id)
    {
    case 37:    // Trim lo
      if (bGet)
        vBuf.value = f2sSetup.servoTrimLo;
      else
        f2sSetup.servoTrimLo = vBuf.value;
      break;

    case 38:    // Trim hi
      if (bGet)
        vBuf.value = f2sSetup.servoTrimHi;
      else
        f2sSetup.servoTrimHi = vBuf.value;
      break;

    default:
      r = nullptr;
      break;
    }
  return (r);
  }

/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	Callback code for menu set/get input values
 *
 *	param:
 *	returns:	Value request callback for servo reduced rate value
 ***/
MD_Menu::value_t *mnuRateRqst(MD_Menu::mnuId_t id, bool bGet)
  {
  MD_Menu::value_t *r = &vBuf;

  if (id == 39)
    {
    if (bGet)
      vBuf.value = f2sSetup.dualRateFactor;
    else
      f2sSetup.dualRateFactor = vBuf.value;
    }
  else
    r = nullptr;
  return(r);
  }

/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	Callback code for menu set/get input values
 *
 *	param:
 *	returns:	Value request callback for servo midpoint
 *            pulse width value
 ***/
MD_Menu::value_t *mnuMidPointRqst(MD_Menu::mnuId_t id, bool bGet)
  {
  MD_Menu::value_t *r = &vBuf;

  if (id == 40)
    {
    if (bGet)
      vBuf.value = f2sSetup.midPoint;
    else
      {
      if (vBuf.value < (getPhysicalServoMax() - 10) && vBuf.value > (getPhysicalServoMin() + 10))
        f2sSetup.midPoint = vBuf.value;
      }
    }
  else
    r = nullptr;
  return(r);
  }



//////////////////////////////////////////////////////////////////////////
// Generic code for LCD and navighation
//////////////////////////////////////////////////////////////////////////

/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	Display actions for the menu system
 *
 *	param:	action - What to do
 *  param:  message - to display
 *
 *	returns:	boolean
 *******************************************************
 */
bool LCDdisplay(MD_Menu::userDisplayAction_t action, char *msg)
  {
  static char szLine[LCD_COLS + 1] = { '\0' };

  switch (action)
    {
    case MD_Menu::DISP_INIT:
      lcd.begin(LCD_COLS, LCD_ROWS);
      lcd.clear();
      lcd.noCursor();
      memset(szLine, ' ', LCD_COLS);
      break;

    case MD_Menu::DISP_CLEAR:
      lcd.clear();
      break;

    case MD_Menu::DISP_L0:
      lcd.setCursor(0, 0);
      lcd.print(szLine);
      lcd.setCursor(0, 0);
      if (strlen(msg) >= LCD_COLS)
        *(msg + LCD_COLS) = 0;  // force null termination - string too long
      lcd.print(msg);
      break;

    case MD_Menu::DISP_L1:
      lcd.setCursor(0, 1);
      lcd.print(szLine);
      lcd.setCursor(0, 1);
      if (strlen(msg) >= LCD_COLS)
        *(msg + LCD_COLS) = 0;  // force null termination - string too long
      lcd.print(msg);
      break;
    }
  return(true);
  }


/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	Navigation callback for the menu system
 *          Get keystrokes from the LCD shield
 *
 *	param:	incDelta - how many to increment at once
 *
 *	returns:	nav action
 *******************************************************
 */
MD_Menu::userNavAction_t menuNavigation(uint16_t &incDelta)
  {
  incDelta = 1;

  switch (getKey())
    {
    case btnDOWN:
      return(MD_Menu::NAV_DEC);

    case btnUP:
      return(MD_Menu::NAV_INC);

    case btnSELECT:
      return(MD_Menu::NAV_SEL);

    case btnLEFT:
    case btnRIGHT:
      return(MD_Menu::NAV_ESC);
    }
  return(MD_Menu::NAV_NULL);
  }


/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	Navigation callback for the menu system
 *          Get keystrokes from the LCD shield
 *
 *	returns:	int
 *******************************************************
 */
static int getKey(void)
  {
  int key_hit = btnNONE;
  //static int key_mem = btnNONE;

  // Read the keys
  if (btns.onPressAndAfter(btnSELECT, KEY_REPEAT_WAITTIME, KEY_REPEAT_RATE))
    key_hit = btnSELECT;
  else if (btns.onPressAndAfter(btnLEFT, KEY_REPEAT_WAITTIME, KEY_REPEAT_RATE))
    key_hit = btnLEFT;
  else if (btns.onPressAndAfter(btnRIGHT, KEY_REPEAT_WAITTIME, KEY_REPEAT_RATE))
    key_hit = btnRIGHT;
  else if (btns.onPressAndAfter(btnUP, KEY_REPEAT_WAITTIME, KEY_REPEAT_RATE))
    key_hit = btnUP;
  else if (btns.onPressAndAfter(btnDOWN, KEY_REPEAT_WAITTIME, KEY_REPEAT_RATE))
    key_hit = btnDOWN;
  return (key_hit);
  }

/*
*	author: Thom
*	date:	15-12-2018
*
*	brief:	Writes <txt> to LCD @ <row>, <col>
*
*******************************************************
*/
void printonLCDatPos(char *txt, int8_t row, int8_t col)
  {
  lcd.setCursor(col, row);
  lcd.print(txt);
  }

/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	CRC-32 Calculate 
 *
 *	param:
 *	returns:	CRC32 for array[length]
 *
 ***/
static uint32_t eeprom_crc(byte *elemPtr, uint16_t elemLen) 
  {
  uint16_t idx;
  uint16_t EEpromLen = EEPROM.length();
  uint32_t crc = ~0L;

  for (idx = 0; idx < min(elemLen, EEpromLen); idx++) 
    {
    crc = crc_table[(crc ^ elemPtr[idx]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (elemPtr[idx] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
    }
  return crc;
  }

/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	Setup the menu system
 *
 *	param:
 *	returns:
 *******************************************************
 */
void menusysSetup(void)
  {
  uint16_t recordLength =  sizeof(f2sSetup) - sizeof(f2sSetup._crc);

  // Setup menu data structure with saved EEPROM values
  EEPROM.get(0, f2sSetup);
  if (eeprom_crc((byte *)&f2sSetup, recordLength) != f2sSetup._crc)
    {
    /*
    * EEPROM empty or corrupt   */
    f2sSetup = defaultSetup;    // Copy default settings to config
    f2sSetup._crc = eeprom_crc((byte *)&f2sSetup, recordLength);
    EEPROM.put(0, f2sSetup);    // Write default to EEPROM /w CRC
    }
  crcCopy = f2sSetup._crc;    // Copy the crc for fast check of changed data

  // setup LED
  pinMode(LED_PIN, OUTPUT);
  btns.update();

  // Initialize the LCD
  LCDdisplay(MD_Menu::DISP_INIT, nullptr);

  Mnu.begin();
  Mnu.setMenuWrap(true);
  Mnu.setAutoStart(AUTO_START);
  Mnu.setTimeout(MENU_TIMEOUT);
  }


/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	Runs the menu system - should be called continuously
 *
 *	param:
 *
 *	returns:  true if in a menu 
 *
 *******************************************************
 */
bool menusysRunLoop(void)
  {
  static bool crc_checked = false;
  uint16_t recordLength =  sizeof(f2sSetup) - sizeof(f2sSetup._crc);
  uint16_t dummy;
  uint32_t current_crc;

  // start with updating all the LCD shield buttons
  btns.update();

  /*
  * If we are not running and not autostart
  * check if there is a reason to start the menu    */
  if (!Mnu.isInMenu() && !AUTO_START)
    {
    if (menuNavigation(dummy) == MD_Menu::NAV_SEL)
      Mnu.runMenu(true);
    }
  Mnu.runMenu();   // just run the menu code

  // Check for menu data changes
  if (!Mnu.isInMenu())
    {
    if (!crc_checked)
      {
      current_crc = eeprom_crc((byte *)&f2sSetup, recordLength);
      if (current_crc != crcCopy)
        {
        // Change - Update EEPROM content
        f2sSetup._crc = current_crc;
        crcCopy = current_crc;
        EEPROM.put(0, f2sSetup);
        }
      crc_checked = true;
      }
    }
  else
    crc_checked = false;
  return (Mnu.isInMenu());
  }


///////////////////////////////////////////////////////////////
///  Utility functions
///////////////////////////////////////////////////////////////

/*
 *	author: Thom
 *	date:	29-12-2018
 *
 *	brief:	Fetches and returns the current physical span
 *          for fanServo (including endpoint trims).
 *
 *	returns:	 Physical span
 *
 *******************************************************
 */
int getPhysicalServoSpan(void)
  {
  return (getPhysicalServoMax() - getPhysicalServoMin());
  }

/*
 *	author: Thom
 *	date:	29-12-2018
 *
 *	brief:	Fetches and returns the current physical
 *          endpoint minimum pulse length for fanServo, 
 *          including the endpoint trim.
 *
 *	returns:	 Physical minimum endpoint
 *
 *******************************************************
 */
int getPhysicalServoMin(void)
  {
  return (SERVO_PULSE_MIN + f2sSetup.servoTrimLo);
  }

/*
 *	author: Thom
 *	date:	29-12-2018
 *
 *	brief:	Fetches and returns the current physical
 *          endpoint maximum pulse length for fanServo, 
 *          including the endpoint trim.
 *
 *	returns:	 Physical maximum endpoint
 *
 *******************************************************
 */
int getPhysicalServoMax(void)
  {
  return (SERVO_PULSE_MAX + f2sSetup.servoTrimHi);
  }
