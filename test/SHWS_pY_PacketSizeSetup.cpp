/***************************************************************************//**
 * @file	SHWS_pY_PacketSizeSetup.cpp
 * @brief	Test file to read and set the packet size
 *
 * @author	Thibaud Talon
 * @date	25/09/2017
 *
 * @param [in] Nbytes
 *	Size of packets in bytes
 *******************************************************************************/

#include "UserInterface.hpp"
#include "SHWSCamera.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("SHWS_pY_PacketSizeSetup");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc < 2) return log.error("No exposure (us) specified",-1);
    else if(argc > 2) log.printf("WARNING: Extra inputs discarded");

    SHWSCamera_Error error;

    // 2. Connect camera
    log.printf("2. Connect camera");
    SHWSCamera shws(SHWSCAMERA_pY_APERTURE);
    if( shws.status != SHWSCAMERA_ON ) return log.error("Error connecting to camera", shws.status);

    int size;

    // 3. Read packet size
    log.printf("3. Read packet size");
    if( error = shws.getPacketSize(size) ) return log.error("Could not read packet size", error);

    // 4. Set packet size
    log.printf("4. Set packet size");
    if( error = shws.setPacketSize(atoi(argv[1])) ) return log.error("Could not set packet size", error);

    // 5. Read packet size
    log.printf("5. Read packet size");
    if( error = shws.getPacketSize(size) ) return log.error("Could not read packet size", error);

    return log.success();
}
