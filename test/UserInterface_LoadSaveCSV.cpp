/***************************************************************************//**
 * @file	UserInterface_LoadSaveCSV.cpp
 * @brief	Test file to load, display and save CSV files
 *
 * @author	Thibaud Talon
 * @date	21/09/2017
 *
 * @param [in] filename
 *	Input CSV file
 * @param [in] filename
 *	Output CSV file when reading input as floating numbers
 * @param [in] filename
 *	Output CSV file when reading input as integer numbers
 *******************************************************************************/

#include "UserInterface.hpp"

int main(int argc, char* argv[]){
    UserInterface::Log log("Algorithm");

    // 1. Parsing data
    log.printf("1. Parsing data");
    if(argc < 4) return log.error("No filenames (input CSV, output float CSV, output int CSV) specified",-1);
    else if(argc > 4) log.printf("WARNING: Extra inputs discarded");

    UserInterface::UserInterface_Error error;

    // 2. Load matrix as float
    log.printf("2. Load matrix as float");
    cv::Mat_<float> fmat;
    if( error = UserInterface::loadMatFromCSV(argv[1], fmat) ) return log.error("Error loading matrix", error);
    log.printMat("Matrix:", fmat);

    // 3. Save matrix as float
    log.printf("3. Save matrix as float");
    if( error = UserInterface::saveMatAsCSV(fmat, argv[2]) ) return log.error("Error saving matrix", error);

    // 4. Load matrix as int
    log.printf("4. Load matrix as int");
    cv::Mat_<int> mat;
    if( error = UserInterface::loadMatFromCSV(argv[1], mat) ) return log.error("Error loading matrix", error);
    log.printMat("Matrix:", mat);

    // 5. Save matrix as int
    log.printf("5. Save matrix as int");
    if( error = UserInterface::saveMatAsCSV(mat, argv[3]) ) return log.error("Error saving matrix", error);

    return log.success();
}
