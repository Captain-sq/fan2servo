/*
 * signals.c
 *
 * Created: 2018-12-10 22:04:12
 *  Author: Thom
 */ 


#include <Arduino.h>

// include the servo library
#include <Servo.h>

// Include the FreqPeriodCounter library
#include <FreqPeriodCounter.h>

// Application specific includes
#include "app.h"

// symbolics
#define DUTY_SETTLING_TIME    500     // milliseconds "slack" time for duty cycle 

// Constants
static const byte counter_pin = FAN_INPUT;
static const byte cnt_int = INT0;
static const byte servo_out = SERVO_OUT;

// Signal processing variables
Servo             fanServo;
FreqPeriodCounter inputPWMsig(counter_pin, micros);
static int        fanServoPos_us;         // Current servo position in uSecs

// Local Prototypes
static void     INT0_ISR(void);
static int16_t  servoVirtualToPhysicalPulse(int16_t);

/////////////////////////////////////////////////////////////////////////////////////////

/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	Setup inputs and outputs
 *
 *	param:
 *	returns:
 *******************************************************
 */
void initSignalProcessing(void)
  {
  pinMode(counter_pin, INPUT_PULLUP);                           // Fan PWM input @ ca 400 Hz
  fanServo.attach(servo_out, SERVO_PULSE_MIN, SERVO_PULSE_MAX); // Servo output PWM range limits
  attachInterrupt(cnt_int, INT0_ISR, CHANGE);                   // Attach interrupt 0 to INT0_ISR
  // Set servo to default start position (closed)
  fanServoPos_us = servoVirtualToPhysicalPulse(0);        // Close damper initially
  fanServo.writeMicroseconds(fanServoPos_us);
  }
  
/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	ISR - Frequency period counter interrupt
 *
 *	param:
 *	returns:
 *******************************************************
 */
static void INT0_ISR(void)
  {
  inputPWMsig.poll();
  }
  
/*
 *	author: Thom
 *	date:	15-12-2018
 *
 *	brief:	Get Duty cycle and frequency values
 *
 *	param:
 *	returns:
 *******************************************************
 */
extern int16_t getWaveformData(bool dutyFullLevel, waveFormData_t *wfData)
  {
  uint16_t retval = 0;
  uint32_t msTmp;
  static uint32_t timeOut = millis();  // "slack" time

  retval = inputPWMsig.ready();
  if (retval)
    { 
    /*
    * Pulses is arriving    */
    if (dutyFullLevel == DUTY_FULL_LEVEL_LOW)
      wfData->pwm_duty = (uint16_t)((inputPWMsig.pulseWidthLow * 100) / (inputPWMsig.pulseWidth + inputPWMsig.pulseWidthLow));
    else
      wfData->pwm_duty = (uint16_t)((inputPWMsig.pulseWidth * 100) / (inputPWMsig.pulseWidth + inputPWMsig.pulseWidthLow));
    wfData->pwm_freqency = inputPWMsig.hertz();
    timeOut = millis();   // set slack timer to "now"
    }
  else if (((msTmp = millis()) - timeOut) > DUTY_SETTLING_TIME)
    {
    if (dutyFullLevel == DUTY_FULL_LEVEL_LOW)
      wfData->pwm_duty = digitalRead(counter_pin) ? 0: 100;
    else
      wfData->pwm_duty = digitalRead(counter_pin) ? 100 :0;
    wfData->pwm_freqency = 0;
    retval = true;    // Pulses gone missing, set appropriate duty and freq to 0
    }
  return (retval);
  }

/*
 *	author: Thom
 *	date:	29-12-2018
 *
 *	brief:	Calculates servo position for current update by the 
 *          getWaveforData() when update is pending
 *
 *  param:  Duty cycle from 0...100% 
 *
 *	returns:  Servo pulse width in uS
 *
 *******************************************************
 */
