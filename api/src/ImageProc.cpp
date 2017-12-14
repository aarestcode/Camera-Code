/***************************************************************************//**
 * @file	UserInterface.cpp
 * @brief	Source file to process images
 *
 * This file contains all the implementations for the functions defined in:
 * api/include/ImageProc.hpp
 *
 * @author	Thibaud Talon
 * @date	02/10/2017
 *******************************************************************************/

#include <math.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp> // for getSpotLoc
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp> // for Params
#include "UserInterface.hpp"
#include "ImageProc.hpp"

namespace ImageProc{

#define nchoosek(n,k) tgamma(n+1)/(tgamma(n-(k)+1)*tgamma(k+1))

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Filter an image
 *
 * @param [in] img
 *	Image to filter
 * @param [in] threshold_value
 *	Value of threshold (0 - 255)(Pixels under the threshold are set to 0)
 * @param [in] erode_iterations
 *	Number of erosions to apply
 * @param [in] dilate_iterations
 *	Number of dilations to apply
 * @param [out] filtered_img
 *	Returned filtered image
 * @param [in] order
 *	If order = 0, erosion first then dilation, otherwise it's the inverse
 ******************************************************************************/
ImageProc_Error filter(cv::Mat & img, int threshold_value, int erode_iterations, int dilate_iterations, cv::Mat & filtered_img, int order = 0) {
    // this is intended for use on a Mat with 8 bit elements
    // this filters the image enough so that you can work with the detector while the lights are on
    UserInterface::Log log("ImageProc::filter");
    try
    {
        // 1. Check the inputs
        log.printf("1. Check the inputs");
        if ( img.empty() ) return (ImageProc_Error) log.error("No image", ERR_IMG_MATRIX);
        if ( threshold_value < 0 ||  threshold_value > 255 )  return (ImageProc_Error) log.error("Threshold out-of-bounds", ERR_FILTER_THRESH); // invalid threshold value
        if ( erode_iterations < 0  ) return (ImageProc_Error) log.error("Negative number of erosions", ERR_FILTER_ERODE); // invalid erode iterations value
        if ( dilate_iterations < 0  )  return (ImageProc_Error) log.error("Negative number of dilations", ERR_FILTER_DILATE); // invalid dilate iterations value

        img.copyTo(filtered_img);

        // 2. Apply threshold
        log.printf("2. Apply threshold = %i",threshold_value);
        cv::threshold( filtered_img, filtered_img, threshold_value, 255,cv::THRESH_TOZERO);


        if(order==0){
            // 3. Apply the erosion operation
            log.printf("3. Apply the erosion operation = %i",erode_iterations);
            cv::erode( filtered_img, filtered_img, cv::Mat(),cv::Point(-1,-1),erode_iterations);

            // 4. Apply the dilation operation
            log.printf("4. Apply the dilation operation = %i",dilate_iterations);
            cv::dilate( filtered_img, filtered_img, cv::Mat(),cv::Point(-1,-1),dilate_iterations);
        }
        else{
            // 3. Apply the dilation operation
            log.printf("3. Apply the dilation operation = %i",dilate_iterations);
            cv::dilate( filtered_img, filtered_img, cv::Mat(),cv::Point(-1,-1),dilate_iterations);

            // 4. Apply the erosion operation
            log.printf("4. Apply the erosion operation = %i",erode_iterations);
            cv::erode( filtered_img, filtered_img, cv::Mat(),cv::Point(-1,-1),erode_iterations);

        }


        return (ImageProc_Error) log.success();
    }
    catch(  const std::exception& e  ){
        return (ImageProc_Error) log.error(e.what(), ERR_FILTER_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Cut an image
 *
 * @param [in] img
 *	Image to cut
 * @param [in] roiLeft
 *	Left position of the Region of Interest
 * @param [in] roiTop
 *	Top position of the Region of Interest
 * @param [in] roiWidth
 *	Width of the Region of Interest
 * @param [in] roiHeight
 *	Height of the Region of Interest
 * @param [out] cut_img
 *	Cut image
 ******************************************************************************/
ImageProc_Error cut(cv::Mat & img, int roiLeft, int roiTop, int roiWidth, int roiHeight, cv::Mat & cut_img){
    UserInterface::Log log("ImageProc::cut");
    try{
        // 1. Check the inputs
        log.printf("1. Check the inputs");
        if ( img.empty() ) return (ImageProc_Error) log.error("No image", ERR_IMG_MATRIX);
        int width = img.cols;
        int height = img.rows;
        log.printf("Image size = %ix%i pixels", width, height);
        log.printf("ROI top-left corner = %ix%i pixels", roiLeft, roiTop);
        log.printf("ROI size = %ix%i pixels", roiWidth, roiHeight);
        if( (roiLeft >= width) || roiLeft < 0 ) return (ImageProc_Error) log.error("Left position of region of interest out-of-bounds", ERR_CUT_ROI_LEFT_OOB);
        if( (roiTop >= height) || roiTop < 0 ) return (ImageProc_Error) log.error("Top position of region of interest out-of-bounds", ERR_CUT_ROI_TOP_OOB);
        roiWidth = std::min(roiWidth,width - roiLeft);
        roiHeight = std::min(roiHeight,height - roiTop);

        // 2. Perform cut
        log.printf("2. Perform cut");
        cut_img = img( cv::Rect(roiLeft, roiTop, roiWidth, roiHeight) );

        return (ImageProc_Error) log.success();
    }
    catch( const std::exception& e ){
        return (ImageProc_Error) log.error(e.what(), ERR_CUT_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Get centroid of light (center of single spot on image)
 *
 * @param [in] img
 *	Image
 * @param [out] spotPositionArray
 *	Vector returning the centroid of the light (x;y)
 ******************************************************************************/
ImageProc_Error getSpotLoc(cv::Mat & img, cv::Mat_<float> & spotPositionArray){
    UserInterface::Log log("ImageProc::getSpotLoc");
    try{
        // 1. Check the inputs
        log.printf("1. Check the inputs");
        if ( img.empty() ) return (ImageProc_Error) log.error("No image", ERR_IMG_MATRIX);

        // 2. Find the center of mass (intensity center)
        log.printf("2. Find the center of mass (intensity center)");
        // Get the moments
        cv::Moments m = cv::moments(img, false);

        // Get the center of mass // If no intensity on image, return {-1; -1}
        spotPositionArray = cv::Mat_<float>(2,1);
        if (m.m00==0){
            spotPositionArray(0) = -1;
            spotPositionArray(1) = -1;
        }
        else {
            spotPositionArray(0) = m.m10/m.m00;
            spotPositionArray(1) = m.m01/m.m00;
        }

        return (ImageProc_Error) log.success();
    }
    catch( const std::exception& e ){
        return (ImageProc_Error) log.error(e.what(), ERR_SPOTLOC_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Get all the spots on an imgae
 *
 * @param [in] img
 *	Image
 * @param [out] spotsPositionArray
 *	Vector returning the centroid of the light (x; y; size)
 * @param [in] minArea
 *	Minimum area of a spot
 * @param [in] maxArea
 *	Maximum area of a spot
 * @param [in] minCircularity
 *	Minimum circularity of a spot
 * @param [in] maxCircularity
 *	Maximum circularity of a spot
 * @param [in] maxSize
 *	Maximum size ("diameter") of spot
 ******************************************************************************/
ImageProc_Error getSpotsLoc(cv::Mat & img, cv::Mat_<float> & spotsPositionArray, float minArea = 3, float maxArea = 4000000, float minCircularity = 0, float maxCircularity = 1, float maxSize = 2000)
{
    UserInterface::Log log("ImageProc::getSpotsLoc");
    try{
        // 1. Check the inputs
        log.printf("1. Check the inputs");
        if ( img.empty() ) return (ImageProc_Error) log.error("No image", ERR_IMG_MATRIX);
        if ( maxArea < minArea ) return (ImageProc_Error) log.error("maxArea smaller than minArea", ERR_SPOTSLOC_AREA);
        if ( maxCircularity < minCircularity ) return (ImageProc_Error) log.error("maxCircularity smaller than minCircularity", ERR_SPOTSLOC_CIRCULARITY);
        if ( maxSize*maxSize < minArea ) return (ImageProc_Error) log.error("maxSize smaller than minArea", ERR_SPOTSLOC_SIZE);

        // 2. Set the parameters of the blob detector
        log.printf("2. Set the parameters of the blob detector");
        cv::SimpleBlobDetector::Params params;

        // Filter by Area
        params.filterByArea = true;
        params.minArea = minArea;
        params.maxArea = maxArea;
        log.printf("minArea = %f", params.minArea);
        log.printf("maxArea = %f", params.maxArea);

        // Filter by Circularity
        params.filterByCircularity = true;
        params.minCircularity  = minCircularity;
        params.maxCircularity  = maxCircularity;
        log.printf("minCircularity  = %f", params.minCircularity );
        log.printf("maxCircularity  = %f", params.maxCircularity );

        // Filter by Inertia
        params.filterByInertia = false;

        // Filter by Color
        params.filterByColor = false;

        // Filter by Convexity
        params.filterByConvexity = false;

        // 3. Detect blobs
        log.printf("3. Detect blobs");
        cv::SimpleBlobDetector detector(params);
        std::vector<cv::KeyPoint> keypoints;
        detector.detect(img, keypoints);

        if ( keypoints.size() == 0 )  return (ImageProc_Error) log.error("No blob detected", ERR_SPOTSLOC_KEYPTS_SIZE);

        // 4. Load Spots into output array
        log.printf("4. Load Spots into output array");
        spotsPositionArray = cv::Mat_<float>::zeros(0,3);
        cv::Mat_<float> data = cv::Mat_<float>::zeros(1,3);
        ImageProc_Error errorCode = OK_IMAGEPROC;
        for (int i=0;i<keypoints.size();i++){
            if ( 4.*keypoints.at(i).size < maxSize ){
                data(0) = (float)keypoints.at(i).pt.x;
                data(1) = (float)keypoints.at(i).pt.y;
                data(2) = 4.*keypoints.at(i).size;
                spotsPositionArray.push_back( data );

                if ((keypoints.at(i).pt.x - 2*keypoints.at(i).size < 1) ||
                        (keypoints.at(i).pt.y - 2*keypoints.at(i).size < 1) ||
                        (keypoints.at(i).pt.x + 2*keypoints.at(i).size > img.cols-1) ||
                        (keypoints.at(i).pt.y + 2*keypoints.at(i).size > img.rows-1))
                    log.printf("Spot #%i on the edge of the image", i);
            }

        }
        spotsPositionArray = spotsPositionArray.t();

        return (ImageProc_Error) log.success();
    }
    catch( const std::exception& e ){
        return (ImageProc_Error) log.error(e.what(), ERR_SPOTSLOC_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Find the radius of encircled energy
 *
 * @param [in] img
 *	Image with one unique spot
 * @param [in] center
 *	Center of unique spot
 * @param [in] energy
 *	Percentage of energy inside the circle
 * @param [in] tol
 *	Tolerance on the energy (%)
 * @param [out] radius
 *	Radius of circle in px
 * @param [in] Nmax
 *	Maximum number of iteration
 ******************************************************************************/
ImageProc_Error getRadiusOfEncircleEnergy(cv::Mat & img, const cv::Mat_<float> & center, float energy, float tol, float & radius, int Nmax = 100)
{
    UserInterface::Log log("ImageProc::getRadiusOfEncircleEnergy");
    try{
        // 1. Check the inputs
        log.printf("1. Check the inputs");
        if ( img.empty() ) return (ImageProc_Error) log.error("No image", ERR_IMG_MATRIX);
        if ( center.cols != 1) return (ImageProc_Error) log.error("Center not a vector", ERR_ENCIRCLE_CENTER_COLS_OOB);
        if ( center.rows != 2) return (ImageProc_Error) log.error("Too many centers", ERR_ENCIRCLE_CENTER_ROWS_OOB);
        if ( energy > 100 || energy < 0) return (ImageProc_Error) log.error("Energy target out-of-bounds", ERR_ENCIRCLE_ENERGY_OOB);
        if ( tol <= 0 ) return (ImageProc_Error) log.error("Error on energy out-of-bounds", ERR_ENCIRCLE_ERROR_OOB);

        // 2. Find radius with a binary process
        log.printf("2. Find radius with a binary process");
        float Rmax = pow(pow(img.rows,2)+pow(img.cols,2),0.5);
        float Rmin = 0;
        float R = (Rmax+Rmin)/2;
        cv::Mat mask = cv::Mat::zeros(img.size(),CV_8U);
        int shift = 10;
        cv::Point centerP;
        centerP.x = center(0);
        centerP.y = center(1);
        cv::circle(mask, centerP, (int)(R*pow(2,shift)), cv::Scalar(255,255,255), -1, CV_AA, shift);
        cv::Mat masked_img = img.mul(mask/255);
        float intensity0 = cv::sum(img)(0);
        float intensity = 100*cv::sum(masked_img)(0)/intensity0;
        int n = 0;

        // Binary process
        while( (fabs(intensity-energy) > tol) && n<Nmax)
        {
            log.printf("Error = %f", fabs(intensity-energy));
            log.printf("Radius = %f", R);

            // Calculate new radius
            if ( intensity > energy ) Rmax = R;
            else Rmin = R;

            R = (Rmin+Rmax)/2;

            // Create new mask
            mask.setTo(cv::Scalar(0,0,0));
            cv::circle(mask, centerP, (int)(R*pow(2,shift)), cv::Scalar(255,255,255), -1, CV_AA, shift);
            masked_img = img.mul(mask/255);

            // Calculate new intensity
            intensity = 100*cv::sum(masked_img)(0)/intensity0;
            n += 1;
        }

        if (n==Nmax) return (ImageProc_Error) (ImageProc_Error) log.error("Too many iterations", ERR_ENCIRCLE_ERROR_TOOMANYITERATIONS);

        radius = R;

        return (ImageProc_Error) log.success();
    }
    catch( const std::exception& e ){
        return (ImageProc_Error) log.error(e.what(), ERR_ENCIRCLE_FATAL);
    }
}


} // namespace
