/***************************************************************************//**
 * @file	SHWS_pY_ExposureSetup.cpp
 * @brief	Test file to read and set the exposure
 *
 * @author	Thibaud Talon
 * @date	25/09/2017
 *
 * @param [in] exposure_us
 *	Exposure in us
 *******************************************************************************/

#include "UserInterface.hpp"
#include "SHWSCamera.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("SHWS_pY_ExposureSetup");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc < 2) return log.error("No exposure (us) specified",-1);
    else if(argc > 2) log.printf("WARNING: Extra inputs discarded");

    SHWSCamera_Error error;

    // 2. Connect camera
    log.printf("2. Connect camera");
    SHWSCamera shws(SHWSCAMERA_pY_APERTURE);
    if( shws.status != SHWSCAMERA_ON ) return log.error("Error connecting to camera", shws.status);

    int exposure;

    // 3. Read exposure
    log.printf("3. Read exposure");
    if( error = shws.getExposure(exposure) ) return log.error("Could not read exposure", error);

    // 4. Set exposure
    log.printf("4. Set exposure");
    if( error = shws.setExposure(atoi(argv[1])) ) return log.error("Could not set exposure", error);

    // 5. Read exposure
    log.printf("5. Read exposure");
    if( error = shws.getExposure(exposure) ) return log.error("Could not read exposure", error);

    return log.success();
}
