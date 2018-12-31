
#include <stdio.h>
#include <Arduino.h>

// LiquidCrystal library
#include <LiquidCrystal.h>

// Timed function callback library
#include <uTimerLib.h>

// Include the EEPROM library
#include <EEPROM.h>

#include "app.h"


// Externals

// Locals
static volatile bool    lcdRealTimeUpdateFlag = false;          // Update pacing flags & tickers
static volatile bool    servoRealTimeUpdateFlag = false;
static volatile uint8_t lcd_ticks = REALTIME_UPDATE_TICKS_LCD;

static char           textBuffer[LCD_ROWS ][LCD_COLS+10];        // holding buffer for sprintf()
static const char     fixText[][LCD_COLS+10] = 
    {
    "Fan freq=%4d Hz ",   // 16 chars
    "Fn=%3d%% Sv=%4du"
    };

// local functions
static void   realTimeUpdateTicker(void);

////////////////////////////////////////////////////////////////////////////////////


/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	realTimeUpdateTicker() 
 *          Controls update pacing flags for 
 *          LCD and SERVO
 *******************************************************
 */
static void realTimeUpdateTicker(void)
  {
  servoRealTimeUpdateFlag = true;
  if (lcd_ticks-- == 0)
    {
    lcd_ticks = REALTIME_UPDATE_TICKS_LCD;
    lcdRealTimeUpdateFlag = true;
    }
  }


/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	setup() 
 *          Standard Arduino sketch initialization function
 *******************************************************
 */
void setup()
  {
  // Setup the EEPROM data if it exists
  menusysSetup();
  // Initialize signal processing
  initSignalProcessing();
  // callback_function for updates tick rate
  TimerLib.setInterval_us(realTimeUpdateTicker, REALTIME_UPDATE_RATE);
  }



/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	loop()
 *          Standard Arduino loop function
 *******************************************************
 */
void loop()
  {
  static waveFormData_t wfd;
  uint16_t wasUpdated;
  static int sPos = 0;

  wasUpdated = getWaveformData(f2sSetup.fullDutyAtLevel, &wfd);  // Get current duty cycle (percent) of fan PWM signal

  // Calculate servo position after waveform update
  // LCD update attempt is run when no servo update is made
  if (wasUpdated && servoRealTimeUpdateFlag)
    {
    sPos = servoPosSet(wfd.pwm_duty);        // New servo position
    servoRealTimeUpdateFlag = false;  // arm pace flag for next update
    }
  else if (!menusysRunLoop() && lcdRealTimeUpdateFlag && wasUpdated)
    {
    /*
    * run this code if menu system is idle and the update condition is met (120 mS interval)   */
    sprintf(textBuffer[LCD_FIRST_LINE], fixText[0], wfd.pwm_freqency);   // format...
    sprintf(textBuffer[LCD_SECOND_LINE], fixText[1], wfd.pwm_duty, sPos);
    printonLCDatPos(textBuffer[LCD_FIRST_LINE], LCD_FIRST_LINE, 0);
    printonLCDatPos(textBuffer[LCD_SECOND_LINE], LCD_SECOND_LINE, 0);
    lcdRealTimeUpdateFlag = false;
    }
  }

