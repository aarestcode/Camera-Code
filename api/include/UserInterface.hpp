/***************************************************************************//**
 * @file	UserInterface.hpp
 * @brief	Header file to interact with files and log for debug
 *
 * This header file contains all the required definitions and function prototypes
 * through which to load/save log, csv files, images
 *
 * @author	Thibaud Talon
 * @date	21/09/2017
 *******************************************************************************/

#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <time.h>
#include <sys/time.h>

namespace UserInterface {

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   20/09/2017
 *
 * Parameters
 ******************************************************************************/
#ifndef OK
#define OK 0
#endif

enum UserInterface_Error{
    OK_USERINTERFACE = 0,

    ERR_IMG_MATRIX,

    // SaveMatAsCSV
    ERR_SAVECSV_FATAL,
    ERR_SAVECSV_MAT,
    ERR_SAVECSV_FILENAME,
    ERR_SAVECSV_OPEN,
    ERR_SAVECSV_CLOSE,

    // SaveMatAsCSV
    ERR_LOADCSV_FATAL,
    ERR_LOADCSV_FILENAME,
    ERR_LOADCSV_OPEN,
    ERR_LOADCSV_CLOSE,

    // SaveImage
    ERR_SAVEIMG_FATAL,
    ERR_SAVEIMG_FILENAME,
    ERR_SAVEIMG_WRITE,

    // LoadImage
    ERR_LOADIMG_FATAL,
    ERR_LOADIMG_FILENAME,
    ERR_LOADIMG_LOAD,

    // CreateVideo
    ERR_CREATEVIDEO_FATAL,
    ERR_CREATE_VIDEO,

};

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Log class to generate log/debug files
 ******************************************************************************/
class Log{
public:
    Log(const char* name) : _name(name) {increment = 1;} // Initialize the object
    void printMat(const char* text, cv::Mat & mat); // Display a matrix in the log
    void printf (const char * fmt, ...); // Classic printf function but formated to log format
    int error(const char* errorText, int errorInt); // Display and return errors
    int success(void); // Display and return success

private:
    const char* _name; // Name of function
    int increment; // Increment of log
    time_t rawtime; // To save the time
    struct timeval tv; // To save the time
    char time_str[100]; // String to hold the time
};

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * CSV file management functions
 ******************************************************************************/

UserInterface_Error saveMatAsCSV(cv::Mat_<float> & mat, const char * filename);
UserInterface_Error saveMatAsCSV(cv::Mat_<int> & mat, const char * filename);

UserInterface_Error loadMatFromCSV(const char * filename, cv::Mat_<float> & mat);
UserInterface_Error loadMatFromCSV(const char * filename, cv::Mat_<int> & mat);

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Images management functions
 ******************************************************************************/

UserInterface_Error saveImage(cv::Mat & img, const char * filename);
UserInterface_Error loadImage(const char * filename, cv::Mat & img);

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Videos management functions
 ******************************************************************************/

UserInterface_Error createVideo(cv::VideoWriter & video, const char * filename, float fps, int width, int height);

}

#endif // USER_INTERFACE_H

