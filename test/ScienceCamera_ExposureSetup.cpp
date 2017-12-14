/***************************************************************************//**
 * @file	ScienceCamera_ExposureSetup.cpp
 * @brief	Test file to read and set the exposure
 *
 * @author	Thibaud Talon
 * @date	21/09/2017
 *
 * @param [in] exposure_us
 *	Exposure in us
 *******************************************************************************/

#include "UserInterface.hpp"
#include "ImagingCamera.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("Algorithm");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc < 2) return log.error("No exposure (us) specified",-1);
    else if(argc > 2) log.printf("WARNING: Extra inputs discarded");

    ImagingCamera_Error error;

    // 2. Connect camera
    log.printf("2. Connect camera");
    ImagingCamera ScienceCamera(IMAGINGCAMERA_SCIENCE_CAMERA);
    if( ScienceCamera.status != IMAGINGCAMERA_ON ) return log.error("Error connecting to camera", ScienceCamera.status);

    int exposure;

    // 3. Read exposure
    log.printf("3. Read exposure");
    if( error = ScienceCamera.getExposure(exposure) ) return log.error("Could not read exposure", error);

    // 4. Set exposure
    log.printf("4. Set exposure");
    if( error = ScienceCamera.setExposure(atoi(argv[1])) ) return log.error("Could not set exposure", error);

    // 5. Read exposure
    log.printf("5. Read exposure");
    if( error = ScienceCamera.getExposure(exposure) ) return log.error("Could not read exposure", error);

    return log.success();
}
