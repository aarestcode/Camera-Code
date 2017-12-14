/***************************************************************************//**
 * @file	SHWS_pY_ReadParameters.cpp
 * @brief	Test file to read all parameters
 *
 * @author	Thibaud Talon
 * @date	21/09/2017
 *
 *******************************************************************************/

#include "UserInterface.hpp"
#include "SHWSCamera.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("SHWS_pY_ReadParameters");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc > 1) log.printf("WARNING: Extra inputs discarded");

    SHWSCamera_Error error;

    // 2. Connect camera
    log.printf("2. Connect camera");
    SHWSCamera shws(SHWSCAMERA_pY_APERTURE);
    if( shws.status != SHWSCAMERA_ON ) return log.error("Error connecting to camera", shws.status);

    int iVal;
    float fVal;

    // 3. Read timeout
    log.printf("3. Read timeout");
    if( error = shws.getTimeout(iVal) ) return log.error("Could not read timeout", error);

    // 4. Read retry number
    log.printf("4. Read retry number");
    if( error = shws.getRetryNumber(iVal) ) return log.error("Could not read retry number", error);

    // 5. Read width
    log.printf("5. Read width");
    if( error = shws.getWidth(iVal) ) return log.error("Could not read width", error);

    // 6. Read height
    log.printf("6. Read height");
    if( error = shws.getHeight(iVal) ) return log.error("Could not read height", error);

    // 7. Read horizontal offset
    log.printf("7. Read horizontal offset");
    if( error = shws.getOffsetX(iVal) ) return log.error("Could not read horizontal offset", error);

    // 8. Read vertical offset
    log.printf("8. Read vertical offset");
    if( error = shws.getOffsetY(iVal) ) return log.error("Could not read vertical offset", error);

    // 9. Read gain
    log.printf("9. Read gain");
    if( error = shws.getGain(fVal) ) return log.error("Could not read gain", error);

    // 10. Read exposure
    log.printf("10. Read exposure");
    if( error = shws.getExposure(iVal) ) return log.error("Could not read exposure", error);

    // 11. Read packet size
    log.printf("11. Read packet size");
    if( error = shws.getPacketSize(iVal) ) return log.error("Could not read packet size", error);

    // 12. Read packet delay
    log.printf("12. Read packet delay");
    if( error = shws.getPacketDelay(iVal) ) return log.error("Could not read packet delay", error);


    return log.success();
}
