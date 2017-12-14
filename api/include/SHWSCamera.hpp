/***************************************************************************//**
 * @file	SHWSCamera.hpp
 * @brief	Header file to operate the Shack–Hartmann wavefront sensor
 *
 * This header file contains all the required definitions and function prototypes
 * through which to control the AAReST Shack–Hartmann wavefront sensor
 *
 * @author	Thibaud Talon
 * @date	22/09/2017
 *******************************************************************************/

#ifndef SHWS_CAMERA_H
#define SHWS_CAMERA_H

#include <opencv2/core/core.hpp>
#include "bgapi2_genicam.hpp"

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   22/09/2017
 *
 * Parameters
 ******************************************************************************/
#ifndef OK
#define OK 0
#endif

enum SHWSCamera_Index{
    SHWSCAMERA_mY_APERTURE = 0, // sensor looking at the -Y aperture
    SHWSCAMERA_pY_APERTURE = 1  // sensor looking at the +Y aperture
};

enum SHWSCamera_Error{
    OK_SHWSCAMERA = 0,

    ERR_SHWSCAMERA_SYSTEM_LIST_FATAL,
    ERR_SHWSCAMERA_INTERFACE_LIST_FATAL,
    ERR_SHWSCAMERA_INTERFACE_ALREADY_OPENED,
    ERR_SHWSCAMERA_INTERFACE_NEXT_FATAL,
    ERR_SHWSCAMERA_SYSTEM_ALREADY_OPENED,
    ERR_SHWSCAMERA_INTERFACE_FATAL,
    ERR_SHWSCAMERA_NO_SYSTEM_FOUND,
    ERR_SHWSCAMERA_NO_CAMERA_FOUND,
    ERR_SHWSCAMERA_DEVICE_ALREADY_OPENED,
    ERR_SHWSCAMERA_DEVICE_ACCESS_DENIED,
    ERR_SHWSCAMERA_OPEN_DEVICE_FATAL,
    ERR_SHWSCAMERA_NO_DEVICE_FOUND,
    ERR_SHWSCAMERA_TRIGGER_MODE_FATAL,
    ERR_SHWSCAMERA_SET_PACKET_DELAY_FATAL,

    ERR_SHWSCAMERA_DISCONNECT_FATAL,

    ERR_SHWSCAMERA_NO_DEVICE,

    ERR_SHWSCAMERA_DATASTREAM_LIST_FATAL,
    ERR_SHWSCAMERA_DATASTREAM_OPEN_FATAL,
    ERR_SHWSCAMERA_NO_DATASTREAM_FOUND,
    ERR_SHWSCAMERA_BUFFER_LIST_FATAL,
    ERR_SHWSCAMERA_BUFFER_QUEUED_FATAL,
    ERR_SHWSCAMERA_DATASTREAM_START_FATAL,
    ERR_SHWSCAMERA_CAMERA_START_FATAL,
    ERR_SHWSCAMERA_TOO_MANY_ATTEMPTS,
    ERR_SHWSCAMERA_CAPTURE_IMAGE_FATAL,
    ERR_SHWSCAMERA_CAMERA_STOP_FATAL,
    ERR_SHWSCAMERA_DATASTREAM_STOP_FATAL,
    ERR_SHWSCAMERA_RELEASE_BUFFERS_FATAL,

    ERR_SHWSCAMERA_ROI_WOOB,
    ERR_SHWSCAMERA_ROI_HOOB,
    ERR_SHWSCAMERA_SET_WIDTH,
    ERR_SHWSCAMERA_SET_HEIGHT,
    ERR_SHWSCAMERA_SET_OFFSETX,
    ERR_SHWSCAMERA_SET_OFFSETY,
    ERR_SHWSCAMERA_SET_GAIN,
    ERR_SHWSCAMERA_SET_EXPOSURE,
    ERR_SHWSCAMERA_SET_PACKET_SIZE,
    ERR_SHWSCAMERA_SET_PACKET_DELAY,

