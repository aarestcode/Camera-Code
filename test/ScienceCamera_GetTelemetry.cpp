/***************************************************************************//**
 * @file	ScienceCamera_GetTelemetry.cpp
 * @brief	Test file to read all telemetry
 *
 * @author	Thibaud Talon
 * @date	21/09/2017
 *
 *******************************************************************************/

#include "UserInterface.hpp"
#include "ImagingCamera.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("ScienceCamera_GetTelemetry.cpp");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc > 1) log.printf("WARNING: Extra inputs discarded");

    ImagingCamera_Error error;

    // 2. Connect camera
    log.printf("2. Connect camera");
    ImagingCamera ScienceCamera(IMAGINGCAMERA_SCIENCE_CAMERA);
    if( ScienceCamera.status != IMAGINGCAMERA_ON ) return log.error("Error connecting to camera", ScienceCamera.status);

    // 3. Get telemetry
    log.printf("3. Get telemetry");
    ImagingCamera_Telemetry telemetry;
    if( error = ScienceCamera.getTelemetry(telemetry) ) return log.error("Could not get telemetry", error);

    return log.success();
}
