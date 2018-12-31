/*
 * Menusys.h
 *
 * Created: 2018-12-14 20:57:44
 *  Author: Thom
 */ 

#pragma once

#include <MD_Menu.h>

const bool      AUTO_START = true;      // auto start the menu, manual detect and start if false
const uint16_t  MENU_TIMEOUT = 35000;   // in milliseconds (35 sec)
const uint8_t   LED_PIN = LED_BUILTIN;  // for myLEDCode function

// Function prototypes for Navigation/Display
bool                     LCDdisplay(MD_Menu::userDisplayAction_t action, char *msg);
MD_Menu::userNavAction_t menuNavigation(uint16_t &incDelta);

// Function prototypes for variable get/set functions
// Input PWM
MD_Menu::value_t *mnuDutyRqst(MD_Menu::mnuId_t id, bool bGet);
MD_Menu::value_t *mnuSkipRqst(MD_Menu::mnuId_t id, bool bGet);

// Servo
MD_Menu::value_t *mnuSDirRqst(MD_Menu::mnuId_t id, bool bGet);
MD_Menu::value_t *mnuSMovTypeRqst(MD_Menu::mnuId_t id, bool bGet);
MD_Menu::value_t *mnuCurvePtRqst(MD_Menu::mnuId_t id, bool bGet);
MD_Menu::value_t *mnuTrimRqst(MD_Menu::mnuId_t id, bool bGet);
MD_Menu::value_t *mnuRateRqst(MD_Menu::mnuId_t id, bool bGet);
MD_Menu::value_t *mnuMidPointRqst(MD_Menu::mnuId_t id, bool bGet);

