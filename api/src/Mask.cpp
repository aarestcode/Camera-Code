/***************************************************************************//**
 * @file	Mask.cpp
 * @brief	Source file to actuate the mask motor in the Telescope Camera of AAReST
 *
 * This file contains all the implementations for the functions defined in:
 * api/include/Mask.hpp
 *
 * @author	Thibaud Talon
 * @date	20/09/2017
 *******************************************************************************/

#include "Mask.hpp"
#include "Board.hpp"
#include "AAReST.hpp"
#include <time.h>
#include "UserInterface.hpp"

#define MASK_ENABLE "E23" // PE23
#define MASK_RESET "E24" // PE24
#define MASK_SLEEP "E26" // PE26
#define MASK_MS1 "C12" // PC12
#define MASK_MS2 "C14" // PC14
#define MASK_MS3 "C10" // Pc10
#define MASK_LIMIT_SWITCH_NARROW "A2" // PA2
#define MASK_LIMIT_SWITCH_WIDE "A5" // PA5
#define MASK_STEP "D28" // PD28
#define MASK_DIRECTION "D29" // PD29

#define MASK_DIRECTION_NARROW 0
#define MASK_DIRECTION_WIDE 1

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   20/09/2017
 *
 * Connect the controller
 *
 ******************************************************************************/
