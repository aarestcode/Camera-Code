/***************************************************************************//**
 * @file	AAReST.hpp
 * @brief	Header file to define parameters for the AAReST mission
 *
 * This header file contains all the required definitions
 * for the AAReST mission for the telescope
 *
 * @author	Thibaud Talon
 * @date	25/09/2017
 *******************************************************************************/

#ifndef AAREST_H
#define AAREST_H

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   25/09/2017
 *
 * Boom Inspection Camera
 ******************************************************************************/
#define BIC_EXPOSURE 10000 // Exposure of sensor in us
#define BIC_GAIN 0 // Gain of sensor in dB
#define BIC_OFFSETX 560 // Horizontal offset of ROI in px
#define BIC_OFFSETY 150 // Vertical offset of ROI in px
#define BIC_WIDTH 1560 // Width of ROI in px
#define BIC_HEIGHT 1560 // Height of ROI in px

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   25/09/2017
 *
 * Science Camera
 ******************************************************************************/
#define SCIENCE_CAMERA_EXPOSURE 50000 // Exposure of sensor in us
#define SCIENCE_CAMERA_GAIN 0 // Gain of sensor in dB

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   25/09/2017
 *
 * SHWS looking at -Y aperture
 ******************************************************************************/
#define SHWS_mY_EXPOSURE 10000 // Exposure of sensor in us
#define SHWS_mY_GAIN 0 // Gain of sensor in dB
#define SHWS_mY_OFFSETX 0 // Horizontal offset of ROI in px
#define SHWS_mYC_OFFSETY 0 // Vertical offset of ROI in px
#define SHWS_mY_WIDTH 2040 // Width of ROI in px
#define SHWS_mY_HEIGHT 2040 // Height of ROI in px

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   25/09/2017
 *
 * SHWS looking at +Y aperture
 ******************************************************************************/
#define SHWS_pY_EXPOSURE 10000 // Exposure of sensor in us
#define SHWS_pY_GAIN 0 // Gain of sensor in dB
#define SHWS_pY_OFFSETX 0 // Horizontal offset of ROI in px
#define SHWS_pY_OFFSETY 0 // Vertical offset of ROI in px
#define SHWS_pY_WIDTH 2040 // Width of ROI in px
#define SHWS_pY_HEIGHT 2040 // Height of ROI in px

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   25/09/2017
 *
 * Mask motor
 ******************************************************************************/
#define MASK_RESOLUTION 4 // Step resolution of the motor. See Mask.hpp > Mask_Resolution
#define MASK_SPEED 100 // Speed in Hz (frequency)

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   25/09/2017
 *
 * Mirrors
 ******************************************************************************/
#define RM1_ADDRESS 0x0013A20040E6882E // MAC address of Rigid Mirror 1
#define RM2_ADDRESS 0x0013A20040E687FE // MAC address of Rigid Mirror 2
#define DM1_ADDRESS 0x0013A20041757D5B // MAC address of Deformable Mirror 1
#define DM2_ADDRESS 0x0013A20040E9A42B // MAC address of Deformable Mirror 2


#endif
