/***************************************************************************//**
 * @file	BIC_ROISetup.cpp
 * @brief	Test file to read and set the region of interest
 *
 * @author	Thibaud Talon
 * @date	25/09/2017
 *
 * @param [in] offsetX_px
 *	Horizontal offset in pixels (offsetX + width < 2592)
 * @param [in] offsetY_px
 *	Vertical offset in pixels (offsetY + height < 1944)
 * @param [in] width_px
 *	Width of the image in pixels (offsetX + width < 2592)
 * @param [in] height_px
 *	Height of the image in pixels (offsetY + height < 1944)
 *******************************************************************************/

#include "UserInterface.hpp"
#include "ImagingCamera.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("BIC_ROISetup");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc < 5) return log.error("No offsetX (px), offsetY (px), width(px) and height (px) specified",-1);
    else if(argc > 5) log.printf("WARNING: Extra inputs discarded");

    ImagingCamera_Error error;

    // 2. Connect camera
    log.printf("2. Connect camera");
    ImagingCamera BoomInspectionCamera(IMAGINGCAMERA_BOOM_INSPECTION_CAMERA);
    if( BoomInspectionCamera.status != IMAGINGCAMERA_ON ) return log.error("Error connecting to camera", BoomInspectionCamera.status);

    int offsetX, offsetY, width, height;

    // 3. Read ROI
    log.printf("3. Read ROI");
    if( error = BoomInspectionCamera.getROI(offsetX, offsetY, width, height) ) return log.error("Could not read ROI", error);

    // 4. Set ROI
    log.printf("4. Set ROI");
    if( error = BoomInspectionCamera.setROI(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]))) return log.error("Could not set ROI", error);

    // 5. Read ROI
    log.printf("5. Read ROI");
    if( error = BoomInspectionCamera.getROI(offsetX, offsetY, width, height) ) return log.error("Could not read ROI", error);

    return log.success();
}
