/***************************************************************************//**
 * @file	XBee_GetNodeTelemetry.cpp
 * @brief	Test file to read all telemetry
 *
 * @author	Thibaud Talon
 * @date	26/09/2017
 *
 *******************************************************************************/

#include "UserInterface.hpp"
#include "XBee.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("XBee_GetNodeTelemetry.cpp");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc < 2) return log.error("No MAC address",-1);
    else if(argc > 2) log.printf("WARNING: Extra inputs discarded");

    XBee_Error error;

    // 2. Connect XBee
    log.printf("2. Connect XBee");
    XBee xbee(9600);
    if( xbee.status != XBEE_ON ) return log.error("Error connecting to camera", xbee.status);

    // 3. Connect Node
    log.printf("3. Connect Node");
    XBeeNode handle;
    if( error = xbee.connectNode(handle, atol(argv[1])) ) return log.error("Error connecting to node", xbee.status);

    // 4. Get telemetry
    log.printf("4. Get telemetry");
    XBeeNode_Telemetry telemetry;
    if( error = xbee.getNodeTelemetry(handle, telemetry) ) return log.error("Could not get telemetry", error);

    return log.success();
}