    ERR_SHWSCAMERA_GET_WIDTH,
    ERR_SHWSCAMERA_GET_HEIGHT,
    ERR_SHWSCAMERA_GET_OFFSETX,
    ERR_SHWSCAMERA_GET_OFFSETY,
    ERR_SHWSCAMERA_GET_GAIN,
    ERR_SHWSCAMERA_GET_EXPOSURE,
    ERR_SHWSCAMERA_GET_PACKET_SIZE,
    ERR_SHWSCAMERA_GET_PACKET_DELAY,

    ERR_SHWSCAMERA_GET_TELEMETRY,
};

enum SHWSCamera_Status{
    SHWSCAMERA_ON = 0,
    SHWSCAMERA_OFF = 1,
    SHWSCAMERA_ERROR = 2,
};

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Telemetry
 ******************************************************************************/
struct SHWSCamera_Telemetry{
    char DeviceVendorName[30];
    char DeviceModelName[10];
    char DeviceManufacturerInfo[50];
    char DeviceVersion[10];
    char DeviceFirmwareVersion[30];
    int DeviceSFNCVersionMajor;
    int DeviceSFNCVersionMinor;
    int DeviceSFNCVersionSubMinor;
    char DeviceUserID[50];
    char DeviceReset[50];
    int SensorWidth;
    int SensorHeight;
    int WidthMax;
    int HeightMax;
    int Width;
    int Height;
    int OffsetX;
    int OffsetY;
    int BinningHorizontal;
    int BinningVertical;
    bool ReverseX;
    bool ReverseY;
    char PixelFormat[20];
    char TestImageSelector[25];
    char AcquisitionMode[50];
    float AcquisitionFrameRate;
    char TriggerSelector[20];
    char TriggerMode[5];
    char TriggerSource[50];
    char TriggerActivation[15];
    char TriggerOverlap[15];
    float TriggerDelay;
    char ExposureMode[20];
    float ExposureTime;
    char LineSelector[50];
    char LineMode[10];
    bool LineInverter;
    bool LineStatus;
    int LineStatusAll;
    char LineSource[20];
    char UserOutputSelector[15];
    bool UserOutputValue;
    int UserOutputValueAll;
    char TimerSelector[10];
    float TimerDuration;
    float TimerDelay;
    char TimerTriggerSource[20];
    char TimerTriggerActivation[15];
    char EventSelector[40];
    char EventNotification[5];
    char GainSelector[15];
    float Gain;
    char BlackLevelSelector[10];
    float BlackLevel;
    float BlackLevelRaw;
    float Gamma;
    char LUTSelector[20];
    bool LUTEnable;
    int LUTIndex;
    int LUTValue;
    int TLParamsLocked;
    int PayloadSize;
    int GevVersionMajor;
    int GevVersionMinor;
    bool GevDeviceModeIsBigEndian;
    char GevDeviceModeCharacterSet[5];
    int GevInterfaceSelector;
    int GevMACAddress;
    char GevSupportedOptionSelector[50];
    bool GevSupportedOption;
    bool GevCurrentIPConfigurationLLA;
    bool GevCurrentIPConfigurationDHCP;
    bool GevCurrentIPConfigurationPersistentIP;
    int GevCurrentIPAddress;
    int GevCurrentSubnetMask;
    int GevCurrentDefaultGateway;
    char GevFirstURL[40];
    char GevSecondURL[20];
    int GevNumberOfInterfaces;
    int GevPersistentIPAddress;
    int GevPersistentSubnetMask;
    int GevPersistentDefaultGateway;
    int GevLinkSpeed;
    int GevMessageChannelCount;
    int GevStreamChannelCount;
    int GevHeartbeatTimeout;
    int GevTimestampTickFrequency;
    int GevTimestampValue;
    bool GevGVCPPendingAck;
    bool GevGVCPHeartbeatDisable;
    int GevGVCPPendingTimeout;
    char GevCCP[35];
    int GevPrimaryApplicationSocket;
    int GevPrimaryApplicationIPAddress;
    int GevMCPHostPort;
    int GevMCDA;
    int GevMCTT;
    int GevMCRC;
    int GevStreamChannelSelector;
    int GevSCPInterfaceIndex;
    int GevSCPHostPort;
    bool GevSCPSFireTestPacket;
    bool GevSCPSDoNotFragment;
    bool GevSCPSBigEndian;
    int GevSCPSPacketSize;
    int GevSCPD;
    int GevSCDA;
    char UserSetSelector[10];
    char UserSetDefaultSelector[10];
    bool ChunkModeActive;
    char ChunkSelector[30];
    bool ChunkEnable;
    int ActionSelector;
    int ActionGroupMask;
    int ActionGroupKey;
    char DeviceID[15];
};

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   22/09/2017
 *
 * Class
 ******************************************************************************/
