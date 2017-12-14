/***************************************************************************//**
 * @file	ScienceCamera_GainSetup.cpp
 * @brief	Test file to read and set the gain
 *
 * @author	Thibaud Talon
 * @date	21/09/2017
 *
 * @param [in] gain_dB
 *	Gain in dB
 *******************************************************************************/

#include "UserInterface.hpp"
#include "ImagingCamera.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("Algorithm");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc < 2) return log.error("No gain (dB) specified",-1);
    else if(argc > 2) log.printf("WARNING: Extra inputs discarded");

    ImagingCamera_Error error;

    // 2. Connect camera
    log.printf("2. Connect camera");
    ImagingCamera ScienceCamera(IMAGINGCAMERA_SCIENCE_CAMERA);
    if( ScienceCamera.status != IMAGINGCAMERA_ON ) return log.error("Error connecting to camera", ScienceCamera.status);

    float gain;

    // 3. Read gain
    log.printf("3. Read gain");
    if( error = ScienceCamera.getGain(gain) ) return log.error("Could not read gain", error);

    // 4. Set gain
    log.printf("4. Set gain");
    if( error = ScienceCamera.setGain(atof(argv[1])) ) return log.error("Could not set gain", error);

    // 5. Read gain
    log.printf("5. Read gain");
    if( error = ScienceCamera.getGain(gain) ) return log.error("Could not read gain", error);

    return log.success();
}
