/***************************************************************************//**
 * @file	UserInterface_LoadSaveIMG.cpp
 * @brief	Test file to load and save image files
 *
 * @author	Thibaud Talon
 * @date	21/09/2017
 *
 * @param [in] filename
 *	Input image file
 * @param [in] filename
 *	Output PNG image file (ends with .png)
 * @param [in] filename
 *	Output JPG image file (ends with .jpg)
 *******************************************************************************/

#include "UserInterface.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("Algorithm");

    // 1. Parsing data
    log.printf("1. Parsing data");
    if(argc < 4) return log.error("No filenames (input image, output PNG image, output JPG image) specified",-1);
    else if(argc > 4) log.printf("WARNING: Extra inputs discarded");

    UserInterface::UserInterface_Error error;
    cv::Mat img;

    // 2. Load image
    log.printf("2. Load image");
    if( error = UserInterface::loadImage(argv[1], img) ) return log.error("Error loading image", error);
    log.printf("width = %i", img.cols);
    log.printf("height = %i", img.rows);

    // 3. Save image as PNG
    log.printf("3. Save image as PNG");
    if( error = UserInterface::saveImage(img, argv[2]) ) return log.error("Error saving image", error);

    // 4. Save image as JPG
    log.printf("4. Save image as JPG");
    if( error = UserInterface::saveImage(img, argv[3]) ) return log.error("Error loading image", error);

    return log.success();
}
