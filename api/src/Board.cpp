/***************************************************************************//**
 * @file	Board.cpp
 * @brief	Source file to manage the boards
 *
 * This file contains all the implementations for the functions defined in:
 * api/include/Board.hpp
 *
 * @author	Thibaud Talon
 * @date	27/09/2017
 *******************************************************************************/

#include "Board.hpp"
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include "UserInterface.hpp"

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   27/09/2017
 *
 * Connect the pin
 * http://hertaville.com/introduction-to-accessing-the-raspberry-pis-gpio-in-c.html
 *
 ******************************************************************************/
GPIO_Error GPIO::connect(const char * _pin, GPIO_Direction _dir){
    UserInterface::Log log("GPIO::connect");

    status = GPIO_OFF;
    sprintf(pin, "%s", _pin);
    num = ((int)(pin[0]) - 65)*32 + atoi(&pin[1]);
    dir = _dir;

    // 1. Export GPIO
    log.printf("1. Export GPIO %s (%i)", pin, num);
    std::ofstream exportgpio("/sys/class/gpio/export");

    if (exportgpio < 0) {status = GPIO_ERROR; return (GPIO_Error) log.error("Cannot open exoprt file", ERR_GPIO_EXPORT_OPEN);}

    char buffer[50];
    sprintf(buffer, "%i", num);

    exportgpio << buffer ; //write GPIO number to export
    exportgpio.close(); //close export file

    // 2. Set direction
    log.printf("2. Set direction to %i", _dir);

    sprintf(buffer, "/sys/class/gpio/pio%s/direction", pin);
    std::ofstream setdirgpio(buffer);

    if (setdirgpio < 0) {status = GPIO_ERROR; return (GPIO_Error) log.error("Cannot open direction file", ERR_GPIO_SETDIR_OPEN);}

    if (_dir == GPIO_INPUT) setdirgpio << "in"; //write direction to direction file
    if (_dir == GPIO_OUTPUT) setdirgpio << "out"; //write direction to direction file
    setdirgpio.close(); // close direction file

    status = GPIO_ON;

    return (GPIO_Error) log.success();
}
GPIO_Error GPIO::connect(int _num, GPIO_Direction _dir){
    UserInterface::Log log("GPIO::connect");

    status = GPIO_OFF;
    num = _num;
    sprintf(pin, "%c%i", num/32 + 65, num%32);
    dir = _dir;

    // 1. Export GPIO
    log.printf("1. Export GPIO %s (%i)", pin, num);
    std::ofstream exportgpio("/sys/class/gpio/export");

    if (exportgpio < 0) {status = GPIO_ERROR; return (GPIO_Error) log.error("Cannot open exoprt file", ERR_GPIO_EXPORT_OPEN);}

    char buffer[50];
    sprintf(buffer, "%i", num);

    exportgpio << buffer ; //write GPIO number to export
    exportgpio.close(); //close export file

    // 2. Set direction
    log.printf("2. Set direction to %i", _dir);
    sprintf(buffer, "/sys/class/gpio/pio%s/direction", pin);
    std::ofstream setdirgpio(buffer);

    if (setdirgpio < 0) {status = GPIO_ERROR; return (GPIO_Error) log.error("Cannot open direction file", ERR_GPIO_SETDIR_OPEN);}

    if (_dir == GPIO_INPUT) setdirgpio << "in"; //write direction to direction file
    if (_dir == GPIO_OUTPUT) setdirgpio << "out"; //write direction to direction file
    setdirgpio.close(); // close direction file

    status = GPIO_ON;

    return (GPIO_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   27/09/2017
 *
 * Disconnect the pin
 * http://hertaville.com/introduction-to-accessing-the-raspberry-pis-gpio-in-c.html
 *
 ******************************************************************************/
GPIO_Error GPIO::disconnect(void){
    UserInterface::Log log("GPIO::disconnect");

    log.printf("Unexport GPIO %s (%i)", pin, num);
    std::ofstream unexportgpio("/sys/class/gpio/unexport");

    if (unexportgpio < 0) {status = GPIO_ERROR; return (GPIO_Error) log.error("Cannot open unexport file", ERR_GPIO_UNEXPORT_OPEN);}

    char buffer[5];
    sprintf(buffer, "%i", num);

    unexportgpio << buffer ; //write GPIO number to unexport
    unexportgpio.close(); //close unexport file

    status = GPIO_OFF;

    return (GPIO_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   20/09/2017
 *
 * Reset the pin
 *
 ******************************************************************************/
GPIO_Error GPIO::reset(void){
    disconnect();
    return connect(num, dir);
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   27/09/2017
 *
 * Set value of the pin
 * http://hertaville.com/introduction-to-accessing-the-raspberry-pis-gpio-in-c.html
 *
 * @param [in] val
 *	Value of the pin
 ******************************************************************************/
GPIO_Error GPIO::set(int val){
    UserInterface::Log log("GPIO::set");

    char buffer[50];
    sprintf(buffer, "/sys/class/gpio/pio%s/value", pin);
    std::ofstream setvalgpio(buffer);

    if (setvalgpio < 0) {status = GPIO_ERROR; return (GPIO_Error) log.error("Cannot open GPIO value file", ERR_GPIO_SETVAL_OPEN);}

    sprintf(buffer, "%i", val);
    setvalgpio << buffer ;//write value to value file
    setvalgpio.close();// close value file

    if (verbose) return (GPIO_Error) log.success();
    return OK_GPIO;
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   27/09/2017
 *
 * Get value of the pin
 * http://hertaville.com/introduction-to-accessing-the-raspberry-pis-gpio-in-c.html
 *
 * @param [out] val
 *	pointer to the read value
 ******************************************************************************/
GPIO_Error GPIO::get(int& val){
    UserInterface::Log log("GPIO::get");

    char buffer[50];
    sprintf(buffer, "/sys/class/gpio/pio%s/value", pin);
    std::ifstream getvalgpio(buffer);

    if (getvalgpio < 0) {status = GPIO_ERROR; return (GPIO_Error) log.error("Cannot open GPIO value file", ERR_GPIO_GETVAL_OPEN);}

    getvalgpio >> buffer ;  //read gpio value

    val = atoi(buffer);
    if (verbose) log.printf("Value of pin %s (%i) = %i", pin, num, val);

    getvalgpio.close(); //close the value file

    if (verbose) return (GPIO_Error) log.success();
    return OK_GPIO;
}

