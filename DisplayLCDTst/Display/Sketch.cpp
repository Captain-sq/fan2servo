
#include <Arduino.h>

//Sample using LiquidCrystal library
#include <LiquidCrystal.h>

// Library for using single pin analog button for 5 LCD buttons
// include the AnalogMultiButton library
#include <AnalogMultiButton.h>

// Button configuration - 5 on a analog ladder
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5
#define btnTotal  btnNONE

const int buttonLimitValues[] = {0, 100, 250, 410, 640};

// make an AnalogMultiButton object, pass in the pin, total and values array
AnalogMultiButton btns(A0, btnTotal, buttonLimitValues);

// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

static int    getKey(void);     // LCD module buttons


/**
 * Setup()
 *  All initializations
 */
void setup()
	{
	lcd.begin(16, 2);              // start the library
	lcd.setCursor(0,0);
	lcd.print("Push the buttons"); // print a simple message
  }
    
    
/**
  * Loop()
  *  Main execution loop
  */  
void loop()
	{
  btns.update();                // LCD buttons update method  
	lcd.setCursor(9,1);           // move cursor to second line "1" and 9 spaces over
	lcd.print(millis()/1000);     // display seconds elapsed since power-up


	lcd.setCursor(0,1);            // move to the begining of the second line

	switch (getKey())               // depending on which button was pushed, we perform an action
		{
		case btnRIGHT:
			{
			lcd.print("RIGHT ");
			break;
			}
		case btnLEFT:
			{
			lcd.print("LEFT   ");
			break;
			}
		case btnUP:
			{
			lcd.print("UP    ");
      break;
      }
		case btnDOWN:
			{
			lcd.print("DOWN  ");
			break;
			}
		case btnSELECT:
			{
			lcd.print("SELECT");
			break;
			}
		case btnNONE:
			{
			lcd.print("NONE  ");
			break;
			}
		}
	}


/**
 *  @author Thom
 *  @brief  get keystrokes from the LCD module
 *
 *  @return:  See btnXXXXXX above
 */
static int getKey(void)
  {
  int key_hit = btnNONE;
  //static int key_mem = btnNONE;

  // Read the keys
  if (btns.isPressed(btnSELECT))
    key_hit = btnSELECT;
  else if (btns.isPressed(btnLEFT))
    key_hit = btnLEFT;
  else if (btns.isPressed(btnRIGHT))
    key_hit = btnRIGHT;
  else if (btns.isPressed(btnUP))
    key_hit = btnUP;
  else if (btns.isPressed(btnDOWN))
    key_hit = btnDOWN;

  /*
  // check for make/break
  if ((key_mem == btnNONE && key_hit != btnNONE) ||
      (key_hit != key_mem && key_hit != btnNONE) )
    {
    // make
    key_mem = key_hit;
    }
  else
    key_hit = btnNONE;  // held or break
  */
  return (key_hit);
  }
  