int servoPosSet(int16_t dutyCyclePercent)
  {
  int8_t cnt, intervals[] = { p0, p1, p2, p3, p4, p5 };
  float intCoeff, yOut;

  if (dutyCyclePercent < 0)
    dutyCyclePercent = 0;
  if (dutyCyclePercent > 100)
    dutyCyclePercent = 100;
  // factorize to pro mille

  // Calculate position
  if (f2sSetup.linOrCurve == SERVO_MODE_CURVE)
    {
    // X/Y curve operation /w interpolation, if linear
    // mode is selected, this entire if ()-section is skipped
    // The transformation is done in the virtual domain
    for (cnt = 0; cnt < (SERVO_CURVE_PTS - 1); cnt++)
      {
      // go through the coordinates and interpolate between
      if (dutyCyclePercent == intervals[cnt])
        {
        // Spot on curve point, get point value
        dutyCyclePercent = f2sSetup.cp[cnt];
        break;
        }
      else if (dutyCyclePercent > intervals[cnt] && dutyCyclePercent < intervals[cnt + 1])
        {
        // found X position between two curve points, interpolate
        intCoeff = (float)(dutyCyclePercent % SERVO_CURVE_INTERVAL) / (float)SERVO_CURVE_INTERVAL;  // find the coefficient between points (>0 <1)
        yOut = (float)(f2sSetup.cp[cnt + 1] - f2sSetup.cp[cnt]) * intCoeff + (float)f2sSetup.cp[cnt];   // Output value from curve
        dutyCyclePercent = (int16_t)lrintf(yOut);     // Make integer again
        break;    // we are finished here
        }
      } // for....
    } // if (f2s....

  // New pulse width
  fanServoPos_us = servoVirtualToPhysicalPulse(dutyCyclePercent * 10);
  // update position now
  fanServo.writeMicroseconds(fanServoPos_us);
  //fanServo.writeMicroseconds(1400);
  return (fanServoPos_us);
  }



/*
 *	author: Thom
 *	date:	29-12-2018
 *
 *	brief:	Converts virtual position to physical position
 *          (With respect to Norm/rev direction and endpoints)
 *
 *  param:  virtual position in %% (pro mille)
 *
 *          * 500 is always equal to the configured midpoint 
 *            pulse width of the servo object.
 *          * 0 is the configured minimum point as set by
 *            servo object initialization ± low trim
 *          * 1000 is the configured maximum point as set
 *            by servo object initialization ± high trim
 *
 *	returns:	servo output value in uS
 *******************************************************
 */
static int16_t servoVirtualToPhysicalPulse(int16_t vPosition)
  {
  int16_t xDevFromMidPos;
  float convFactor, stretch, finalPulseWidth;

  // Sanity check
  if (vPosition > SERVO_VIRTUAL_SPAN)
    vPosition = SERVO_VIRTUAL_SPAN;

  // Invert vPosition in case or servo reverse setting
  if (f2sSetup.servoDir == SERVO_DIR_REV)
    vPosition = SERVO_VIRTUAL_SPAN - vPosition;

  // Adjust physical span to trim and convert virtual stretch to 
  // final physical pulse width adjusted with rate 
  if (vPosition < SERVO_VIRTUAL_MIDPOINT)
    {
    xDevFromMidPos = SERVO_VIRTUAL_MIDPOINT - vPosition;
    // Adjust for rate
    convFactor = (float)f2sSetup.dualRateFactor / (float)100;
    stretch = convFactor * (float)xDevFromMidPos;
    xDevFromMidPos = (int16_t)lrintf(stretch);
    // Convert from virtual to physical domain
    convFactor = (float)(f2sSetup.midPoint - getPhysicalServoMin()) / (float)SERVO_VIRTUAL_MIDPOINT;    // Transform X into Y domain using factor
    stretch = convFactor * (float)xDevFromMidPos;
    finalPulseWidth = (float)f2sSetup.midPoint - stretch;
    }
  else if (vPosition > SERVO_VIRTUAL_MIDPOINT)
    {
    // Adjust for rate
    convFactor = (float)f2sSetup.dualRateFactor / (float)100;
    stretch = convFactor * (float)xDevFromMidPos;
    xDevFromMidPos = (int16_t)lrintf(stretch);
    // Convert from virtual to physical domain
    xDevFromMidPos = vPosition - SERVO_VIRTUAL_MIDPOINT;
    convFactor = (float)(getPhysicalServoMax() - f2sSetup.midPoint) / (float)SERVO_VIRTUAL_MIDPOINT;
    stretch = convFactor * (float)xDevFromMidPos;
    finalPulseWidth = (float)f2sSetup.midPoint + stretch;
    }
  else
    finalPulseWidth = (float)f2sSetup.midPoint;

  // Now return the final pulse width
  return ((int16_t)lrintf(finalPulseWidth));
  }



