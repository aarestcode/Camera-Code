/***************************************************************************//**
 * @file	Mask.hpp
 * @brief	Header file to actuate the mask motor in the Telescope Camera of
 * 		AAReST
 *
 * This header file contains all the required definitions and function prototypes
 * through which to control the AAReST Camera mask motor
 *
 * @author	Thibaud Talon
 * @date	20/09/2017
 *******************************************************************************/

#ifndef MASK_H
#define MASK_H

#include "Board.hpp"

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   20/09/2017
 *
 * Parameters
 ******************************************************************************/
#ifndef OK
#define OK 0
#endif

enum Mask_Position{
    MASK_NARROW = 0,
    MASK_WIDE = 1,
    MASK_INTER = 2,
    MASK_CRITICAL = -1
};

enum Mask_Error{
    OK_MASK = 0,
    ERR_MASK_CONNECT_PIN,
    ERR_MASK_SET_PIN,
    ERR_MASK_GET_PIN,
    ERR_MASK_DISCONNECT_PIN,
    ERR_MASK_CONTROLLER_OFF,
    ERR_MASK_WRONG_POSITION,
    ERR_MASK_LIMIT_SWITCHES_BOTH_ON,
};

enum Mask_Resolution{
    MASK_FULL_STEP = 0,
    MASK_HALF_STEP = 1,
    MASK_QUARTER_STEP = 2,
    MASK_EIGHTH_STEP = 3,
    MASK_SIXTEENTH_STEP = 4,
};

enum Mask_Status{
    MASK_ON = 0,
    MASK_OFF = 1,
    MASK_ERROR = 2,
};

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   20/09/2017
 *
 * Class
 ******************************************************************************/
class Mask{
public:
    Mask_Status status; // Status of connection

    Mask (void) {status = MASK_OFF;} // Constructor to create the object without connecting
    Mask (int) {connect();} // Constructor to connect the controller
    ~Mask (void) {disconnect();} // Destructor to safely disconnect the controller
    Mask_Error connect(void); // Connect to the controller
    Mask_Error disconnect(void); // Disconnect to the controller
    Mask_Error reset(void); // Rest connection to the controller
    Mask_Error moveTo(Mask_Position pos); // Move mask to one of its too position
    Mask_Error getPosition(Mask_Position &pos); // Get the current position of the mask
    Mask_Error moveSteps(int steps, int speed, Mask_Resolution resolution); // Move the mask of a certain amount of steps at some defined speed (debug function)

private:
    GPIO enable_gpio;
    GPIO reset_gpio;
    GPIO sleep_gpio;
    GPIO ms1_gpio;
    GPIO ms2_gpio;
    GPIO ms3_gpio;
    GPIO narrow_gpio;
    GPIO wide_gpio;
    GPIO step_gpio;
    GPIO direction_gpio;
    void delay(int millis); // Delay function in milliseconds
};

#endif
