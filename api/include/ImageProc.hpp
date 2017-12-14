/***************************************************************************//**
 * @file	ImageProc.hpp
 * @brief	Header file to process images
 *
 * This header file contains all the required definitions and function prototypes
 * through which to process images images
 *
 * @author	Thibaud Talon
 * @date	02/10/2017
 *******************************************************************************/

#ifndef IMAGE_PROC_H
#define IMAGE_PROC_H

#include <opencv2/core/core.hpp>

namespace ImageProc{

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   02/10/2017
 *
 * Parameters
 ******************************************************************************/
#ifndef OK
#define OK 0
#endif

enum ImageProc_Error{
    OK_IMAGEPROC = 0,

    ERR_IMG_MATRIX,

    // preProcessImage
    ERR_FILTER_FATAL,
    ERR_FILTER_THRESH,
    ERR_FILTER_ERODE,
    ERR_FILTER_DILATE,

    // cutImage
    ERR_CUT_FATAL,
    ERR_CUT_ROI_LEFT_OOB,
    ERR_CUT_ROI_TOP_OOB,

    // getSpotLoc
    ERR_SPOTLOC_FATAL,

    // getSpotLoc
    ERR_SPOTSLOC_AREA,
    ERR_SPOTSLOC_CIRCULARITY,
    ERR_SPOTSLOC_SIZE,
    ERR_SPOTSLOC_FATAL,
    ERR_SPOTSLOC_KEYPTS_SIZE,

    // getRadiusofEncircleEnergy
    ERR_ENCIRCLE_FATAL,
    ERR_ENCIRCLE_CENTER_ROWS_OOB,
    ERR_ENCIRCLE_CENTER_COLS_OOB,
    ERR_ENCIRCLE_ENERGY_OOB,
    ERR_ENCIRCLE_ERROR_OOB,
    ERR_ENCIRCLE_ERROR_TOOMANYITERATIONS,
};

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Functions
 ******************************************************************************/
ImageProc_Error filter(cv::Mat & img, int threshold_value, int erode_iterations, int dilate_iterations, cv::Mat & filtered_img, int order); // Filter an image
ImageProc_Error cut(cv::Mat & img, int roiLeft, int roiTop, int roiWidth, int roiHeigh, cv::Mat & cut_img); // Cut an image
ImageProc_Error getSpotLoc(cv::Mat & img, cv::Mat_<float> & spotsPositionArray); // Find centroid of light
ImageProc_Error getSpotsLoc(cv::Mat & img, cv::Mat_<float> & spotsPositionArray, float minArea, float maxArea, float minInertiaRatio, float maxInertiaRatio); // Find all the spots
ImageProc_Error getRadiusOfEncircleEnergy(cv::Mat & img, const cv::Mat_<float> & center, float energy, float error, float & radius, int Nmax); // Get radius of encircled energy


} // namespace


#endif
