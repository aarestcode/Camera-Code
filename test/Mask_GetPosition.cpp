/***************************************************************************//**
 * @file	Mask_GetPosition.cpp
 * @brief	Test file to get current position of the mask
 *
 * @author	Thibaud Talon
 * @date	21/09/2017
 *
 *******************************************************************************/

#include "UserInterface.hpp"
#include "Mask.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("Algorithm");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
     if(argc > 1) log.printf("WARNING: Extra inputs discarded");

    Mask_Error error;

    // 2. Turn controller ON
    log.printf("2. Turn controller ON");
    Mask motor(0);
    if( motor.status != MASK_ON ) return log.error("Error turning controller ON", motor.status);

    // 3. Get position
    Mask_Position pos;
    log.printf("3. Get position");
    if( error = motor.getPosition(pos) ) return log.error("Could not get position", error);

    return log.success();
}
