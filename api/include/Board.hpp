/***************************************************************************//**
 * @file	Board.hpp
 * @brief	Header file to actuate the the AAReST Telescope Camera boards
 *
 * This header file contains all the required definitions and function prototypes
 * through which to control the AAReST Camera boards
 *
 * @author	Thibaud Talon
 * @date	27/09/2017
 *******************************************************************************/

#ifndef BOARD_H
#define BOARD_H

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   27/09/2017
 *
 * GPIO Parameters
 ******************************************************************************/
#ifndef OK
#define OK 0
#endif

enum GPIO_Error{
    OK_GPIO = 0,
    ERR_GPIO_EXPORT_OPEN,
    ERR_GPIO_UNEXPORT_OPEN,
    ERR_GPIO_SETDIR_OPEN,
    ERR_GPIO_SETVAL_OPEN,
    ERR_GPIO_GETVAL_OPEN,
};

enum GPIO_Direction{
    GPIO_INPUT = 0,
    GPIO_OUTPUT = 1
};

enum GPIO_Status{
    GPIO_ON = 0,
    GPIO_OFF = 1,
    GPIO_ERROR = 2
};


/***************************************************************************//**
 * @author Thibaud Talon
 * @date   27/09/2017
 *
 * GPIO Class
 ******************************************************************************/
class GPIO{
public:
    GPIO_Status status;
    char pin[3];
    int num;
    bool verbose;
    GPIO_Direction dir;

    GPIO (void) {verbose = 1; status = GPIO_OFF;} // Constructor to create the object without connecting
    GPIO (const char* _pin, GPIO_Direction _dir) {verbose = 1; connect(_pin, _dir);} // Constructor to connect the pin
    GPIO (int _num, GPIO_Direction _dir) {verbose = 1; connect(_num, _dir);} // Constructor to connect the pin
    ~GPIO (void) {disconnect();} // Destructor to safely disconnect the pin

    GPIO_Error connect(const char* _pin, GPIO_Direction _dir); // Connect the pin
    GPIO_Error connect(int _num, GPIO_Direction _dir); // Connect the pin
    GPIO_Error disconnect(void); // Disconnect to the pin
    GPIO_Error reset(void); // Rest connection to the pin

    GPIO_Error set(int val); // Set value of the pin
    GPIO_Error get(int & val); // Get value of the pin

};

#endif
