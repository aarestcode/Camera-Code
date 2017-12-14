/***************************************************************************//**
 * @file	ImagingCamera.hpp
 * @brief	Header file to operate the XIMEA cameras
 *
 * This header file contains all the required definitions and function prototypes
 * through which to control the AAReST XIMEA cameras
 *
 * @author	Thibaud Talon
 * @date	21/09/2017
 *******************************************************************************/

#ifndef IMAGING_CAMERA_H
#define IMAGING_CAMERA_H

#include <m3api/xiApi.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Parameters
 ******************************************************************************/
#ifndef OK
#define OK 0
#endif

enum ImagingCamera_Index{
    IMAGINGCAMERA_SCIENCE_CAMERA = 0,
    IMAGINGCAMERA_BOOM_INSPECTION_CAMERA = 1
};

enum ImagingCamera_Error{
    OK_IMAGINGCAMERA = 0,
    ERR_IMAGINGCAMERA_CANNOT_DETECT,
    ERR_IMAGINGCAMERA_NO_DETECT,
    ERR_IMAGINGCAMERA_CANNOT_OPEN,
    ERR_IMAGINGCAMERA_SET_SHUTTER,
    ERR_IMAGINGCAMERA_START_ACQUISITION,
    ERR_IMAGINGCAMERA_GET_IMAGE,
    ERR_IMAGINGCAMERA_CONNECT_FATAL,
    ERR_IMAGINGCAMERA_DISCONNECT_FATAL,
    ERR_IMAGINGCAMERA_NO_DEVICE,
    ERR_IMAGINGCAMERA_GETIMAGE_FATAL,
    ERR_IMAGINGCAMERA_NO_VIDEO,
    ERR_IMAGINGCAMERA_ENABLE_TRIGGER,
    ERR_IMAGINGCAMERA_TRIGGER,
    ERR_IMAGINGCAMERA_VIDEO_FRAMERATE,
    ERR_IMAGINGCAMERA_DISABLE_TRIGGER,
    ERR_IMAGINGCAMERA_GETVIDEO_FATAL,
    ERR_IMAGINGCAMERA_ROI_WOOB,
    ERR_IMAGINGCAMERA_ROI_HOOB,
    ERR_IMAGINGCAMERA_SET_OFFSETX,
    ERR_IMAGINGCAMERA_SET_OFFSETY,
    ERR_IMAGINGCAMERA_SET_WIDTH,
    ERR_IMAGINGCAMERA_SET_HEIGHT,
    ERR_IMAGINGCAMERA_SET_ROI_FATAL,
    ERR_IMAGINGCAMERA_SET_GAIN,
    ERR_IMAGINGCAMERA_SET_GAIN_FATAL,
    ERR_IMAGINGCAMERA_SET_EXPOSURE,
    ERR_IMAGINGCAMERA_SET_EXPOSURE_FATAL,
    ERR_IMAGINGCAMERA_GET_WIDTH,
    ERR_IMAGINGCAMERA_GET_HEIGHT,
    ERR_IMAGINGCAMERA_GET_OFFSETX,
    ERR_IMAGINGCAMERA_GET_OFFSETY,
    ERR_IMAGINGCAMERA_GET_ROI_FATAL,
    ERR_IMAGINGCAMERA_GET_OFFSETX_FATAL,
    ERR_IMAGINGCAMERA_GET_OFFSETY_FATAL,
    ERR_IMAGINGCAMERA_GET_WIDTH_FATAL,
    ERR_IMAGINGCAMERA_GET_HEIGHT_FATAL,
    ERR_IMAGINGCAMERA_GET_GAIN,
    ERR_IMAGINGCAMERA_GET_GAIN_FATAL,
    ERR_IMAGINGCAMERA_GET_EXPOSURE,
    ERR_IMAGINGCAMERA_GET_EXPOSURE_FATAL,
    ERR_IMAGINGCAMERA_GET_TELEMETRY,
    ERR_IMAGINGCAMERA_GET_TELEMETRY_FATAL
};

enum ImagingCamera_Status{
    IMAGINGCAMERA_ON = 0,
    IMAGINGCAMERA_OFF = 1,
    IMAGINGCAMERA_ERROR = 2,
};

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Telemetry
 ******************************************************************************/
