/***************************************************************************//**
 * @file	SHWS_pY__GetTelemetry.cpp
 * @brief	Test file to read all telemetry
 *
 * @author	Thibaud Talon
 * @date	26/09/2017
 *
 *******************************************************************************/

#include "UserInterface.hpp"
#include "SHWSCamera.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("SHWS_pY__GetTelemetry.cpp");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc > 1) log.printf("WARNING: Extra inputs discarded");

    SHWSCamera_Error error;

    // 2. Connect camera
    log.printf("2. Connect camera");
    SHWSCamera shws(SHWSCAMERA_pY_APERTURE);
    if( shws.status != SHWSCAMERA_ON ) return log.error("Error connecting to camera", shws.status);

    // 3. Get telemetry
    log.printf("3. Get telemetry");
    SHWSCamera_Telemetry telemetry;
    if( error = shws.getTelemetry(telemetry) ) return log.error("Could not get telemetry", error);

    return log.success();
}
