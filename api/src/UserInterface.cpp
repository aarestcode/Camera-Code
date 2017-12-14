/***************************************************************************//**
 * @file	UserInterface.cpp
 * @brief	Source file to interact with files and log for debug
 *
 * This file contains all the implementations for the functions defined in:
 * api/include/UserInterface.hpp
 *
 * @author	Thibaud Talon
 * @date	21/09/2017
 *******************************************************************************/

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdio.h>
#include <stdarg.h> // va_list
#include <stdlib.h> // atof
#include <iostream> // streams (clog)
#include <fstream>  // std::ifstream
#include <string.h> // strchr
#include <iomanip>  // stew, setfill
#include <time.h>   // time_t, struct tm, difftime, time, mktime
#include <sys/time.h>
#include "UserInterface.hpp"

namespace UserInterface {

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Display a matrix in the log
 *
 * @param [in] text
 *	Name of the matrix
 * @param [in] mat
 *	Matrix to display
 ******************************************************************************/
void Log::printMat(const char* text, cv::Mat & mat){
    int N = mat.rows;
    for(int II=0; II<N; II++){
        gettimeofday(&tv, NULL);
        rawtime = tv.tv_sec;
        strftime( time_str, 100, "%F\t%T", localtime (&rawtime));
        std::cout << time_str << "." << std::setfill('0') << std::setw(6) << tv.tv_usec << '\t' << _name << '\t' << increment << '\t' << text << " = " << mat.row(II) << std::endl;
        increment++;
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Classic printf function but in the log format
 *
 * @param [in] fmt
 *	Formated text (see: http://www.cplusplus.com/reference/cstdio/printf/)
 ******************************************************************************/
void Log::printf (const char * fmt, ...){
    va_list va;
    va_start(va, fmt);
    char buffer[256];
    vsprintf(buffer,fmt, va);
    va_end(va);

    gettimeofday(&tv, NULL);
    rawtime = tv.tv_sec;
    strftime( time_str, 100, "%F\t%T", localtime (&rawtime));
    std::cout << time_str << "." << std::setfill('0') << std::setw(6) << tv.tv_usec << '\t' << _name << '\t' << increment << '\t' << buffer << std::endl;
    increment++;
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Display and return errors
 *
 * @param [in] errorText
 *	Description of the error
 * @param [in] errorInt
 *	Error code
 ******************************************************************************/
int Log::error(const char* errorText, int errorInt){
    gettimeofday(&tv, NULL);
    rawtime = tv.tv_sec;
    strftime( time_str, 100, "%F\t%T", localtime (&rawtime));
    std::cout << time_str << "." << std::setfill('0') << std::setw(6) << tv.tv_usec << '\t' << _name << '\t' << increment << "\tERROR = " << errorText << " (" << errorInt << ")" << std::endl;
    increment++;
    return errorInt;
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Display and return success
 *
 ******************************************************************************/
int Log::success(){
    gettimeofday(&tv, NULL);
    rawtime = tv.tv_sec;
    strftime( time_str, 100, "%F\t%T", localtime (&rawtime));
    std::cout << time_str << "." << std::setfill('0') << std::setw(6) << tv.tv_usec << '\t' << _name << '\t' << increment << "\tNO ERROR (" << OK << ")" << std::endl;
    increment++;
    return OK;
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Save OpenCV Mat_ to .csv file
 *
 * @param [in] mat
 *	Matrix to save
 * @param [in] filename
 *	Name of the file (ends with .csv)
 ******************************************************************************/
UserInterface_Error saveMatAsCSV(cv::Mat_<float> & mat, const char * filename){
    Log log("UserInterface::saveMatAsCSV");
    try{
        // 1. Check inputs
        log.printf("1. Check inputs");
        if ( mat.empty() ) return (UserInterface_Error) log.error("No data",ERR_SAVECSV_MAT);
        if (sizeof(filename) == 0) return (UserInterface_Error) log.error("No filename", ERR_SAVECSV_FILENAME);

        // 2. Create new file
        log.printf("2. Create new file");
        std::ofstream csvfile;
        csvfile.open(filename);
        if ( !csvfile ) return (UserInterface_Error) log.error("Cannot create file",ERR_SAVECSV_OPEN);

        // 3. Loop over vector and save
        log.printf("3. Loop over vector and save");
        csvfile.precision(10);
        for(int II=0; II < mat.rows; II++){
            for(int III=0; III < mat.cols; III++){
                csvfile << mat(II,III);

                if (III < mat.cols-1) csvfile << ",";
                else csvfile << std::endl;
            }
        }
        csvfile.close();

        return (UserInterface_Error) log.success();
    }
    catch( const std::exception& e ){
        return (UserInterface_Error) log.error(e.what(),ERR_SAVECSV_FATAL);
    }
}
UserInterface_Error saveMatAsCSV(cv::Mat_<int> & mat, const char * filename){
    Log log("UserInterface::saveMatAsCSV");
    try{
        // 1. Check inputs
        log.printf("1. Check inputs");
        if ( mat.empty() ) return (UserInterface_Error) log.error("No data",ERR_SAVECSV_MAT);
        if (sizeof(filename) == 0) return (UserInterface_Error) log.error("No filename", ERR_SAVECSV_FILENAME);

        // 2. Create new file
        log.printf("2. Create new file");
        std::ofstream csvfile;
        csvfile.open(filename);
        if ( !csvfile ) return (UserInterface_Error) log.error("Cannot create file",ERR_SAVECSV_OPEN);

        // 3. Loop over vector and save
        log.printf("3. Loop over vector and save");
        csvfile.precision(10);
        for(int II=0; II < mat.rows; II++){
            for(int III=0; III < mat.cols; III++){
                csvfile << mat(II,III);

                if (III < mat.cols-1) csvfile << ",";
                else csvfile << std::endl;
            }
        }
        csvfile.close();

        return (UserInterface_Error) log.success();
    }
    catch( const std::exception& e ){
        return (UserInterface_Error) log.error(e.what(),ERR_SAVECSV_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Load .csv file OpenCV Mat_
 *
 * @param [in] filename
 *	Name of the file (ends with .csv)
 * @param [out] mat
 *	Matrix to load the data in
 ******************************************************************************/
UserInterface_Error loadMatFromCSV(const char * filename, cv::Mat_<float> &mat){
    Log log("UserInterface::loadMatFromCSV");

    try{
        // 1. Check inputs
        log.printf("1. Check inputs");
        if (sizeof(filename) == 0) return (UserInterface_Error) log.error("No filename", ERR_LOADCSV_FILENAME);

        // 2. Open file
        log.printf("2. Open file");
        std::ifstream csvfile;
        csvfile.open(filename);
        if ( !csvfile ) return (UserInterface_Error) log.error("Cannot open file", ERR_LOADCSV_OPEN);

        // 3. Find number of columns by reading first line
        log.printf("3. Find number of columns by reading first line");
        mat = cv::Mat_<float>(1, 1);
        char buffer[256];

        // First line
        csvfile.getline(buffer,256);
        mat(0) = (float)atof(buffer);
        char * comma = strchr(buffer,',');

        // Remaining of line
        while (comma!=NULL){
            mat.push_back((float)atof(comma+1));
            comma = strchr(comma+1,',');
        }

        // Reformet matrix
        mat = mat.t();
        log.printf("cols = %i", mat.cols);

        // 4. Loop over file and load
        log.printf("4. Loop over file and load");
        cv::Mat_<float> row = cv::Mat_<float>::zeros(1,mat.cols);
        csvfile.getline(buffer,256);
        while(!csvfile.eof() && !csvfile.fail()){

            row(0) = (float)atof(buffer);
            comma = strchr(buffer,',');

            // Remaining of line
            int II = 1;
            while (comma!=NULL){
                row(II++) = (float)atof(comma+1);
                comma = strchr(comma+1,',');
            }

            if (II == mat.cols) mat.push_back(row);
            csvfile.getline(buffer,256);
        }
        log.printf("rows = %i", mat.rows);
        csvfile.close();

        return (UserInterface_Error) log.success();
    }
    catch( const std::exception& e ){
        return (UserInterface_Error) log.error(e.what(),ERR_LOADCSV_FATAL);
    }
}
UserInterface_Error loadMatFromCSV(const char * filename, cv::Mat_<int> &mat){
    Log log("UserInterface::loadMatFromCSV");

    try{
        // 1. Check inputs
        log.printf("1. Check inputs");
        if (sizeof(filename) == 0) return (UserInterface_Error) log.error("No filename", ERR_LOADCSV_FILENAME);

        // 2. Open file
        log.printf("2. Open file");
        std::ifstream csvfile;
        csvfile.open(filename);
        if ( !csvfile ) return (UserInterface_Error) log.error("Cannot open file", ERR_LOADCSV_OPEN);

        // 3. Find number of columns by reading first line
        log.printf("3. Find number of columns by reading first line");
        mat = cv::Mat_<int>(1, 1);
        char buffer[256];

        // First line
        csvfile.getline(buffer,256);
        mat(0) = (int)atoi(buffer);
        char * comma = strchr(buffer,',');

        // Remaining of line
        while (comma!=NULL){
            mat.push_back((int)atoi(comma+1));
            comma = strchr(comma+1,',');
        }

        // Reformet matrix
        mat = mat.t();
        log.printf("cols = %i", mat.cols);

        // 4. Loop over file and load
        log.printf("4. Loop over file and load");
        cv::Mat_<int> row = cv::Mat_<int>::zeros(1,mat.cols);
        csvfile.getline(buffer,256);
        while(!csvfile.eof() && !csvfile.fail()){

            row(0) = (int)atoi(buffer);
            comma = strchr(buffer,',');

            // Remaining of line
            int II = 1;
            while (comma!=NULL){
                row(II++) = (int)atoi(comma+1);
                comma = strchr(comma+1,',');
            }

            if (II == mat.cols) mat.push_back(row);
            csvfile.getline(buffer,256);
        }
        log.printf("rows = %i", mat.rows);
        csvfile.close();

        return (UserInterface_Error) log.success();
    }
    catch( const std::exception& e ){
        return (UserInterface_Error) log.error(e.what(),ERR_LOADCSV_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Save OpenCV Mat to image file (extension in filename)
 *
 * @param [in] img
 *	Image to save
 * @param [in] filename
 *	Name of the file (ends with .png or .jpg)
 ******************************************************************************/
UserInterface_Error saveImage(cv::Mat & img, const char * filename){
    Log log("UserInterface::saveImage");
    try{
        // 1. Check inputs
        log.printf("1. Check inputs");
        if ( img.empty() ) return (UserInterface_Error) log.error("No image", ERR_IMG_MATRIX);
        if (sizeof(filename) == 0) return (UserInterface_Error) log.error("No filename", ERR_SAVEIMG_FILENAME);

        // 2. Save the image
        log.printf("2. Save the image");
        if ( cv::imwrite(filename, img) == false) return (UserInterface_Error) log.error("Cannot save the image", ERR_SAVEIMG_WRITE);

        return (UserInterface_Error) log.success();
    }
    catch( const std::exception& e ){
        return (UserInterface_Error) log.error(e.what(),ERR_SAVEIMG_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Load image file OpenCV Mat
 *
 * @param [in] filename
 *	Name of the file (ends with .png or .jpg)
 * @param [out] mat
 *	Image to load the data in
 ******************************************************************************/
UserInterface_Error loadImage(const char * filename, cv::Mat & img){
    Log log("UserInterface::loadImage");
    try{
        // 1. Check inputs
        log.printf("1. Check inputs");
        if (sizeof(filename) == 0) return (UserInterface_Error) log.error("No filename", ERR_LOADIMG_FILENAME);

        // 2. Load the image
        log.printf("2. Load the image");
        img = cv::imread(filename, 0);
        if ( img.empty() ) return (UserInterface_Error) log.error("Cannot load the image", ERR_LOADIMG_LOAD);

        return (UserInterface_Error) log.success();
    }
    catch( const std::exception& e ){
        return (UserInterface_Error) log.error(e.what(),ERR_LOADIMG_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Videos management functions
 *
 * @param [out] video
 *	Video object
 * @param [in] filename
 *	Name of the file (ends with .avi)
 * @param [in] fps
 *	Frames per second of the video
 * @param [in] width
 *	Width of the images in the video
 * @param [in] height
 *	Height of the images in the video
 ******************************************************************************/

UserInterface_Error createVideo(cv::VideoWriter & video, const char * filename, float fps, int width, int height){
    Log log("UserInterface::createVideo");
    try{
        video.open(filename, CV_FOURCC('M','P','4','2'), (double)fps, cv::Size(width, height), 0);
        // CV_FOURCC('M','P','4','2') CV_FOURCC('D','I','V','3') CV_FOURCC('D','I','V','4')
        if (!video.isOpened()) return (UserInterface_Error) log.error("Cannot create the video file", ERR_CREATE_VIDEO);

        return (UserInterface_Error) log.success();
    }
    catch( const std::exception& e ){
        return (UserInterface_Error) log.error(e.what(),ERR_CREATEVIDEO_FATAL);
    }
}

}