class SHWSCamera
{
public:
    SHWSCamera_Status status; // Status of connection
    SHWSCamera_Index index; // Index of active sensor

    SHWSCamera(void) {status = SHWSCAMERA_OFF;} // Create object without connecting
    SHWSCamera (SHWSCamera_Index sensorID) {connect(sensorID);} // Create object and connect
    ~SHWSCamera() {disconnect();} // Destruct the object safely
    SHWSCamera_Error connect(SHWSCamera_Index sensorID); // Connect the camera
    SHWSCamera_Error disconnect(void); // Disconnect the camera
    SHWSCamera_Error disconnect(SHWSCamera_Index); // Disconnect the sensor + device
    SHWSCamera_Error reset(void); // Reset the connection to the camera
    SHWSCamera_Error getImage(cv::Mat & img); // Get an image from the camera

    SHWSCamera_Error setTimeout(int timeout_ms); // Set capture timeout
    SHWSCamera_Error setRetryNumber(int retry_max); // Set number for retries when taking an image
    SHWSCamera_Error setROI(int offsetX_px, int offsetY_px, int width_px, int height_px); // Set region of interest
    SHWSCamera_Error setGain(float gain_dB); // Set gain
    SHWSCamera_Error setExposure(int exposure_us); // Set exposure
    SHWSCamera_Error setPacketSize(int Nbytes); // Set size of transmission packet
    SHWSCamera_Error setPacketDelay(int Ntics); // Set delay between transmission packets

    SHWSCamera_Error getTimeout(int & timeout_ms); // Get capture timeout
    SHWSCamera_Error getRetryNumber(int & retry_max); // Get number for retries when taking an image
    SHWSCamera_Error getROI(int & offsetX_px, int & offsetY_px, int & width_px, int & height_px); // Get region of interest
    SHWSCamera_Error getOffsetX(int & offsetX_px); // Get horizontal offset
    SHWSCamera_Error getOffsetY(int & offsetY_px); // Get vertical offset
    SHWSCamera_Error getWidth(int & width_px); // Get width
    SHWSCamera_Error getHeight(int & height_px); // Get height
    SHWSCamera_Error getGain(float & gain_dB); // Get gain
    SHWSCamera_Error getExposure(int & exposure_us); // Get exposure
    SHWSCamera_Error getPacketSize(int & Nbytes); // Get size of transmission packet
    SHWSCamera_Error getPacketDelay(int & Ntics); // Get delay between transmission packets

    SHWSCamera_Error getTelemetry(SHWSCamera_Telemetry & telemetry); // Gat all the telemetry from the camera

private:
    int _timeout;
    int _retry_max;

    BGAPI2::System * pSystem = NULL;
    BGAPI2::Interface * pInterface = NULL;
    BGAPI2::Device * pDevice = NULL;

    BGAPI2::DeviceList *deviceList = NULL;
};

#endif
