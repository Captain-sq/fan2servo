/*
 * app.h
 *
 * Created: 2018-12-10 22:08:13
 *  Author: Thom
 */ 


#ifndef APP_H_
#define APP_H_

#define ONE_SECOND      1000    // Frequency counting gate time

// allocated MCU pins
#define LCDDATA_0       4     // For LCD control
#define LCDDATA_1       5
#define LCDDATA_2       6
#define LCDDATA_3       7
#define LCD_REG_SELECT  8
#define LCD_ENABLE      9

// LCD display properties
#define LCD_ROWS          2
#define LCD_COLS          16
#define LCD_FIRST_LINE    0
#define LCD_SECOND_LINE   1

#define FAN_INPUT       2     // For fan PWM in
#define SERVO_OUT       3     // for servo PWM out

#define KEY_REPEAT_WAITTIME     1000      // mS to hold down key until repeat starts
#define KEY_REPEAT_RATE         80        // mS between key hits in key repeat mode

// Servo properties
#define SERVO_PULSE_MIN         800       // uS min
#define SERVO_PULSE_MAX         2200      // uS max
#define SERVO_PULSE_SPAN        (SERVO_PULSE_MAX - SERVO_PULSE_MIN)
#define SERVO_DIR_NORM          false
#define SERVO_DIR_REV           true
#define SERVO_VIRTUAL_SPAN      1000
#define SERVO_VIRTUAL_MIDPOINT  (SERVO_VIRTUAL_SPAN / 2)
#define SERVO_MODE_LINEAR       0
#define SERVO_MODE_CURVE        1
#define SERVO_CURVE_PTS         6       // 4 point and the (2) endpoints
#define SERVO_CURVE_INTERVAL    20      // 20 steps between each cpt

typedef enum
  {
  p0      = 0,
  p1      = p0 + SERVO_CURVE_INTERVAL,
  p2      = p1 + SERVO_CURVE_INTERVAL,
  p3      = p2 + SERVO_CURVE_INTERVAL,
  p4      = p3 + SERVO_CURVE_INTERVAL,
  p5      = p4 + SERVO_CURVE_INTERVAL       
  } srvPTinterval_t;

// Signal processing properties
#define DUTY_FULL_LEVEL_LOW     false     // logic 0 is full duty (100% ) duty cycle
#define DUTY_FULL_LEVEL_HIGH    true      // logic 1 is full duty (100% ) duty cycle

// timing and control
#define REALTIME_UPDATE_RATE        9000      // Tick rate for LCD and Servo updates in uS
#define REALTIME_UPDATE_TICKS_LCD   20        // update LCD each 20 ticks

// Types
typedef struct 
  {
  int16_t       pwm_duty;
  int16_t       pwm_freqency;
  } waveFormData_t;

typedef struct __fan2servoSetup
  {
  byte            headerID;             // Header
  bool            fullDutyAtLevel;      // PWM full output level
  uint16_t        ignorefirst;          // iIgnore the first n PWM transisions
  byte            servoDir;             // Normal or reverse
  byte            linOrCurve;           // Linear move or curve points
  int16_t         cp[SERVO_CURVE_PTS];  // Curve points
  int16_t         servoTrimLo;          // End trim low
  int16_t         servoTrimHi;          // end trim high
  int16_t         dualRateFactor;       // Rate in % of full output
  int16_t         midPoint;             // Servo midpoint in uS (1500 is standard)
  uint32_t        _crc;
  } fanToServoSetup_t;

// globals
extern fanToServoSetup_t  f2sSetup;   // saved setup structure

// Prototypes
extern void       initSignalProcessing(void);
extern int16_t    getWaveformData(bool, waveFormData_t *);
extern int        servoPosSet(int16_t);
extern bool       menusysRunLoop(void);
extern void       menusysSetup(void);
extern void       printonLCDatPos(char *, int8_t, int8_t);
extern int        getPhysicalServoSpan(void);
extern int        getPhysicalServoMin(void);
extern int        getPhysicalServoMax(void);


#endif /* APP_H_ */