/***************************************************************************//**
 * @file	BIC_ReadParameters.cpp
 * @brief	Test file to read all parameters
 *
 * @author	Thibaud Talon
 * @date	21/09/2017
 *
 *******************************************************************************/

#include "UserInterface.hpp"
#include "ImagingCamera.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("BIC_ReadParameters");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc > 1) log.printf("WARNING: Extra inputs discarded");

    ImagingCamera_Error error;

    // 2. Connect camera
    log.printf("2. Connect camera");
    ImagingCamera BoomInspectionCamera(IMAGINGCAMERA_BOOM_INSPECTION_CAMERA);
    if( BoomInspectionCamera.status != IMAGINGCAMERA_ON ) return log.error("Error connecting to camera", BoomInspectionCamera.status);

    int iVal;
    float fVal;

    // 3. Read timeout
    log.printf("3. Read timeout");
    if( error = BoomInspectionCamera.getTimeout(iVal) ) return log.error("Could not read timeout", error);

    // 4. Read width
    log.printf("4. Read width");
    if( error = BoomInspectionCamera.getWidth(iVal) ) return log.error("Could not read width", error);

    // 5. Read height
    log.printf("5. Read height");
    if( error = BoomInspectionCamera.getHeight(iVal) ) return log.error("Could not read height", error);

    // 6. Read horizontal offset
    log.printf("6. Read horizontal offset");
    if( error = BoomInspectionCamera.getOffsetX(iVal) ) return log.error("Could not read horizontal offset", error);

    // 7. Read vertical offset
    log.printf("7. Read vertical offset");
    if( error = BoomInspectionCamera.getOffsetY(iVal) ) return log.error("Could not read vertical offset", error);

    // 8. Read gain
    log.printf("8. Read gain");
    if( error = BoomInspectionCamera.getGain(fVal) ) return log.error("Could not read gain", error);

    // 9. Read exposure
    log.printf("9. Read exposure");
    if( error = BoomInspectionCamera.getExposure(iVal) ) return log.error("Could not read exposure", error);


    return log.success();
}
