/***************************************************************************//**
 * @file	SienceCamera_ROISetup.cpp
 * @brief	Test file to reas and set the region of interest
 *
 * @author	Thibaud Talon
 * @date	21/09/2017
 *
 * @param [in] steps
 *	number of steps to move
 * @param [in] speed
 *	frequency of step signal
 * @param [in] resolution
 *	step resolution (see /api/include/Mask.hpp)
 *******************************************************************************/

#include "UserInterface.hpp"
#include "Mask.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("Algorithm");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc < 4) return log.error("No steps and speed specified",-1);
    else if(argc > 4) log.printf("WARNING: Extra inputs discarded");

    Mask_Error error;

    // 2. Turn controller ON
    log.printf("2. Turn controller ON");
    Mask motor(0);
    if( motor.status != MASK_ON ) return log.error("Error turning controller ON", motor.status);

    // 3. Turn motor
    log.printf("3. Turn motor");
    if( error = motor.moveSteps(atoi(argv[1]), atoi(argv[2]) , (Mask_Resolution)atoi(argv[3]))) return log.error("Could not move motor", error);

    return (int)log.success();
}