struct ImagingCamera_Telemetry{
    char device_name[20];
    char device_inst_path[30];
    char device_loc_path[30];
    char device_type[20];
    int device_model_id;
    char device_sn[20];
    int debug_level;
    int auto_bandwidth_calculation;
    int new_process_chain_enable;
    int exposure;
    float gain;
    int downsampling;
    int downsampling_type;
    int shutter_type;
    int imgdataformat;
    int imgdataformatrgb32alpha;
    int imgpayloadsize;
    int transport_pixel_format;
    float framerate;
    int buffer_policy;
    int counter_selector;
    int counter_value;
    int offsetX;
    int offsetY;
    int width;
    int height;
    int trigger_source;
    int trigger_software;
    int trigger_delay;
    int available_bandwidth;
    int limit_bandwidth;
    float sensor_clock_freq_hz;
    int sensor_clock_freq_index;
    int sensor_bit_depth;
    int output_bit_depth;
    int image_data_bit_depth;
    int output_bit_packing;
    int acq_timing_mode;
    int trigger_selector;
    float wb_kr;
    float wb_kg;
    float wb_kb;
    int auto_wb;
    float gammaY;
    float gammaC;
    float sharpness;
    float ccMTX00;
    float ccMTX01;
    float ccMTX02;
    float ccMTX03;
    float ccMTX10;
    float ccMTX11;
    float ccMTX12;
    float ccMTX13;
    float ccMTX20;
    float ccMTX21;
    float ccMTX22;
    float ccMTX23;
    float ccMTX30;
    float ccMTX31;
    float ccMTX32;
    float ccMTX33;
    int iscolor;
    int cfa;
    int cms;
    int apply_cms;
    char input_cms_profile[50];
    char output_cms_profile[50];
    int gpi_selector;
    int gpi_mode;
    int gpi_level;
    int gpo_selector;
    int gpo_mode;
    int acq_buffer_size;
    int acq_transport_buffer_size;
    int buffers_queue_size;
    int acq_transport_buffer_commit;
    int recent_frame;
    int device_reset;
    int aeag;
    int ae_max_limit;
    float ag_max_limit;
    float exp_priority;
    float aeag_level;
    int aeag_roi_offset_x;
    int aeag_roi_offset_y;
    int dbnc_en;
    int dbnc_t0;
    int dbnc_t1;
    int dbnc_pol;
    int iscooled;
    int cooling;
    float target_temp;
    int isexist;
    int bpc;
    int column_fpn_correction;
    int sensor_mode;
    int image_black_level;
    char api_version[20];
    char drv_version[20];
    char version_mcu1[20];
    char version_fpga1[20];
};

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Class
 ******************************************************************************/
class ImagingCamera
{
public:
    ImagingCamera_Index index; // Index of camera
    ImagingCamera_Status status; // Status of connection
    XI_RETURN error; // Error from XIMEA API

    ImagingCamera(void) {status = IMAGINGCAMERA_OFF;} // Create object without connecting
    ImagingCamera (ImagingCamera_Index cameraID) {connect(cameraID);} // Create object and connect
    ~ImagingCamera() {disconnect();} // Destruct the object safely

    ImagingCamera_Error connect(ImagingCamera_Index cameraID); // Connect the camera
    ImagingCamera_Error disconnect(void); // Disconnect the camera
    ImagingCamera_Error reset(void); // Reset the connection to the camera

    ImagingCamera_Error getImage(cv::Mat & img); // Get an image from the camera
    ImagingCamera_Error getVideo(cv::VideoWriter & video, float fps, float duration_s); // Get an video from the camera

    ImagingCamera_Error setTimeout(int timeout_ms); // Set capture timeout
    ImagingCamera_Error setROI(int offsetX_px, int offsetY_px, int width_px, int height_px); // Set region of interest
    ImagingCamera_Error setGain(float gain_dB); // Set gain
    ImagingCamera_Error setExposure(int exposure_us); // Set exposure

    ImagingCamera_Error getTimeout(int & timeout_ms); // Get capture timeout
    ImagingCamera_Error getROI(int & offsetX_px, int & offsetY_px, int & width_px, int & height_px); // Get region of interest
    ImagingCamera_Error getOffsetX(int & offsetX_px); // Get horizontal offset
    ImagingCamera_Error getOffsetY(int & offsetY_px); // Get vertical offset
    ImagingCamera_Error getWidth(int & width_px); // Get width
    ImagingCamera_Error getHeight(int & height_px); // Get height
    ImagingCamera_Error getGain(float & gain_dB); // Get gain
    ImagingCamera_Error getExposure(int & exposure_us); // Get exposure

    ImagingCamera_Error getTelemetry(ImagingCamera_Telemetry & telemetry); // Gat all the telemetry from the camera

private:
    HANDLE handle;
    int _timeout;
};


#endif