Mask_Error Mask::connect(void){
    UserInterface::Log log("Mask::connect");

    status = MASK_OFF;

    // 1. Connect GPIO pins
    log.printf("1. Connect GPIO pins");
    if (enable_gpio.connect(MASK_ENABLE, GPIO_OUTPUT)) {status = MASK_ERROR; log.error("Cannot connect Enable pin", ERR_MASK_CONNECT_PIN);}
    if (reset_gpio.connect(MASK_RESET, GPIO_OUTPUT)) {status = MASK_ERROR; log.error("Cannot connect Rest pin", ERR_MASK_CONNECT_PIN);}
    if (sleep_gpio.connect(MASK_SLEEP, GPIO_OUTPUT)) {status = MASK_ERROR; log.error("Cannot connect Sleep pin", ERR_MASK_CONNECT_PIN);}
    if (ms1_gpio.connect(MASK_MS1, GPIO_OUTPUT)) {status = MASK_ERROR; log.error("Cannot connect MS1 pin", ERR_MASK_CONNECT_PIN);}
    if (ms2_gpio.connect(MASK_MS2, GPIO_OUTPUT)) {status = MASK_ERROR; log.error("Cannot connect MS2 pin", ERR_MASK_CONNECT_PIN);}
    if (ms3_gpio.connect(MASK_MS3, GPIO_OUTPUT)) {status = MASK_ERROR; log.error("Cannot connect MS3 pin", ERR_MASK_CONNECT_PIN);}
    if (narrow_gpio.connect(MASK_LIMIT_SWITCH_NARROW, GPIO_INPUT)) {status = MASK_ERROR; log.error("Cannot connect Limit Switch 1 pin", ERR_MASK_CONNECT_PIN);}
    if (wide_gpio.connect(MASK_LIMIT_SWITCH_WIDE, GPIO_INPUT)) {status = MASK_ERROR; log.error("Cannot connect Limit Switch 2 pin", ERR_MASK_CONNECT_PIN);}
    if (step_gpio.connect(MASK_STEP, GPIO_OUTPUT)) {status = MASK_ERROR; log.error("Cannot connect Step pin", ERR_MASK_CONNECT_PIN);}
    if (direction_gpio.connect(MASK_DIRECTION, GPIO_OUTPUT)) {status = MASK_ERROR; log.error("Cannot connect Direction pin", ERR_MASK_CONNECT_PIN);}
    step_gpio.verbose = 0; // Otherwise it's too much for the log file
    narrow_gpio.verbose = 0;
    wide_gpio.verbose = 0;

    // 2. Enable controller
    log.printf("2. Enable controller");
    if (enable_gpio.set(0)) {disconnect(); return (Mask_Error) log.error("Cannot Enable controller", ERR_MASK_SET_PIN);}
    if (step_gpio.set(0)) {disconnect(); return (Mask_Error) log.error("Cannot set Step to low", ERR_MASK_SET_PIN);}

    // 3. Disable RESET of controller
    log.printf("3. Disable RESET of controller");
    if (reset_gpio.set(1)) {disconnect(); return (Mask_Error) log.error("Cannot disable reset", ERR_MASK_SET_PIN);}

    // 4. Put controller in sleep mode
    log.printf("4. Put controller in sleep mode");
    if (sleep_gpio.set(0)) {status = MASK_ERROR; return (Mask_Error) log.error("Cannot put controller in sleep mode", ERR_MASK_SET_PIN);}

    status = MASK_ON;

    return (Mask_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   20/09/2017
 *
 * Disconnect the controller
 *
 ******************************************************************************/
Mask_Error Mask::disconnect(void){
    UserInterface::Log log("Mask::disconnect");

    // 1. Unexport GPIO pins
    log.printf("Disconnect GPIO pins");
    if (enable_gpio.disconnect()) {status = MASK_ERROR; log.error("Cannot disconnect Enable pin", ERR_MASK_DISCONNECT_PIN);}
    if (reset_gpio.disconnect()) {status = MASK_ERROR; log.error("Cannot disconnect Rest pin", ERR_MASK_DISCONNECT_PIN);}
    if (sleep_gpio.disconnect()) {status = MASK_ERROR; log.error("Cannot disconnect Sleep pin", ERR_MASK_DISCONNECT_PIN);}
    if (ms1_gpio.disconnect()) {status = MASK_ERROR; log.error("Cannot disconnect MS1 pin", ERR_MASK_DISCONNECT_PIN);}
    if (ms2_gpio.disconnect()) {status = MASK_ERROR; log.error("Cannot disconnect MS2 pin", ERR_MASK_DISCONNECT_PIN);}
    if (ms3_gpio.disconnect()) {status = MASK_ERROR; log.error("Cannot disconnect MS3 pin", ERR_MASK_DISCONNECT_PIN);}
    if (narrow_gpio.disconnect()) {status = MASK_ERROR; log.error("Cannot disconnect Limit Switch 1 pin", ERR_MASK_DISCONNECT_PIN);}
    if (wide_gpio.disconnect()) {status = MASK_ERROR; log.error("Cannot disconnect Limit Switch 2 pin", ERR_MASK_DISCONNECT_PIN);}
    if (step_gpio.disconnect()) {status = MASK_ERROR; log.error("Cannot disconnect Step pin", ERR_MASK_DISCONNECT_PIN);}
    if (direction_gpio.disconnect()) {status = MASK_ERROR; log.error("Cannot disconnect Direction pin", ERR_MASK_DISCONNECT_PIN);}

    status = MASK_OFF;

    return (Mask_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   20/09/2017
 *
 * Reset the controller
 *
 ******************************************************************************/
Mask_Error Mask::reset(void){
    disconnect();
    return connect();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   20/09/2017
 *
 * Move the mask to one of its 2 positions
 *
 * @param [in] pos
 *	position of the mask = MASK_NARROW or MASK_WIDE
 ******************************************************************************/
Mask_Error Mask::moveTo(Mask_Position pos){
    UserInterface::Log log("Mask::moveTo");

    if (status == MASK_OFF) return (Mask_Error) log.error("Controller is OFF", ERR_MASK_CONTROLLER_OFF);

    // 1. Set step size and direction
    log.printf("1. Set step size and direction");
    GPIO * limit_switch;
    if (pos == MASK_NARROW){
        if (direction_gpio.set(MASK_DIRECTION_NARROW)) { disconnect(); return (Mask_Error) log.error("Cannot set direction of motor", ERR_MASK_SET_PIN);}
        limit_switch = &narrow_gpio;
    }
    else if (pos == MASK_WIDE){
        if (direction_gpio.set(MASK_DIRECTION_WIDE)) { disconnect(); return (Mask_Error) log.error("Cannot set direction of motor", ERR_MASK_SET_PIN);}
        limit_switch = &wide_gpio;
    }
    else return (Mask_Error) log.error("Wrong position", ERR_MASK_WRONG_POSITION);
    switch ((Mask_Resolution) MASK_RESOLUTION){
    case MASK_FULL_STEP:
        if (ms1_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot set MS1", ERR_MASK_SET_PIN);}
        if (ms2_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot set MS2", ERR_MASK_SET_PIN);}
        if (ms3_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot set MS3", ERR_MASK_SET_PIN);}
        break;
    case MASK_HALF_STEP:
        if (ms1_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot set MS1", ERR_MASK_SET_PIN);}
        if (ms2_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot set MS2", ERR_MASK_SET_PIN);}
        if (ms3_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot set MS3", ERR_MASK_SET_PIN);}
        break;
    case MASK_QUARTER_STEP:
        if (ms1_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot set MS1", ERR_MASK_SET_PIN);}
        if (ms2_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot set MS2", ERR_MASK_SET_PIN);}
        if (ms3_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot set MS3", ERR_MASK_SET_PIN);}
        break;
    case MASK_EIGHTH_STEP:
        if (ms1_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot set MS1", ERR_MASK_SET_PIN);}
        if (ms2_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot set MS2", ERR_MASK_SET_PIN);}
        if (ms3_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot set MS3", ERR_MASK_SET_PIN);}
        break;
    case MASK_SIXTEENTH_STEP:
    default:
        if (ms1_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot set MS1", ERR_MASK_SET_PIN);}
        if (ms2_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot set MS2", ERR_MASK_SET_PIN);}
        if (ms3_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot set MS3", ERR_MASK_SET_PIN);}
        break;
    }

    // 2. Wake up controller
    log.printf("2. Wake up controller");
    if (sleep_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot set wake up controller", ERR_MASK_SET_PIN);}
    delay(1); // wait 1 ms

    // 3. Turn mask
    log.printf("3. Turn mask");
    if (step_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot take step", ERR_MASK_SET_PIN);}
    int limit;
    if ((*limit_switch).get(limit)) { disconnect(); return (Mask_Error) log.error("Cannot read limit switch", ERR_MASK_GET_PIN);}
    while(limit == 0){
        if (step_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot take step", ERR_MASK_SET_PIN);}
        delay(1000./(2.*MASK_SPEED));
        if (step_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot take step", ERR_MASK_SET_PIN);}
        delay(1000./(2.*MASK_SPEED));
        if ((*limit_switch).get(limit)) { disconnect(); return (Mask_Error) log.error("Cannot read limit switch", ERR_MASK_GET_PIN);}
    }

    // 4. Put controller in sleep mode
    log.printf("4. Put controller in sleep mode");
    if (sleep_gpio.set(0)) {status = MASK_ERROR; return (Mask_Error) log.error("Cannot put controller in sleep mode", ERR_MASK_SET_PIN);}

    return (Mask_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   20/09/2017
 *
 * Get the position of the mask
 *
 * @param [out] pos
 *	pointer to the position of the mask
 ******************************************************************************/
Mask_Error Mask::getPosition(Mask_Position &pos){
    UserInterface::Log log("Mask::getPosition");

    int limit_narrow, limit_wide;
    if (narrow_gpio.get(limit_narrow)) {return (Mask_Error) log.error("Cannot read limit switch 1", ERR_MASK_GET_PIN);}
    if (wide_gpio.get(limit_wide)) {return (Mask_Error) log.error("Cannot read limit switch 2", ERR_MASK_GET_PIN);}

    if (limit_narrow == 0 && limit_wide == 0) {pos = MASK_INTER; log.printf("Mask in intermediate position");}
    else if (limit_narrow == 1 && limit_wide == 0) {pos = MASK_NARROW; log.printf("Mask in narrow configuration");}
    else if (limit_narrow == 0 && limit_wide == 1) {pos = MASK_WIDE; log.printf("Mask in wide configuration");}
    else if (limit_narrow == 1 && limit_wide == 1) {pos = MASK_CRITICAL; return (Mask_Error) log.error("Both limit switches pressed", ERR_MASK_LIMIT_SWITCHES_BOTH_ON);}

    return (Mask_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   20/09/2017
 *
 * Get the position of the mask
 *
 * @param [in] steps
 *	number of steps to move
 * @param [in] speed
 *	frequency of step signal
 * @param [in] resolution
 *	step resolution
 ******************************************************************************/
Mask_Error Mask::moveSteps(int steps, int speed, Mask_Resolution resolution){
    UserInterface::Log log("Mask::moveSteps");

    if (status == MASK_OFF) return (Mask_Error) log.error("Controller is OFF", ERR_MASK_CONTROLLER_OFF);

    // 1. Set step size and direction
    log.printf("1. Set step size and direction");
    GPIO * limit_switch;
    if (steps > 0){
        if (direction_gpio.set(MASK_DIRECTION_NARROW)) { disconnect(); return (Mask_Error) log.error("Cannot set direction of motor", ERR_MASK_SET_PIN);}
        limit_switch = &narrow_gpio;
    }
    else if (steps < 0){
        if (direction_gpio.set(MASK_DIRECTION_WIDE)) { disconnect(); return (Mask_Error) log.error("Cannot set direction of motor", ERR_MASK_SET_PIN);}
        limit_switch = &wide_gpio;
    }
    else return (Mask_Error) log.success();
    switch (resolution){
    case MASK_FULL_STEP:
        if (ms1_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot set MS1", ERR_MASK_SET_PIN);}
        if (ms2_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot set MS2", ERR_MASK_SET_PIN);}
        if (ms3_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot set MS3", ERR_MASK_SET_PIN);}
        break;
    case MASK_HALF_STEP:
        if (ms1_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot set MS1", ERR_MASK_SET_PIN);}
        if (ms2_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot set MS2", ERR_MASK_SET_PIN);}
        if (ms3_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot set MS3", ERR_MASK_SET_PIN);}
        break;
    case MASK_QUARTER_STEP:
        if (ms1_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot set MS1", ERR_MASK_SET_PIN);}
        if (ms2_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot set MS2", ERR_MASK_SET_PIN);}
        if (ms3_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot set MS3", ERR_MASK_SET_PIN);}
        break;
    case MASK_EIGHTH_STEP:
        if (ms1_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot set MS1", ERR_MASK_SET_PIN);}
        if (ms2_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot set MS2", ERR_MASK_SET_PIN);}
        if (ms3_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot set MS3", ERR_MASK_SET_PIN);}
        break;
    case MASK_SIXTEENTH_STEP:
    default:
        if (ms1_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot set MS1", ERR_MASK_SET_PIN);}
        if (ms2_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot set MS2", ERR_MASK_SET_PIN);}
        if (ms3_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot set MS3", ERR_MASK_SET_PIN);}
        break;
    }

    // 2. Wake up controller
    log.printf("2. Wake up controller");
    if (sleep_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot set wake up controller", ERR_MASK_SET_PIN);}
    delay(1); // wait 1 ms

    // 3. Turn masklog.printf("3. Turn mask");
    if (step_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot take step", ERR_MASK_SET_PIN);}
    int limit;
    int count = 0;
    if ((*limit_switch).get(limit)) { disconnect(); return (Mask_Error) log.error("Cannot read limit switch", ERR_MASK_GET_PIN);}
    log.printf("Limit = %i", limit);
    while(limit == 0 && count++ < abs(steps)){
        if (step_gpio.set(1)) { disconnect(); return (Mask_Error) log.error("Cannot take step", ERR_MASK_SET_PIN);}
        delay(1000./(2.*speed));
        if (step_gpio.set(0)) { disconnect(); return (Mask_Error) log.error("Cannot take step", ERR_MASK_SET_PIN);}
        delay(1000./(2.*speed));
        if ((*limit_switch).get(limit)) { disconnect(); return (Mask_Error) log.error("Cannot read limit switch", ERR_MASK_GET_PIN);}
    }
    log.printf("Limit = %i", limit);

    // 4. Put controller in sleep mode
    log.printf("4. Put controller in sleep mode");
    if (sleep_gpio.set(0)) {status = MASK_ERROR; return (Mask_Error) log.error("Cannot put controller in sleep mode", ERR_MASK_SET_PIN);}

    return (Mask_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   20/09/2017
 *
 * Delay function in millis
 * http://c-for-dummies.com/blog/?p=69
 *
 * @param [in] millis
 *	time to wait in milliseconds
 ******************************************************************************/
void Mask::delay(int millis){
    long pause;
    clock_t now,then;

    pause = millis*(CLOCKS_PER_SEC/1000);
    now = then = clock();
    while( (now-then) < pause )
        now = clock();
}
