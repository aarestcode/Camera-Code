/***************************************************************************//**
 * @file	TakeBICImage.cpp
 * @brief	Take image from Boom Inspection Camera and save it
 *
 * @author	Thibaud Talon
 * @date	25/09/2017
 *
 * @param [in] filename
 *	Name of the file (ends with .png or .jpg)
 *******************************************************************************/

#include "UserInterface.hpp"
#include "ImagingCamera.hpp"
#include "AAReST.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("TakeBICImage");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc < 2) return log.error("No filename specified",-1);
    else if(argc > 2) log.printf("WARNING: Extra inputs discarded");

    ImagingCamera_Error error1;
    UserInterface::UserInterface_Error error2;
    cv::Mat img;

    // 2. Connect camera
    log.printf("2. Connect camera");
    ImagingCamera BoomInspectionCamera(IMAGINGCAMERA_BOOM_INSPECTION_CAMERA);
    if( BoomInspectionCamera.status != IMAGINGCAMERA_ON ) return log.error("Error connecting to camera", BoomInspectionCamera.status);

    // 3. Set ROI
    log.printf("3. Set ROI");
    if( error1 = BoomInspectionCamera.setROI(BIC_OFFSETX, BIC_OFFSETY, BIC_WIDTH, BIC_HEIGHT) ) return log.error("Could not set ROI", error1);

    // 4. Get image
    log.printf("4. Get image");
    if( error1 = BoomInspectionCamera.getImage(img) ) return log.error("Could not get image", error1);
    log.printf("width = %i", img.cols);
    log.printf("height = %i", img.rows);

    // 5. Save image
    log.printf("5. Save image");
    if( error2 = UserInterface::saveImage(img, argv[1]) ) return log.error("Error saving image", error2);

    return log.success();
}
