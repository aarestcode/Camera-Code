/***************************************************************************//**
 * @file	XBee_GetTelemetry.cpp
 * @brief	Test file to read all telemetry
 *
 * @author	Thibaud Talon
 * @date	26/09/2017
 *
 *******************************************************************************/

#include "UserInterface.hpp"
#include "XBee.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("XBee_GetTelemetry.cpp");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc > 1) log.printf("WARNING: Extra inputs discarded");

    XBee_Error error;

    // 2. Connect XBee
    log.printf("2. Connect XBee");
    XBee xbee(9600);
    if( xbee.status != XBEE_ON ) return log.error("Error connecting to XBee", xbee.status);

    // 3. Get telemetry
    log.printf("3. Get telemetry");
    XBee_Telemetry telemetry;
    if( error = xbee.getTelemetry(telemetry) ) return log.error("Could not get telemetry", error);

    return log.success();
}
