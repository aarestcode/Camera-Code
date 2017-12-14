/***************************************************************************//**
 * @file	ScienceCamera_GetImage.cpp
 * @brief	Test file to get an image
 *
 * @author	Thibaud Talon
 * @date	21/09/2017
 *
 *******************************************************************************/

#include "UserInterface.hpp"
#include "ImagingCamera.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("Algorithm");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc > 1) log.printf("WARNING: Extra inputs discarded");

    ImagingCamera_Error error;

    // 2. Connect camera
    log.printf("2. Connect camera");
    ImagingCamera ScienceCamera(IMAGINGCAMERA_SCIENCE_CAMERA);
    if( ScienceCamera.status != IMAGINGCAMERA_ON ) return log.error("Error connecting to camera", ScienceCamera.status);

    cv::Mat img;

    // 3. Get image
    log.printf("3. Get image");
    if( error = ScienceCamera.getImage(img) ) return log.error("Could not get image", error);

    return log.success();
}
