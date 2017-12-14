/***************************************************************************//**
 * @file	ImagingCamera.cpp
 * @brief	Source file to operate the XIMEAS cameras
 *
 * This file contains all the implementations for the functions defined in:
 * api/include/ImagingCamera.hpp
 *
 * @author	Thibaud Talon
 * @date	21/09/2017
 *******************************************************************************/

#include <m3api/xiApi.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp> // video structure
#include <sys/time.h> // time structure for video
#include <math.h> // ceil used for video
#include "ImagingCamera.hpp"
#include "UserInterface.hpp"

#define IMAGINGCAMERA_MAX_WIDTH 2592
#define IMAGINGCAMERA_MAX_HEIGHT 1944

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Connect the camera
 *
 * @param [in] cameraID
 *	index of the camera (SCIENCE_CAMERA or BOOM_INSPECTION_CAMERA)
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::connect(ImagingCamera_Index cameraID){
    UserInterface::Log log("ImagingCamera::connect");

    try{
        handle = NULL;
        _timeout = 0;
        status = IMAGINGCAMERA_OFF;

        // 1. Get number of camera devices
        log.printf("1. Get number of camera devices");
        DWORD dwNumberOfDevices = 0;
        error = xiGetNumberDevices(&dwNumberOfDevices);
        if( error != XI_OK ) return (ImagingCamera_Error) log.error("Cannot retrieve number of connected cameras", ERR_IMAGINGCAMERA_CANNOT_DETECT);
        log.printf("Number of devices = %i", dwNumberOfDevices);
        if( dwNumberOfDevices == 0 ) return (ImagingCamera_Error) log.error("No camera connected", ERR_IMAGINGCAMERA_NO_DETECT);

        // 2. Open device
        log.printf("2. Open device #%i", cameraID);
        error = xiOpenDevice(cameraID, &handle);
        if( error != XI_OK ) return (ImagingCamera_Error) log.error("No camera connected", ERR_IMAGINGCAMERA_CANNOT_OPEN);
        status = IMAGINGCAMERA_ON;
        index = cameraID;

        // 3. Set exposure to 10ms
        log.printf("3. Set exposure to 10ms");
        error = xiSetParamInt( handle,  XI_PRM_EXPOSURE, 10000);
        if( error != XI_OK ) {status = IMAGINGCAMERA_ERROR; log.error("Cannot set exposure to 10 ms", ERR_IMAGINGCAMERA_SET_EXPOSURE);}

        // 4. Set capture _timeout to 5s
        log.printf("4. Set capture timeout to 5s");
        _timeout = 5000;

        return (ImagingCamera_Error) log.success();
    }
    catch( const std::exception& e ){
        status = IMAGINGCAMERA_ERROR;
        return (ImagingCamera_Error) log.error(e.what(),ERR_IMAGINGCAMERA_CONNECT_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Disconnect the camera
 *
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::disconnect(void){
    UserInterface::Log log("ImagingCamera::disconnect");

    try{
        log.printf("Closing the connection");
        if (handle != NULL) xiCloseDevice(handle);
        handle == NULL;
        status = IMAGINGCAMERA_OFF;

        return (ImagingCamera_Error) log.success();
    }
    catch( const std::exception& e ){
        status = IMAGINGCAMERA_ERROR;
        return (ImagingCamera_Error) log.error(e.what(),ERR_IMAGINGCAMERA_DISCONNECT_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Reset the connection to the camera
 *
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::reset(void){
    disconnect();
    return connect(index);
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Get an image from the camera
 *
 * @param [out] img
 *	OpenCV image corresponding to the frame retrieved from the camera
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::getImage(cv::Mat & img){
    UserInterface::Log log("ImagingCamera::getImage");

    try{
        // 1. Check inputs
        log.printf("1. Check inputs");
        img.release();
        if( handle == NULL ) {return (ImagingCamera_Error) log.error("No opened device", ERR_IMAGINGCAMERA_NO_DEVICE);}

        // 2. Start acquisition
        log.printf("2. Start acquisition");
        error = xiStartAcquisition(handle);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot start acquisition", ERR_IMAGINGCAMERA_START_ACQUISITION);}

        // 3. Take image
        log.printf("3. Take image");
        XI_IMG xi_image;
        memset(&xi_image, 0, sizeof(XI_IMG));
        xi_image.size = sizeof(XI_IMG);
        xi_image.bp = NULL;
        xi_image.bp_size = 0;
        error = xiGetImage( handle, _timeout, &xi_image);
        if (error != XI_OK) {xiStopAcquisition(handle); status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot take image", ERR_IMAGINGCAMERA_GET_IMAGE);}

        // 4. Convert image
        log.printf("4. Convert image");
        img = cv::Mat(xi_image.height, xi_image.width, CV_8UC1, xi_image.bp);

        xiStopAcquisition(handle);

        return (ImagingCamera_Error) log.success();

    }
    catch( const std::exception& e ){
        status = IMAGINGCAMERA_ERROR;
        return (ImagingCamera_Error) log.error(e.what(),ERR_IMAGINGCAMERA_GETIMAGE_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Get an video from the camera
 *
 * @param [out] video
 *	OpenCV video to store the frames
 * @param [in] fps
 *	Frames per seconds of the video
 * @param [in] duration_s
 *	Duration of the video in seconds
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::getVideo(cv::VideoWriter & video, float fps, float duration_s){
    UserInterface::Log log("ImagingCamera::getVideo");

    try{
        // 1. Check inputs
        log.printf("1. Check inputs");
        if( handle == NULL ) {return (ImagingCamera_Error) log.error("No opened device", ERR_IMAGINGCAMERA_NO_DEVICE);}
        if (!video.isOpened()) {return (ImagingCamera_Error) log.error("Video not opened", ERR_IMAGINGCAMERA_NO_VIDEO);}

        // 2. Enable trigger
        log.printf("2. Enable trigger");
        error = xiSetParamInt(handle, XI_PRM_TRG_SOURCE, XI_TRG_SOFTWARE);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot enable trigger", ERR_IMAGINGCAMERA_ENABLE_TRIGGER);}

        // 3. Set shutter type to rolling
        log.printf("3. Set shutter type to rolling");
        error = xiSetParamInt( handle,  XI_PRM_SHUTTER_TYPE, XI_SHUTTER_ROLLING);
        if( error != XI_OK ) {status = IMAGINGCAMERA_ERROR; log.error("Cannot set shutter type to rolling mode", ERR_IMAGINGCAMERA_SET_SHUTTER);}

        // 4. Initialize timers, buffer images and parameters
        log.printf("4. Initialize timers, buffer images and parameters");
        long start, now, delay; // To save the time in millis
        struct timeval tv; // To save the time
        XI_IMG xi_image;
        memset(&xi_image, 0, sizeof(XI_IMG));
        xi_image.size = sizeof(XI_IMG);
        xi_image.bp = NULL;
        xi_image.bp_size = 0;
        long Nframes = ceil(fps*duration_s);

        // 5. Start acquisition
        log.printf("5. Start acquisition");
        error = xiStartAcquisition(handle);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot start acquisition", ERR_IMAGINGCAMERA_START_ACQUISITION);}

        // 6. Start video
        log.printf("6. Start video");
        gettimeofday(&tv, NULL);
        start = (long)tv.tv_sec*1000 + (long)(tv.tv_usec/1000);
        for (int frame = 0; frame < Nframes; frame++){
            // Wait
            delay = (long)(1000*(frame+1)/fps);
            gettimeofday(&tv, NULL);
            now = (long)tv.tv_sec*1000 + (long)(tv.tv_usec/1000);
            if ( ( now - start ) > delay ) {xiStopAcquisition(handle); return (ImagingCamera_Error) log.error("Framerate too high", ERR_IMAGINGCAMERA_VIDEO_FRAMERATE);}
            while( ( now - start ) < delay ) {gettimeofday(&tv, NULL); now = (long)tv.tv_sec*1000 + (long)(tv.tv_usec/1000);}

            // Trigger next image
            error = xiSetParamInt(handle, XI_PRM_TRG_SOFTWARE, 1);
            if (error != XI_OK) {xiStopAcquisition(handle); status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot trigger next image", ERR_IMAGINGCAMERA_TRIGGER);}

            // Get image
            error = xiGetImage( handle, _timeout, &xi_image);
            if (error != XI_OK) {xiStopAcquisition(handle); status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot take image", ERR_IMAGINGCAMERA_GET_IMAGE);}

            // Add to video
            video << cv::Mat(xi_image.height, xi_image.width, CV_8UC1, xi_image.bp);
            log.printf("Frame #%i added", frame+1);
        }

        xiStopAcquisition(handle);

        // 7. Disable trigger
        log.printf("7. Disable trigger");
        error = xiSetParamInt(handle, XI_PRM_TRG_SOURCE, XI_TRG_OFF);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot disable trigger", ERR_IMAGINGCAMERA_DISABLE_TRIGGER);}

        return (ImagingCamera_Error) log.success();

    }
    catch( const std::exception& e ){
        status = IMAGINGCAMERA_ERROR;
        return (ImagingCamera_Error) log.error(e.what(),ERR_IMAGINGCAMERA_GETVIDEO_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Set capture timeout
 *
 * @param [in] timeout_ms
 *	Capture timeout in ms
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::setTimeout(int timeout_ms){
    UserInterface::Log log("ImagingCamera::setTimeout");

    log.printf("Change timeout to %i ms", timeout_ms);
    _timeout = timeout_ms;

    return (ImagingCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Set region of interest
 *
 * @param [in] offsetX_px
 *	Horizontal offset in pixels (offsetX + width < 2592)
 * @param [in] offsetY_px
 *	Vertical offset in pixels (offsetY + height < 1944)
 * @param [in] width_px
 *	Width of the image in pixels (offsetX + width < 2592)
 * @param [in] height_px
 *	Height of the image in pixels (offsetY + height < 1944)
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::setROI(int offsetX_px, int offsetY_px, int width_px, int height_px){
    UserInterface::Log log("ImagingCamera::setROI");

    try{
        if( handle == NULL ) {return (ImagingCamera_Error) log.error("No opened device", ERR_IMAGINGCAMERA_NO_DEVICE);}

        // 1. Check inputs
        log.printf("1. Check inputs");
        if ( offsetX_px + width_px > IMAGINGCAMERA_MAX_WIDTH ) {width_px = IMAGINGCAMERA_MAX_WIDTH - offsetX_px; log.error("ROI right limit out of bounds", ERR_IMAGINGCAMERA_ROI_WOOB);}
        if ( offsetY_px + height_px > IMAGINGCAMERA_MAX_HEIGHT ) {height_px = IMAGINGCAMERA_MAX_HEIGHT - offsetY_px; log.error("ROI bottom limit out of bounds", ERR_IMAGINGCAMERA_ROI_HOOB);}

        // 2. Get current width and height
        log.printf("2. Get current width and height");
        int current_width, current_height;
        error = xiGetParamInt( handle, XI_PRM_WIDTH, &current_width);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get width", ERR_IMAGINGCAMERA_GET_WIDTH);}
        error = xiGetParamInt( handle, XI_PRM_HEIGHT, &current_height);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get height", ERR_IMAGINGCAMERA_GET_HEIGHT);}

        if ( offsetX_px + current_width > IMAGINGCAMERA_MAX_WIDTH){
            // 3. Change image width to %i px
            error = xiSetParamInt( handle, XI_PRM_WIDTH, width_px);
            if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot change width", ERR_IMAGINGCAMERA_SET_WIDTH);}
            error = xiGetParamInt( handle, XI_PRM_WIDTH, &width_px);
            if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get width", ERR_IMAGINGCAMERA_GET_WIDTH);}
            log.printf("3. ROI width set to %i px", width_px);

            // 4. Change horizontal offset to %i px
            error = xiSetParamInt(handle, XI_PRM_OFFSET_X , offsetX_px);
            if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot change horizontal offset", ERR_IMAGINGCAMERA_SET_OFFSETX);}
            error = xiGetParamInt( handle, XI_PRM_OFFSET_X , &offsetX_px);
            if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get horizontal offset", ERR_IMAGINGCAMERA_GET_OFFSETX);}
            log.printf("4. ROI horizontal offset set to %i px", offsetX_px);
        }
        else{
            // 3. Change horizontal offset to %i px
            error = xiSetParamInt(handle, XI_PRM_OFFSET_X , offsetX_px);
            if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot change horizontal offset", ERR_IMAGINGCAMERA_SET_OFFSETX);}
            error = xiGetParamInt( handle, XI_PRM_OFFSET_X , &offsetX_px);
            if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get horizontal offset", ERR_IMAGINGCAMERA_GET_OFFSETX);}
            log.printf("3. ROI horizontal offset set to %i px", offsetX_px);

            // 4. Change image width to %i px
            error = xiSetParamInt( handle, XI_PRM_WIDTH, width_px);
            if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot change width", ERR_IMAGINGCAMERA_SET_WIDTH);}
            error = xiGetParamInt( handle, XI_PRM_WIDTH, &width_px);
            if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get width", ERR_IMAGINGCAMERA_GET_WIDTH);}
            log.printf("4. ROI width set to %i px", width_px);
        }

        if ( offsetY_px + current_height > IMAGINGCAMERA_MAX_HEIGHT){
            // 5. Change image height to %i px
            error = xiSetParamInt( handle, XI_PRM_HEIGHT , height_px);
            if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot change height", ERR_IMAGINGCAMERA_SET_HEIGHT);}
            error = xiGetParamInt( handle, XI_PRM_HEIGHT, &height_px);
            if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get height", ERR_IMAGINGCAMERA_GET_HEIGHT);}
            log.printf("5. ROI height set to %i px", height_px);

            // 6. Change vertical offset to %i px
            error = xiSetParamInt(handle, XI_PRM_OFFSET_Y , offsetY_px);
            if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot change vertical offset", ERR_IMAGINGCAMERA_SET_OFFSETY);}
            error = xiGetParamInt( handle, XI_PRM_OFFSET_Y , &offsetY_px);
            if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get vertical offset", ERR_IMAGINGCAMERA_GET_OFFSETY);}
            log.printf("6. ROI vertical offset set to %i px", offsetY_px);
        }
        else{
            // 5. Change vertical offset to %i px
            error = xiSetParamInt(handle, XI_PRM_OFFSET_Y , offsetY_px);
            if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot change vertical offset", ERR_IMAGINGCAMERA_SET_OFFSETY);}
            error = xiGetParamInt( handle, XI_PRM_OFFSET_Y , &offsetY_px);
            if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get vertical offset", ERR_IMAGINGCAMERA_GET_OFFSETY);}
            log.printf("5. ROI vertical offset set to %i px", offsetY_px);


            // 6. Change image height to %i px
            error = xiSetParamInt( handle, XI_PRM_HEIGHT , height_px);
            if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot change height", ERR_IMAGINGCAMERA_SET_HEIGHT);}
            error = xiGetParamInt( handle, XI_PRM_HEIGHT, &height_px);
            if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get height", ERR_IMAGINGCAMERA_GET_HEIGHT);}
            log.printf("6. ROI height set to %i px", height_px);
        }

        return (ImagingCamera_Error) log.success();

    }
    catch( const std::exception& e ){
        status = IMAGINGCAMERA_ERROR;
        return (ImagingCamera_Error) log.error(e.what(),ERR_IMAGINGCAMERA_SET_ROI_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Set gain
 *
 * @param [in] gain_dB
 *	Gain in dB
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::setGain(float gain_dB){
    UserInterface::Log log("ImagingCamera::setGain");

    try{
        if( handle == NULL ) {return (ImagingCamera_Error) log.error("No opened device", ERR_IMAGINGCAMERA_NO_DEVICE);}

        error = xiSetParamFloat( handle, XI_PRM_GAIN , gain_dB);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot change gain", ERR_IMAGINGCAMERA_SET_GAIN);}
        error = xiGetParamFloat( handle, XI_PRM_GAIN, &gain_dB);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get gain", ERR_IMAGINGCAMERA_GET_GAIN);}
        log.printf("Gain set to %f dB", gain_dB);

        return (ImagingCamera_Error) log.success();

    }
    catch( const std::exception& e ){
        status = IMAGINGCAMERA_ERROR;
        return (ImagingCamera_Error) log.error(e.what(),ERR_IMAGINGCAMERA_SET_GAIN_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Set exposure
 *
 * @param [in] exposure_us
 *	Exposure in us
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::setExposure(int exposure_us){
    UserInterface::Log log("ImagingCamera::setExposure");

    try{
        if( handle == NULL ) {return (ImagingCamera_Error) log.error("No opened device", ERR_IMAGINGCAMERA_NO_DEVICE);}

        error = xiSetParamInt(handle, XI_PRM_EXPOSURE , exposure_us);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot change exposure", ERR_IMAGINGCAMERA_SET_EXPOSURE);}
        error = xiGetParamInt( handle, XI_PRM_EXPOSURE, &exposure_us);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get exposure", ERR_IMAGINGCAMERA_GET_EXPOSURE);}
        log.printf("Exposure set to %i us", exposure_us);

        return (ImagingCamera_Error) log.success();

    }
    catch( const std::exception& e ){
        status = IMAGINGCAMERA_ERROR;
        return (ImagingCamera_Error) log.error(e.what(),ERR_IMAGINGCAMERA_SET_EXPOSURE_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Get timeout
 *
 * @param [out] timeout_ms
 *	Capture timeout in ms
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::getTimeout(int & timeout_ms){
    UserInterface::Log log("ImagingCamera::getTimeout");

    timeout_ms = _timeout;
    log.printf("Timeout = %i ms", timeout_ms);

    return (ImagingCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Get region of interest
 *
 * @param [out] offsetX_px
 *	Horizontal offset in pixels
 * @param [out] offsetY_px
 *	Vertical offset in pixels
 * @param [out] width_px
 *	Width of the image in pixels
 * @param [out] height_px
 *	Height of the image in pixels
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::getROI(int & offsetX_px, int & offsetY_px, int & width_px, int & height_px){
    UserInterface::Log log("ImagingCamera::getROI");

    try{
        if( handle == NULL ) {return (ImagingCamera_Error) log.error("No opened device", ERR_IMAGINGCAMERA_NO_DEVICE);}

        // 1. Horizontal offset
        error = xiGetParamInt( handle, XI_PRM_OFFSET_X , &offsetX_px);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get horizontal offset", ERR_IMAGINGCAMERA_GET_OFFSETX);}
        log.printf("1. ROI horizontal offset = %i px", offsetX_px);

        // 2. Vertical offset
        error = xiGetParamInt( handle, XI_PRM_OFFSET_Y , &offsetY_px);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get vertical offset", ERR_IMAGINGCAMERA_GET_OFFSETY);}
        log.printf("2. ROI vertical offset = %i px", offsetY_px);

        // 3. Image width
        error = xiGetParamInt( handle, XI_PRM_WIDTH, &width_px);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get width", ERR_IMAGINGCAMERA_GET_WIDTH);}
        log.printf("3. ROI width = %i px", width_px);

        // 4. Image height
        error = xiGetParamInt( handle, XI_PRM_HEIGHT, &height_px);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get height", ERR_IMAGINGCAMERA_GET_HEIGHT);}
        log.printf("4. RIO height = %i px", height_px);

        return (ImagingCamera_Error) log.success();

    }
    catch( const std::exception& e ){
        status = IMAGINGCAMERA_ERROR;
        return (ImagingCamera_Error) log.error(e.what(),ERR_IMAGINGCAMERA_GET_ROI_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Get horizontal offset of ROI
 *
 * @param [out] offsetX_px
 *	Horizontal offset in pixels
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::getOffsetX(int & offsetX_px){
    UserInterface::Log log("ImagingCamera::getOffsetX");

    try{
        if( handle == NULL ) {return (ImagingCamera_Error) log.error("No opened device", ERR_IMAGINGCAMERA_NO_DEVICE);}

        // Horizontal offset
        error = xiGetParamInt( handle, XI_PRM_OFFSET_X , &offsetX_px);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get horizontal offset", ERR_IMAGINGCAMERA_GET_OFFSETX);}
        log.printf("ROI horizontal offset = %i px", offsetX_px);

        return (ImagingCamera_Error) log.success();

    }
    catch( const std::exception& e ){
        status = IMAGINGCAMERA_ERROR;
        return (ImagingCamera_Error) log.error(e.what(),ERR_IMAGINGCAMERA_GET_OFFSETX_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Get vertical offset of ROI
 *
 * @param [out] offsetY_px
 *	Vertical offset in pixels
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::getOffsetY(int & offsetY_px){
    UserInterface::Log log("ImagingCamera::getOffsetY");

    try{
        if( handle == NULL ) {return (ImagingCamera_Error) log.error("No opened device", ERR_IMAGINGCAMERA_NO_DEVICE);}

        // Vertical offset
        error = xiGetParamInt( handle, XI_PRM_OFFSET_Y , &offsetY_px);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get vertical offset", ERR_IMAGINGCAMERA_GET_OFFSETY);}
        log.printf("ROI vertical offset = %i px", offsetY_px);

        return (ImagingCamera_Error) log.success();

    }
    catch( const std::exception& e ){
        return (ImagingCamera_Error) log.error(e.what(),ERR_IMAGINGCAMERA_GET_OFFSETY_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Get width of ROI
 *
 * @param [out] width_px
 *	Width of the image in pixels
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::getWidth(int & width_px){
    UserInterface::Log log("ImagingCamera::getWidth");

    try{
        if( handle == NULL ) {return (ImagingCamera_Error) log.error("No opened device", ERR_IMAGINGCAMERA_NO_DEVICE);}

        // ROI width
        error = xiGetParamInt( handle, XI_PRM_WIDTH, &width_px);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get width", ERR_IMAGINGCAMERA_GET_WIDTH);}
        log.printf("ROI width = %i px", width_px);

        return (ImagingCamera_Error) log.success();

    }
    catch( const std::exception& e ){
        status = IMAGINGCAMERA_ERROR;
        return (ImagingCamera_Error) log.error(e.what(),ERR_IMAGINGCAMERA_GET_WIDTH_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Get height of ROI
 *
 * @param [out] height_px
 *	Height of the image in pixels
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::getHeight(int & height_px){
    UserInterface::Log log("ImagingCamera::getHeight");

    try{
        if( handle == NULL ) {return (ImagingCamera_Error) log.error("No opened device", ERR_IMAGINGCAMERA_NO_DEVICE);}

        // ROI height
        error = xiGetParamInt( handle, XI_PRM_HEIGHT, &height_px);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get height", ERR_IMAGINGCAMERA_GET_HEIGHT);}
        log.printf("RIO height = %i px", height_px);

        return (ImagingCamera_Error) log.success();

    }
    catch( const std::exception& e ){
        status = IMAGINGCAMERA_ERROR;
        return (ImagingCamera_Error) log.error(e.what(),ERR_IMAGINGCAMERA_GET_HEIGHT_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Get gain
 *
 * @param [out] gain_dB
 *	Gain in dB
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::getGain(float & gain_dB){
    UserInterface::Log log("ImagingCamera::getGain");

    try{
        if( handle == NULL ) {return (ImagingCamera_Error) log.error("No opened device", ERR_IMAGINGCAMERA_NO_DEVICE);}

        error = xiGetParamFloat( handle, XI_PRM_GAIN, &gain_dB);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get gain", ERR_IMAGINGCAMERA_GET_GAIN);}
        log.printf("Gain = %f dB", gain_dB);

        return (ImagingCamera_Error) log.success();

    }
    catch( const std::exception& e ){
        status = IMAGINGCAMERA_ERROR;
        return (ImagingCamera_Error) log.error(e.what(),ERR_IMAGINGCAMERA_GET_GAIN_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Get exposure
 *
 * @param [out] exposure_us
 *	Exposure in us
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::getExposure(int & exposure_us){
    UserInterface::Log log("ImagingCamera::getExposure");

    try{
        if( handle == NULL ) {return (ImagingCamera_Error) log.error("No opened device", ERR_IMAGINGCAMERA_NO_DEVICE);}

        error = xiGetParamInt( handle, XI_PRM_EXPOSURE, &exposure_us);
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; return (ImagingCamera_Error) log.error("Cannot get exposure", ERR_IMAGINGCAMERA_GET_EXPOSURE);}
        log.printf("Exposure = %i us", exposure_us);

        return (ImagingCamera_Error) log.success();

    }
    catch( const std::exception& e ){
        status = IMAGINGCAMERA_ERROR;
        return (ImagingCamera_Error) log.error(e.what(),ERR_IMAGINGCAMERA_GET_EXPOSURE_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Get all the telemetry for debugging
 *
 * @param [out] telemetry
 *	Telemetry structure
 ******************************************************************************/
ImagingCamera_Error ImagingCamera::getTelemetry(ImagingCamera_Telemetry & telemetry){
    UserInterface::Log log("ImagingCamera::getTelemetry");

    try{
        if( handle == NULL ) {return (ImagingCamera_Error) log.error("No opened device", ERR_IMAGINGCAMERA_NO_DEVICE);}

        error = xiGetParamString( handle, "device_name", telemetry.device_name, sizeof(telemetry.device_name));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get device_name", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("device_name = %s ", telemetry.device_name);

        error = xiGetParamString( handle, "device_inst_path", telemetry.device_inst_path, sizeof(telemetry.device_inst_path));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get device_inst_path", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("device_inst_path = %s ", telemetry.device_inst_path);

        error = xiGetParamString( handle, "device_loc_path", telemetry.device_loc_path, sizeof(telemetry.device_loc_path));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get device_loc_path", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("device_loc_path = %s ", telemetry.device_loc_path);

        error = xiGetParamString( handle, "device_type", telemetry.device_type, sizeof(telemetry.device_type));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get device_type", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("device_type = %s ", telemetry.device_type);

        error = xiGetParamInt( handle, "device_model_id", &(telemetry.device_model_id));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get device_model_id", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("device_model_id = %i ", telemetry.device_model_id);

        error = xiGetParamString( handle, "device_sn", telemetry.device_sn, sizeof(telemetry.device_sn));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get device_sn", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("device_sn = %s ", telemetry.device_sn);

        error = xiGetParamInt( handle, "debug_level", &(telemetry.debug_level));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get debug_level", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("debug_level = %i ", telemetry.debug_level);

        error = xiGetParamInt( handle, "auto_bandwidth_calculation", &(telemetry.auto_bandwidth_calculation));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get auto_bandwidth_calculation", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("auto_bandwidth_calculation = %i ", telemetry.auto_bandwidth_calculation);

        error = xiGetParamInt( handle, "new_process_chain_enable", &(telemetry.new_process_chain_enable));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get new_process_chain_enable", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("new_process_chain_enable = %i ", telemetry.new_process_chain_enable);

        error = xiGetParamInt( handle, "exposure", &(telemetry.exposure));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get exposure", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("exposure = %i ", telemetry.exposure);

        error = xiGetParamFloat( handle, "gain", &(telemetry.gain));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get gain", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("gain = %f ", telemetry.gain);

        error = xiGetParamInt( handle, "downsampling", &(telemetry.downsampling));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get downsampling", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("downsampling = %i ", telemetry.downsampling);

        error = xiGetParamInt( handle, "downsampling_type", &(telemetry.downsampling_type));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get downsampling_type", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("downsampling_type = %i ", telemetry.downsampling_type);

        error = xiGetParamInt( handle, "shutter_type", &(telemetry.shutter_type));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get shutter_type", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("shutter_type = %i ", telemetry.shutter_type);

        error = xiGetParamInt( handle, "imgdataformat", &(telemetry.imgdataformat));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get imgdataformat", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("imgdataformat = %i ", telemetry.imgdataformat);

        error = xiGetParamInt( handle, "imgdataformatrgb32alpha", &(telemetry.imgdataformatrgb32alpha));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get imgdataformatrgb32alpha", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("imgdataformatrgb32alpha = %i ", telemetry.imgdataformatrgb32alpha);

        error = xiGetParamInt( handle, "imgpayloadsize", &(telemetry.imgpayloadsize));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get imgpayloadsize", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("imgpayloadsize = %i ", telemetry.imgpayloadsize);

        error = xiGetParamInt( handle, "transport_pixel_format", &(telemetry.transport_pixel_format));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get transport_pixel_format", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("transport_pixel_format = %i ", telemetry.transport_pixel_format);

        error = xiGetParamFloat( handle, "framerate", &(telemetry.framerate));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get framerate", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("framerate = %f ", telemetry.framerate);

        error = xiGetParamInt( handle, "buffer_policy", &(telemetry.buffer_policy));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get buffer_policy", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("buffer_policy = %i ", telemetry.buffer_policy);

        error = xiGetParamInt( handle, "counter_selector", &(telemetry.counter_selector));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get counter_selector", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("counter_selector = %i ", telemetry.counter_selector);

        error = xiGetParamInt( handle, "counter_value", &(telemetry.counter_value));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get counter_value", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("counter_value = %i ", telemetry.counter_value);

        error = xiGetParamInt( handle, "offsetX", &(telemetry.offsetX));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get offsetX", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("offsetX = %i ", telemetry.offsetX);

        error = xiGetParamInt( handle, "offsetY", &(telemetry.offsetY));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get offsetY", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("offsetY = %i ", telemetry.offsetY);

        error = xiGetParamInt( handle, "width", &(telemetry.width));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get width", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("width = %i ", telemetry.width);

        error = xiGetParamInt( handle, "height", &(telemetry.height));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get height", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("height = %i ", telemetry.height);

        error = xiGetParamInt( handle, "trigger_source", &(telemetry.trigger_source));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get trigger_source", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("trigger_source = %i ", telemetry.trigger_source);

        error = xiGetParamInt( handle, "trigger_software", &(telemetry.trigger_software));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get trigger_software", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("trigger_software = %i ", telemetry.trigger_software);

        error = xiGetParamInt( handle, "trigger_delay", &(telemetry.trigger_delay));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get trigger_delay", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("trigger_delay = %i ", telemetry.trigger_delay);

        error = xiGetParamInt( handle, "available_bandwidth", &(telemetry.available_bandwidth));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get available_bandwidth", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("available_bandwidth = %i ", telemetry.available_bandwidth);

        error = xiGetParamInt( handle, "limit_bandwidth", &(telemetry.limit_bandwidth));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get limit_bandwidth", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("limit_bandwidth = %i ", telemetry.limit_bandwidth);

        error = xiGetParamFloat( handle, "sensor_clock_freq_hz", &(telemetry.sensor_clock_freq_hz));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get sensor_clock_freq_hz", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("sensor_clock_freq_hz = %f ", telemetry.sensor_clock_freq_hz);

        error = xiGetParamInt( handle, "sensor_clock_freq_index", &(telemetry.sensor_clock_freq_index));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get sensor_clock_freq_index", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("sensor_clock_freq_index = %i ", telemetry.sensor_clock_freq_index);

        error = xiGetParamInt( handle, "sensor_bit_depth", &(telemetry.sensor_bit_depth));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get sensor_bit_depth", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("sensor_bit_depth = %i ", telemetry.sensor_bit_depth);

        error = xiGetParamInt( handle, "output_bit_depth", &(telemetry.output_bit_depth));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get output_bit_depth", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("output_bit_depth = %i ", telemetry.output_bit_depth);

        error = xiGetParamInt( handle, "image_data_bit_depth", &(telemetry.image_data_bit_depth));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get image_data_bit_depth", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("image_data_bit_depth = %i ", telemetry.image_data_bit_depth);

        error = xiGetParamInt( handle, "output_bit_packing", &(telemetry.output_bit_packing));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get output_bit_packing", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("output_bit_packing = %i ", telemetry.output_bit_packing);

        error = xiGetParamInt( handle, "acq_timing_mode", &(telemetry.acq_timing_mode));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get acq_timing_mode", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("acq_timing_mode = %i ", telemetry.acq_timing_mode);

        error = xiGetParamInt( handle, "trigger_selector", &(telemetry.trigger_selector));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get trigger_selector", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("trigger_selector = %i ", telemetry.trigger_selector);

        error = xiGetParamFloat( handle, "wb_kr", &(telemetry.wb_kr));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get wb_kr", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("wb_kr = %f ", telemetry.wb_kr);

        error = xiGetParamFloat( handle, "wb_kg", &(telemetry.wb_kg));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get wb_kg", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("wb_kg = %f ", telemetry.wb_kg);

        error = xiGetParamFloat( handle, "wb_kb", &(telemetry.wb_kb));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get wb_kb", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("wb_kb = %f ", telemetry.wb_kb);

        error = xiGetParamInt( handle, "auto_wb", &(telemetry.auto_wb));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get auto_wb", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("auto_wb = %i ", telemetry.auto_wb);

        error = xiGetParamFloat( handle, "gammaY", &(telemetry.gammaY));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get gammaY", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("gammaY = %f ", telemetry.gammaY);

        error = xiGetParamFloat( handle, "gammaC", &(telemetry.gammaC));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get gammaC", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("gammaC = %f ", telemetry.gammaC);

        error = xiGetParamFloat( handle, "sharpness", &(telemetry.sharpness));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get sharpness", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("sharpness = %f ", telemetry.sharpness);

        error = xiGetParamFloat( handle, "ccMTX00", &(telemetry.ccMTX00));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ccMTX00", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ccMTX00 = %f ", telemetry.ccMTX00);

        error = xiGetParamFloat( handle, "ccMTX01", &(telemetry.ccMTX01));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ccMTX01", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ccMTX01 = %f ", telemetry.ccMTX01);

        error = xiGetParamFloat( handle, "ccMTX02", &(telemetry.ccMTX02));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ccMTX02", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ccMTX02 = %f ", telemetry.ccMTX02);

        error = xiGetParamFloat( handle, "ccMTX03", &(telemetry.ccMTX03));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ccMTX03", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ccMTX03 = %f ", telemetry.ccMTX03);

        error = xiGetParamFloat( handle, "ccMTX10", &(telemetry.ccMTX10));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ccMTX10", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ccMTX10 = %f ", telemetry.ccMTX10);

        error = xiGetParamFloat( handle, "ccMTX11", &(telemetry.ccMTX11));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ccMTX11", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ccMTX11 = %f ", telemetry.ccMTX11);

        error = xiGetParamFloat( handle, "ccMTX12", &(telemetry.ccMTX12));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ccMTX12", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ccMTX12 = %f ", telemetry.ccMTX12);

        error = xiGetParamFloat( handle, "ccMTX13", &(telemetry.ccMTX13));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ccMTX13", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ccMTX13 = %f ", telemetry.ccMTX13);

        error = xiGetParamFloat( handle, "ccMTX20", &(telemetry.ccMTX20));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ccMTX20", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ccMTX20 = %f ", telemetry.ccMTX20);

        error = xiGetParamFloat( handle, "ccMTX21", &(telemetry.ccMTX21));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ccMTX21", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ccMTX21 = %f ", telemetry.ccMTX21);

        error = xiGetParamFloat( handle, "ccMTX22", &(telemetry.ccMTX22));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ccMTX22", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ccMTX22 = %f ", telemetry.ccMTX22);

        error = xiGetParamFloat( handle, "ccMTX23", &(telemetry.ccMTX23));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ccMTX23", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ccMTX23 = %f ", telemetry.ccMTX23);

        error = xiGetParamFloat( handle, "ccMTX30", &(telemetry.ccMTX30));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ccMTX30", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ccMTX30 = %f ", telemetry.ccMTX30);

        error = xiGetParamFloat( handle, "ccMTX31", &(telemetry.ccMTX31));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ccMTX31", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ccMTX31 = %f ", telemetry.ccMTX31);

        error = xiGetParamFloat( handle, "ccMTX32", &(telemetry.ccMTX32));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ccMTX32", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ccMTX32 = %f ", telemetry.ccMTX32);

        error = xiGetParamFloat( handle, "ccMTX33", &(telemetry.ccMTX33));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ccMTX33", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ccMTX33 = %f ", telemetry.ccMTX33);

        error = xiGetParamInt( handle, "iscolor", &(telemetry.iscolor));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get iscolor", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("iscolor = %i ", telemetry.iscolor);

        error = xiGetParamInt( handle, "cfa", &(telemetry.cfa));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get cfa", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("cfa = %i ", telemetry.cfa);

        error = xiGetParamInt( handle, "cms", &(telemetry.cms));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get cms", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("cms = %i ", telemetry.cms);

        error = xiGetParamInt( handle, "apply_cms", &(telemetry.apply_cms));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get apply_cms", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("apply_cms = %i ", telemetry.apply_cms);

        error = xiGetParamString( handle, "input_cms_profile", telemetry.input_cms_profile, sizeof(telemetry.input_cms_profile));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get input_cms_profile", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("input_cms_profile = %s ", telemetry.input_cms_profile);

        error = xiGetParamString( handle, "output_cms_profile", telemetry.output_cms_profile, sizeof(telemetry.output_cms_profile));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get output_cms_profile", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("output_cms_profile = %s ", telemetry.output_cms_profile);

        error = xiGetParamInt( handle, "gpi_selector", &(telemetry.gpi_selector));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get gpi_selector", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("gpi_selector = %i ", telemetry.gpi_selector);

        error = xiGetParamInt( handle, "gpi_mode", &(telemetry.gpi_mode));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get gpi_mode", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("gpi_mode = %i ", telemetry.gpi_mode);

        error = xiGetParamInt( handle, "gpi_level", &(telemetry.gpi_level));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get gpi_level", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("gpi_level = %i ", telemetry.gpi_level);

        error = xiGetParamInt( handle, "gpo_selector", &(telemetry.gpo_selector));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get gpo_selector", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("gpo_selector = %i ", telemetry.gpo_selector);

        error = xiGetParamInt( handle, "gpo_mode", &(telemetry.gpo_mode));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get gpo_mode", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("gpo_mode = %i ", telemetry.gpo_mode);

        error = xiGetParamInt( handle, "acq_buffer_size", &(telemetry.acq_buffer_size));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get acq_buffer_size", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("acq_buffer_size = %i ", telemetry.acq_buffer_size);

        error = xiGetParamInt( handle, "acq_transport_buffer_size", &(telemetry.acq_transport_buffer_size));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get acq_transport_buffer_size", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("acq_transport_buffer_size = %i ", telemetry.acq_transport_buffer_size);

        error = xiGetParamInt( handle, "buffers_queue_size", &(telemetry.buffers_queue_size));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get buffers_queue_size", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("buffers_queue_size = %i ", telemetry.buffers_queue_size);

        error = xiGetParamInt( handle, "acq_transport_buffer_commit", &(telemetry.acq_transport_buffer_commit));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get acq_transport_buffer_commit", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("acq_transport_buffer_commit = %i ", telemetry.acq_transport_buffer_commit);

        error = xiGetParamInt( handle, "recent_frame", &(telemetry.recent_frame));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get recent_frame", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("recent_frame = %i ", telemetry.recent_frame);

        error = xiGetParamInt( handle, "device_reset", &(telemetry.device_reset));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get device_reset", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("device_reset = %i ", telemetry.device_reset);

        error = xiGetParamInt( handle, "aeag", &(telemetry.aeag));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get aeag", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("aeag = %i ", telemetry.aeag);

        error = xiGetParamInt( handle, "ae_max_limit", &(telemetry.ae_max_limit));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ae_max_limit", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ae_max_limit = %i ", telemetry.ae_max_limit);

        error = xiGetParamFloat( handle, "ag_max_limit", &(telemetry.ag_max_limit));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get ag_max_limit", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("ag_max_limit = %f ", telemetry.ag_max_limit);

        error = xiGetParamFloat( handle, "exp_priority", &(telemetry.exp_priority));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get exp_priority", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("exp_priority = %f ", telemetry.exp_priority);

        error = xiGetParamFloat( handle, "aeag_level", &(telemetry.aeag_level));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get aeag_level", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("aeag_level = %f ", telemetry.aeag_level);

        error = xiGetParamInt( handle, "aeag_roi_offset_x", &(telemetry.aeag_roi_offset_x));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get aeag_roi_offset_x", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("aeag_roi_offset_x = %i ", telemetry.aeag_roi_offset_x);

        error = xiGetParamInt( handle, "aeag_roi_offset_y", &(telemetry.aeag_roi_offset_y));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get aeag_roi_offset_y", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("aeag_roi_offset_y = %i ", telemetry.aeag_roi_offset_y);

        error = xiGetParamInt( handle, "dbnc_en", &(telemetry.dbnc_en));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get dbnc_en", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("dbnc_en = %i ", telemetry.dbnc_en);

        error = xiGetParamInt( handle, "dbnc_t0", &(telemetry.dbnc_t0));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get dbnc_t0", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("dbnc_t0 = %i ", telemetry.dbnc_t0);

        error = xiGetParamInt( handle, "dbnc_t1", &(telemetry.dbnc_t1));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get dbnc_t1", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("dbnc_t1 = %i ", telemetry.dbnc_t1);

        error = xiGetParamInt( handle, "dbnc_pol", &(telemetry.dbnc_pol));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get dbnc_pol", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("dbnc_pol = %i ", telemetry.dbnc_pol);

        error = xiGetParamInt( handle, "iscooled", &(telemetry.iscooled));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get iscooled", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("iscooled = %i ", telemetry.iscooled);

        error = xiGetParamInt( handle, "cooling", &(telemetry.cooling));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get cooling", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("cooling = %i ", telemetry.cooling);

        error = xiGetParamFloat( handle, "target_temp", &(telemetry.target_temp));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get target_temp", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("target_temp = %f ", telemetry.target_temp);

        error = xiGetParamInt( handle, "isexist", &(telemetry.isexist));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get isexist", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("isexist = %i ", telemetry.isexist);

        error = xiGetParamInt( handle, "bpc", &(telemetry.bpc));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get bpc", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("bpc = %i ", telemetry.bpc);

        error = xiGetParamInt( handle, "column_fpn_correction", &(telemetry.column_fpn_correction));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get column_fpn_correction", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("column_fpn_correction = %i ", telemetry.column_fpn_correction);

        error = xiGetParamInt( handle, "sensor_mode", &(telemetry.sensor_mode));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get sensor_mode", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("sensor_mode = %i ", telemetry.sensor_mode);

        error = xiGetParamInt( handle, "image_black_level", &(telemetry.image_black_level));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get image_black_level", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("image_black_level = %i ", telemetry.image_black_level);

        error = xiGetParamString( handle, "api_version", telemetry.api_version, sizeof(telemetry.api_version));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get api_version", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("api_version = %s ", telemetry.api_version);

        error = xiGetParamString( handle, "drv_version", telemetry.drv_version, sizeof(telemetry.drv_version));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get drv_version", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("drv_version = %s ", telemetry.drv_version);

        error = xiGetParamString( handle, "version_mcu1", telemetry.version_mcu1, sizeof(telemetry.version_mcu1));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get version_mcu1", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("version_mcu1 = %s ", telemetry.version_mcu1);

        error = xiGetParamString( handle, "version_fpga1", telemetry.version_fpga1, sizeof(telemetry.version_fpga1));
        if (error != XI_OK) {status = IMAGINGCAMERA_ERROR; log.error("Cannot get version_fpga1", ERR_IMAGINGCAMERA_GET_TELEMETRY);}
        log.printf("version_fpga1 = %s ", telemetry.version_fpga1);



        return (ImagingCamera_Error) log.success();

    }
    catch( const std::exception& e ){
        status = IMAGINGCAMERA_ERROR;
        return (ImagingCamera_Error) log.error(e.what(),ERR_IMAGINGCAMERA_GET_TELEMETRY_FATAL);
    }
}
