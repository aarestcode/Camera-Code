/***************************************************************************//**
 * @file	SHWS_pY_PacketDelaySetup.cpp
 * @brief	Test file to read and set the packet delay
 *
 * @author	Thibaud Talon
 * @date	25/09/2017
 *
 * @param [in] Ntics
 *	Number of clock tics between each packet
 *******************************************************************************/

#include "UserInterface.hpp"
#include "SHWSCamera.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("SHWS_pY_PacketDelaySetup");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc < 2) return log.error("No exposure (us) specified",-1);
    else if(argc > 2) log.printf("WARNING: Extra inputs discarded");

    SHWSCamera_Error error;

    // 2. Connect camera
    log.printf("2. Connect camera");
    SHWSCamera shws(SHWSCAMERA_pY_APERTURE);
    if( shws.status != SHWSCAMERA_ON ) return log.error("Error connecting to camera", shws.status);

    int delay;

    // 3. Read packet delay
    log.printf("3. Read packet delay");
    if( error = shws.getPacketDelay(delay) ) return log.error("Could not read packet delay", error);

    // 4. Set packet delay
    log.printf("4. Set packet delay");
    if( error = shws.setPacketDelay(atoi(argv[1])) ) return log.error("Could not set packet delay", error);

    // 5. Read packet delay
    log.printf("5. Read packet delay");
    if( error = shws.getPacketDelay(delay) ) return log.error("Could not read packet delay", error);

    return log.success();
}
