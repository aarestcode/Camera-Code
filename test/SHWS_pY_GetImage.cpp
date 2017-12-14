/***************************************************************************//**
 * @file	SHWS_pY_GetImage.cpp
 * @brief	Test file to get an image
 *
 * @author	Thibaud Talon
 * @date	25/09/2017
 *
 *******************************************************************************/

#include "UserInterface.hpp"
#include "SHWSCamera.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("SHWS_pY_GetImage");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc > 1) log.printf("WARNING: Extra inputs discarded");

    SHWSCamera_Error error;

    // 2. Connect camera
    log.printf("2. Connect camera");
    SHWSCamera shws(SHWSCAMERA_pY_APERTURE);
    if( shws.status != SHWSCAMERA_ON ) return log.error("Error connecting to camera", shws.status);

    cv::Mat img;

    // 3. Get image
    log.printf("3. Get image");
    if( error = shws.getImage(img) ) return log.error("Could not get image", error);

    return log.success();
}
