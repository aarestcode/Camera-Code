/***************************************************************************//**
 * @file	TakeBICVideo.cpp
 * @brief	Take video from Boom Inspection Camera and save it
 *
 * @author	Thibaud Talon
 * @date	25/09/2017
 *
 * @param [in] filename
 *	Name of the file (ends with .avi)
 * @param [in] fps
 *	Frames per second of the video
 * @param [in] duration_ms
 *	Duration of the video in seconds
 *******************************************************************************/

#include "UserInterface.hpp"
#include "ImagingCamera.hpp"
#include "AAReST.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("TakeBICVideo");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc < 4) return log.error("No filename, framerate (fps), duration (s) specified",-1);
    else if(argc > 4) log.printf("WARNING: Extra inputs discarded");

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

    // 4. Create video file
    log.printf("4. Create video file");
    int width, height;
    if( error1 = BoomInspectionCamera.getWidth(width) ) return log.error("Could not read width", error1);
    if( error1 = BoomInspectionCamera.getHeight(height) ) return log.error("Could not read height", error1);
    cv::VideoWriter video;
    if (error2 = UserInterface::createVideo(video, argv[1], atof(argv[2]), width, height)) return log.error("Cannot create video", error2);

    // 5. Get video
    log.printf("5. Get video");
    if( error1 = BoomInspectionCamera.getVideo(video, atof(argv[2]), atof(argv[3])) ) return log.error("Could not get video", error1);

    return log.success();
}
