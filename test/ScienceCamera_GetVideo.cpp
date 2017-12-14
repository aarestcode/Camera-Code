/***************************************************************************//**
 * @file	ScienceCamera_GetVideo.cpp
 * @brief	Test file to get a video
 *
 * @author	Thibaud Talon
 * @date	21/09/2017
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

int main(int argc, char* argv[]){
    UserInterface::Log log("Algorithm");

    // 1. Parsing data
    log.printf("1. Parsing inputs");
    if(argc < 4) return log.error("No filename, framerate (fps), duration (s) specified",-1);
    else if(argc > 4) log.printf("WARNING: Extra inputs discarded");

    ImagingCamera_Error error;
    UserInterface::UserInterface_Error error2;

    // 2. Connect camera
    log.printf("2. Connect camera");
    ImagingCamera ScienceCamera(IMAGINGCAMERA_SCIENCE_CAMERA);
    if( ScienceCamera.status != IMAGINGCAMERA_ON ) return log.error("Error connecting to camera", ScienceCamera.status);

    // 3. Create video file
    log.printf("3. Create video file");
    int width, height;
    if( error = ScienceCamera.getWidth(width) ) return log.error("Could not read width", error);
    if( error = ScienceCamera.getHeight(height) ) return log.error("Could not read height", error);
    cv::VideoWriter video;
    if (error2 = UserInterface::createVideo(video, argv[1], atof(argv[2]), width, height)) return log.error("Cannot create video", error2);

    // 4. Get video
    log.printf("4. Get video");
    if( error = ScienceCamera.getVideo(video, atof(argv[2]), atof(argv[3])) ) return log.error("Could not get video", error);

    return log.success();
}
