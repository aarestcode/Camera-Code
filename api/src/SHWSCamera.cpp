/***************************************************************************//**
 * @file	SHWSCamera.cpp
 * @brief	Source file to operate the Shack–Hartmann wavefront sensor
 *
 * This file contains all the implementations for the functions defined in:
 * api/include/SHWSCamera.hpp
 *
 * @author	Thibaud Talon
 * @date	23/09/2017
 *******************************************************************************/

#include <opencv2/core/core.hpp>
#include <stdio.h>
#include "bgapi2_genicam.hpp"
#include "SHWSCamera.hpp"
#include "UserInterface.hpp"

using namespace BGAPI2;

#define SHWSCAMERA_MAX_WIDTH 2040
#define SHWSCAMERA_MAX_HEIGHT 2044

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   23/09/2017
 *
 * Connect the camera & sensor
 *
 * @param [in] sensorID
 *	index of the sensor (SHWSCAMERA_mY_APERTURE or SHWSCAMERA_pY_APERTURE)
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::connect(SHWSCamera_Index sensorID){
    UserInterface::Log log("SHWSCamera::open");

    if(pDevice==NULL) { // No camera = full connection
        // TODO: switch to the right sensor

        status = SHWSCAMERA_OFF;

        BGAPI2::SystemList *systemList = NULL;
        BGAPI2::String sSystemID;

        BGAPI2::InterfaceList *interfaceList = NULL;
        BGAPI2::String sInterfaceID;

        // Counting number of availabale systems (TL producers)
        try{
            systemList = SystemList::GetInstance();
            systemList->Refresh();
            log.printf("1. Number of available systems = %i",systemList->size());
        }
        catch (BGAPI2::Exceptions::IException& ex){
            log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SYSTEM_LIST_FATAL);
        }

        // Open first system in the list with a camera connected
        try{
            for (SystemList::iterator sysIterator = systemList->begin(); sysIterator != systemList->end(); sysIterator++){
                try{
                    sysIterator->second->Open();
                    log.printf("2. Open system with name = %s",(char *)sysIterator->second->GetFileName());
                    sSystemID = sysIterator->first;

                    // 3. Count available interfaces
                    try{
                        interfaceList = sysIterator->second->GetInterfaces();
                        interfaceList->Refresh(100); // timeout of 100 msec
                        log.printf("3. Number of detected interfaces = %i",interfaceList->size());
                    }
                    catch (BGAPI2::Exceptions::IException& ex){
                        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_INTERFACE_LIST_FATAL);
                    }

                    // 4. Open the next interface in the list
                    try{
                        for (InterfaceList::iterator ifIterator = interfaceList->begin(); ifIterator != interfaceList->end(); ifIterator++){
                            try{
                                log.printf("4. Open interface with name = %s",(char *)ifIterator->second->GetDisplayName());
                                ifIterator->second->Open();

                                // 5. Search for any camera is connected to this interface
                                deviceList = ifIterator->second->GetDevices();
                                deviceList->Refresh(100);
                                log.printf("5. Number of connected camera = %i",deviceList->size());
                                if(deviceList->size() == 0){
                                    log.printf("6. Close interface");
                                    ifIterator->second->Close();
                                }
                                else{
                                    sInterfaceID = ifIterator->first;
                                    log.printf("6. Interface type = %s",(char *)ifIterator->second->GetTLType());
                                    if( ifIterator->second->GetTLType() == "GEV" ){
                                        unsigned int IPAddress = ifIterator->second->GetNode("GevInterfaceSubnetIPAddress")->GetInt();
                                        log.printf("7. GevInterfaceSubnetIPAddress = %i.%i.%i.%i", (IPAddress>>8*3)&0xff, (IPAddress>>8*2)&0xff, (IPAddress>>8)&0xff, (IPAddress)&0xff);
                                        unsigned int SubnetMask = ifIterator->second->GetNode("GevInterfaceSubnetMask")->GetInt();
                                        log.printf("8. GevInterfaceSubnetMask = %i.%i.%i.%i", (SubnetMask>>8*3)&0xff, (SubnetMask>>8*2)&0xff, (SubnetMask>>8)&0xff, (SubnetMask)&0xff);
                                    }
                                    break;
                                }
                            }
                            catch (BGAPI2::Exceptions::ResourceInUseException& ex){
                                log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_INTERFACE_ALREADY_OPENED);
                            }
                        }
                    }
                    catch (BGAPI2::Exceptions::IException& ex){
                        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_INTERFACE_NEXT_FATAL);
                    }

                    //if a camera is connected to the system interface then leave the system loop
                    if(sInterfaceID != ""){
                        break;
                    }
                }
                catch (BGAPI2::Exceptions::ResourceInUseException& ex){
                    log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SYSTEM_ALREADY_OPENED);
                }
            }
        }
        catch (BGAPI2::Exceptions::IException& ex){
            log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_INTERFACE_FATAL);
        }

        if ( sSystemID == "" ){
            disconnect();
            return (SHWSCamera_Error) log.error("No System found",ERR_SHWSCAMERA_NO_SYSTEM_FOUND);
        }
        else{
            pSystem = (*systemList)[sSystemID];
        }

        if (sInterfaceID == ""){
            disconnect();
            return (SHWSCamera_Error) log.error("No camera found",ERR_SHWSCAMERA_NO_CAMERA_FOUND);
        }
        else{
            pInterface = (*interfaceList)[sInterfaceID];
        }

    }
    else{ // Camera already connected

        if (sensorID == index) return (SHWSCamera_Error) log.success();

        // Disconnect the device
        disconnect(sensorID);

        // TODO: switch to the right sensor

    }

    // Open the first camera in the list
    BGAPI2::String sDeviceID;
    try{
        deviceList = pInterface->GetDevices();
        deviceList->Refresh(100);

        for (DeviceList::iterator devIterator = deviceList->begin(); devIterator != deviceList->end(); devIterator++){
            try{
                log.printf("9. Open device with DeviceID = %s",(char *)devIterator->first);
                devIterator->second->Open();
                sDeviceID = devIterator->first;

                if(devIterator->second->GetTLType() == "GEV"){
                    unsigned int IPAddress = devIterator->second->GetRemoteNode("GevCurrentIPAddress")->GetInt();
                    log.printf("11. GevCurrentIPAddress = %i.%i.%i.%i", (IPAddress>>8*3)&0xff, (IPAddress>>8*2)&0xff, (IPAddress>>8)&0xff, (IPAddress)&0xff);
                    unsigned int SubnetMask = devIterator->second->GetRemoteNode("GevCurrentSubnetMask")->GetInt();
                    log.printf("12. GevCurrentSubnetMask = %i.%i.%i.%i", (SubnetMask>>8*3)&0xff, (SubnetMask>>8*2)&0xff, (SubnetMask>>8)&0xff, (SubnetMask)&0xff);
                }
                break;
            }
            catch (BGAPI2::Exceptions::ResourceInUseException& ex){
                log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_DEVICE_ALREADY_OPENED);
            }
            catch (BGAPI2::Exceptions::AccessDeniedException& ex){
                log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_DEVICE_ACCESS_DENIED);
            }
        }
    }
    catch (BGAPI2::Exceptions::IException& ex){
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_OPEN_DEVICE_FATAL);
    }

    if (sDeviceID == ""){
        disconnect();
        return (SHWSCamera_Error) log.error("No Device found",ERR_SHWSCAMERA_NO_DEVICE_FOUND);
    }
    else{
        pDevice = (*deviceList)[sDeviceID];
    }

    status = SHWSCAMERA_ON;

    // Set trigger mode off (FreeRun)
    try{
        pDevice->GetRemoteNode("TriggerMode")->SetString("Off");
        log.printf("13. Set trigger mode OFF = %s", (char *)pDevice->GetRemoteNode("TriggerMode")->GetValue());
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_TRIGGER_MODE_FATAL);
    }

    // Set packet delay to 50000 tics
    try{
        pDevice->GetRemoteNode("GevSCPD")->SetInt(50000);
        log.printf("14. Set Packet delay = %i tics", pDevice->GetRemoteNode("GevSCPD")->GetInt());
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_PACKET_DELAY_FATAL);
    }

    // Set capture timeout to 1s
    log.printf("15. Timeout = %i ms", 1000);
    _timeout = 1000;

    // Set maximum number of retries if image is incomplete or timeout occurs to 10
    log.printf("16. Maximum # of retries = %i", 10);
    _retry_max = 10;

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   23/09/2017
 *
 * Disconnect the camera
 *
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::disconnect(void){
    UserInterface::Log log("SHWSCamera::disconnect");

    try{
        log.printf("Closing the connection");
        if(pDevice){
            pDevice->Close();
            pDevice = NULL;
        }
        if(pInterface){
            pInterface->Close();
            pInterface = NULL;
        }
        if(pSystem){
            pSystem->Close();
            pSystem = NULL;
        }
        BGAPI2::SystemList::ReleaseInstance();
        status = SHWSCAMERA_OFF;

        return (SHWSCamera_Error) log.success();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_DISCONNECT_FATAL);
    }
}
SHWSCamera_Error SHWSCamera::disconnect(SHWSCamera_Index){
    UserInterface::Log log("SHWSCamera::disconnect");

    try{
        log.printf("Closing the connection");
        if(pDevice){
            pDevice->Close();
            pDevice = NULL;
        }
        // TODO: Other things?

        return (SHWSCamera_Error) (SHWSCamera_Error) log.success();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_DISCONNECT_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   23/09/2017
 *
 * Reset the connection to the camera
 *
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::reset(void){
    disconnect();
    return connect(index);
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   23/09/2017
 *
 * Get an image from the camera
 *
 * @param [out] img
 *	OpenCV image corresponding to the frame retrieved from the camera
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::getImage(cv::Mat & img){
    UserInterface::Log log("SHWSCamera::getImage");

    SHWSCamera_Error error = OK_SHWSCAMERA;

    // 1. Check inputs
    log.printf("1. Check inputs");
    img.release();
    if(pDevice==NULL) {return (SHWSCamera_Error) log.error("No opened device",ERR_SHWSCAMERA_NO_DEVICE);}

    BGAPI2::DataStreamList *datastreamList = NULL;
    BGAPI2::DataStream * pDataStream = NULL;
    BGAPI2::String sDataStreamID;

    BGAPI2::BufferList *bufferList = NULL;
    BGAPI2::Buffer * pBuffer = NULL;

    //COUNTING AVAILABLE DATASTREAMS
    try{
        datastreamList = pDevice->GetDataStreams();
        datastreamList->Refresh();
        log.printf("2. Detected datastreams = %i",datastreamList->size());
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_DATASTREAM_LIST_FATAL);
    }

    //OPEN THE FIRST DATASTREAM IN THE LIST
    try{
        for (DataStreamList::iterator dstIterator = datastreamList->begin(); dstIterator != datastreamList->end(); dstIterator++){

            log.printf("3. Open first datastream with ID = %s",(char*)dstIterator->first);
            dstIterator->second->Open();
            sDataStreamID = dstIterator->first;
            break;
        }
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_DATASTREAM_OPEN_FATAL);
    }

    if (sDataStreamID == ""){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error("No DataStream found", ERR_SHWSCAMERA_NO_DATASTREAM_FOUND);
    }
    else{
        pDataStream = (*datastreamList)[sDataStreamID];
    }

    //BUFFER LIST
    try{
        //BufferList
        bufferList = pDataStream->GetBufferList();

        // 4 buffers using internal buffer mode
        for(int i=0; i<4; i++){
            pBuffer = new BGAPI2::Buffer();
            bufferList->Add(pBuffer);
        }
        log.printf("4. Announced buffers = %i",bufferList->GetAnnouncedCount());
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_BUFFER_LIST_FATAL);
        error = ERR_SHWSCAMERA_BUFFER_LIST_FATAL;
    }

    try{
        for (BufferList::iterator bufIterator = bufferList->begin(); bufIterator != bufferList->end(); bufIterator++){
            bufIterator->second->QueueBuffer();
        }
        log.printf("5. Queued buffers = %i",bufferList->GetQueuedCount());
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_BUFFER_QUEUED_FATAL);
        error = ERR_SHWSCAMERA_BUFFER_QUEUED_FATAL;
    }

    //START DataStream acquisition
    try{
        pDataStream->StartAcquisitionContinuous();
        log.printf("6. DataStream started");
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_DATASTREAM_START_FATAL);
        error = ERR_SHWSCAMERA_DATASTREAM_START_FATAL;
    }

    //START CAMERA
    try{
        pDevice->GetRemoteNode("AcquisitionStart")->Execute();
        log.printf("7. Start camera = %s",(char*)pDevice->GetModel());
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_CAMERA_START_FATAL);
        error = ERR_SHWSCAMERA_CAMERA_START_FATAL;
    }

    //CAPTURE 1 IMAGES
    BGAPI2::Buffer * pBufferFilled = NULL;
    try{
        bool imgTaken = false;
        int count = 0;
        while(!imgTaken && (count++ < _retry_max)){
            pBufferFilled = pDataStream->GetFilledBuffer(_timeout);
            if(pBufferFilled == NULL){
                log.printf("8. Error: Buffer Timeout after %i msec", _timeout);;
            }
            else if(pBufferFilled->GetIsIncomplete() == true){
                log.printf("8. Error: Image is incomplete");
                // queue buffer again
                pBufferFilled->QueueBuffer();
            }
            else{
                log.printf("8. Image taken with ID = %i",pBufferFilled->GetFrameID());
                img = cv::Mat(pBufferFilled->GetHeight(),pBufferFilled->GetWidth(),CV_8UC1,pBufferFilled->GetMemPtr() );
                imgTaken = true;
            }
        }
        if (count == _retry_max){
            status = SHWSCAMERA_ERROR;
            error = (SHWSCamera_Error) log.error("Too many attempts",ERR_SHWSCAMERA_TOO_MANY_ATTEMPTS);
        }
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_CAPTURE_IMAGE_FATAL);
        error = ERR_SHWSCAMERA_CAPTURE_IMAGE_FATAL;
    }

    //STOP CAMERA
    try{
        //SEARCH FOR 'AcquisitionAbort'
        if(pDevice->GetRemoteNodeList()->GetNodePresent("AcquisitionAbort")){
            pDevice->GetRemoteNode("AcquisitionAbort")->Execute();
            log.printf("9. Abort device = %s",(char*)pDevice->GetModel());
        }

        pDevice->GetRemoteNode("AcquisitionStop")->Execute();
        log.printf("9. Stop device = %s",(char*)pDevice->GetModel());
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_CAMERA_STOP_FATAL);
        error = ERR_SHWSCAMERA_CAMERA_STOP_FATAL;
    }

    //STOP DataStream acquisition
    try{
        if( pDataStream->GetTLType() == "GEV" ){
            log.printf("10. DataStream Statistic: GoodFrames = %i",pDataStream->GetNodeList()->GetNode("GoodFrames")->GetInt());
            log.printf("11. DataStream Statistic: CorruptedFrames = %i",pDataStream->GetNodeList()->GetNode("CorruptedFrames")->GetInt());
            log.printf("12. DataStream Statistic: LostFrames = %i",pDataStream->GetNodeList()->GetNode("LostFrames")->GetInt());
            log.printf("13. DataStream Statistic: ResendRequests = %i",pDataStream->GetNodeList()->GetNode("ResendRequests")->GetInt());
            log.printf("14. DataStream Statistic: ResendPackets = %i",pDataStream->GetNodeList()->GetNode("ResendPackets")->GetInt());
            log.printf("15. DataStream Statistic: LostPackets = %i",pDataStream->GetNodeList()->GetNode("LostPackets")->GetInt());
            log.printf("16. DataStream Statistic: Bandwidth = %i",pDataStream->GetNodeList()->GetNode("Bandwidth")->GetInt());
        }

        //BufferList Information
        log.printf("17. BufferList Information: DeliveredCount = %i",bufferList->GetDeliveredCount());
        log.printf("18. BufferList Information: UnderrunCount = %i",bufferList->GetUnderrunCount());

        pDataStream->StopAcquisition();
        log.printf("19. DataStream stopped");
        bufferList->DiscardAllBuffers();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_DATASTREAM_STOP_FATAL);
        error = ERR_SHWSCAMERA_DATASTREAM_STOP_FATAL;
    }

    //Release buffers
    try{
        while( bufferList->size() > 0){
            pBuffer = bufferList->begin()->second;
            bufferList->RevokeBuffer(pBuffer);
            delete pBuffer;
        }
        log.printf("20. Buffers after revoke = %i",bufferList->size());
        pDataStream->Close();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_RELEASE_BUFFERS_FATAL);
        error = ERR_SHWSCAMERA_RELEASE_BUFFERS_FATAL;
    }

    if(error) return (SHWSCamera_Error) log.error("Error getting an image",error);
    else return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   23/09/2017
 *
 * Set capture timeout
 *
 * @param [in] timeout_ms
 *	Capture timeout in ms
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::setTimeout(int timeout_ms){
    UserInterface::Log log("SHWSCamera::setTimeout");

    log.printf("Change timeout to %i ms", timeout_ms);
    _timeout = timeout_ms;

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   23/09/2017
 *
 * Set maximum number of capture retries due to incomplete image or timeout
 *
 * @param [in] retry_max
 *	Number of retries
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::setRetryNumber(int retry_max){
    UserInterface::Log log("SHWSCamera::setRetryNumber");

    log.printf("Change number of capture retries to %i", retry_max);
    _retry_max = retry_max;

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   23/09/2017
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
SHWSCamera_Error SHWSCamera::setROI(int offsetX_px, int offsetY_px, int width_px, int height_px){
    UserInterface::Log log("SHWSCamera::setGain");

    if(pDevice==NULL) {return (SHWSCamera_Error) log.error("No opened device",ERR_SHWSCAMERA_NO_DEVICE);}

    // 1. Check inputs
    log.printf("1. Check inputs");
    if ( offsetX_px + width_px > SHWSCAMERA_MAX_WIDTH ) {width_px = SHWSCAMERA_MAX_WIDTH - offsetX_px; log.error("ROI right limit out of bounds", ERR_SHWSCAMERA_ROI_WOOB);}
    if ( offsetY_px + height_px > SHWSCAMERA_MAX_HEIGHT ) {height_px = SHWSCAMERA_MAX_HEIGHT - offsetY_px; log.error("ROI bottom limit out of bounds", ERR_SHWSCAMERA_ROI_HOOB);}

    // 2. Get current width and height
    log.printf("2. Get current width and height");
    int current_width, current_height;
    try{
        current_width = pDevice->GetRemoteNode("Width")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_WIDTH);
    }
    try{
        current_height = pDevice->GetRemoteNode("Height")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_HEIGHT);
    }

    if ( offsetX_px + current_width > SHWSCAMERA_MAX_WIDTH){
        // 3. Change image width to %i px
        try{
            pDevice->GetRemoteNode("Width")->SetInt(width_px);
        }
        catch (BGAPI2::Exceptions::IException& ex){
            status = SHWSCAMERA_ERROR;
            return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_WIDTH);
        }
        try{
            width_px = pDevice->GetRemoteNode("Width")->GetInt();
        }
        catch (BGAPI2::Exceptions::IException& ex){
            status = SHWSCAMERA_ERROR;
            return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_WIDTH);
        }
        log.printf("3. ROI width set to %i px", width_px);


        // 4. Change horizontal offset to %i px
        try{
            pDevice->GetRemoteNode("OffsetX")->SetInt(offsetX_px);
        }
        catch (BGAPI2::Exceptions::IException& ex){
            status = SHWSCAMERA_ERROR;
            return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_OFFSETX);
        }
        try{
            offsetX_px = pDevice->GetRemoteNode("OffsetX")->GetInt();
        }
        catch (BGAPI2::Exceptions::IException& ex){
            status = SHWSCAMERA_ERROR;
            return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_OFFSETX);
        }
        log.printf("4. ROI horizontal offset set to %i px", offsetX_px);
    }
    else{
        // 3. Change horizontal offset to %i px
        try{
            pDevice->GetRemoteNode("OffsetX")->SetInt(offsetX_px);
        }
        catch (BGAPI2::Exceptions::IException& ex){
            status = SHWSCAMERA_ERROR;
            return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_OFFSETX);
        }
        try{
            offsetX_px = pDevice->GetRemoteNode("OffsetX")->GetInt();
        }
        catch (BGAPI2::Exceptions::IException& ex){
            status = SHWSCAMERA_ERROR;
            return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_OFFSETX);
        }
        log.printf("3. ROI horizontal offset set to %i px", offsetX_px);

        // 4. Change image width to %i px
        try{
            pDevice->GetRemoteNode("Width")->SetInt(width_px);
        }
        catch (BGAPI2::Exceptions::IException& ex){
            status = SHWSCAMERA_ERROR;
            return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_WIDTH);
        }
        try{
            width_px = pDevice->GetRemoteNode("Width")->GetInt();
        }
        catch (BGAPI2::Exceptions::IException& ex){
            status = SHWSCAMERA_ERROR;
            return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_WIDTH);
        }
        log.printf("3. ROI width set to %i px", width_px);
    }

    if ( offsetY_px + current_height > SHWSCAMERA_MAX_HEIGHT){
        // 5. Change image height to %i px
        try{
            pDevice->GetRemoteNode("Height")->SetInt(height_px);
        }
        catch (BGAPI2::Exceptions::IException& ex){
            status = SHWSCAMERA_ERROR;
            return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_HEIGHT);
        }
        try{
            height_px = pDevice->GetRemoteNode("Height")->GetInt();
        }
        catch (BGAPI2::Exceptions::IException& ex){
            status = SHWSCAMERA_ERROR;
            return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_HEIGHT);
        }
        log.printf("5. ROI height set to %i px", height_px);

        // 6. Change vertical offset to %i px
        try{
            pDevice->GetRemoteNode("OffsetY")->SetInt(offsetY_px);
        }
        catch (BGAPI2::Exceptions::IException& ex){
            status = SHWSCAMERA_ERROR;
            return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_OFFSETY);
        }
        try{
            offsetY_px = pDevice->GetRemoteNode("OffsetY")->GetInt();
        }
        catch (BGAPI2::Exceptions::IException& ex){
            status = SHWSCAMERA_ERROR;
            return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_OFFSETY);
        }
        log.printf("6. Roi vertical offset set to %i px", offsetY_px);
    }
    else{
        // 5. Change vertical offset to %i px
        try{
            pDevice->GetRemoteNode("OffsetY")->SetInt(offsetY_px);
        }
        catch (BGAPI2::Exceptions::IException& ex){
            status = SHWSCAMERA_ERROR;
            return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_OFFSETY);
        }
        try{
            offsetY_px = pDevice->GetRemoteNode("OffsetY")->GetInt();
        }
        catch (BGAPI2::Exceptions::IException& ex){
            status = SHWSCAMERA_ERROR;
            return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_OFFSETY);
        }
        log.printf("5. ROI vertical offset set to %i px", offsetY_px);


        // 6. Change image height to %i px
        try{
            pDevice->GetRemoteNode("Height")->SetInt(height_px);
        }
        catch (BGAPI2::Exceptions::IException& ex){
            status = SHWSCAMERA_ERROR;
            return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_HEIGHT);
        }
        try{
            height_px = pDevice->GetRemoteNode("Height")->GetInt();
        }
        catch (BGAPI2::Exceptions::IException& ex){
            status = SHWSCAMERA_ERROR;
            return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_HEIGHT);
        }
        log.printf("6. ROI height set to %i px", height_px);
    }

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   23/09/2017
 *
 * Set gain
 *
 * @param [in] gain_dB
 *	Gain in dB
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::setGain(float gain_dB){
    UserInterface::Log log("SHWSCamera::setGain");

    if(pDevice==NULL) {return (SHWSCamera_Error) log.error("No opened device",ERR_SHWSCAMERA_NO_DEVICE);}

    try{
        pDevice->GetRemoteNode("Gain")->SetDouble(gain_dB);
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_GAIN);
    }
    try{
        gain_dB = pDevice->GetRemoteNode("Gain")->GetDouble();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_GAIN);
    }
    log.printf("Gain changed to %f", gain_dB);

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   23/09/2017
 *
 * Set exposure
 *
 * @param [in] exposure_us
 *	Exposure in us
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::setExposure(int exposure_us){
    UserInterface::Log log("SHWSCamera::setExposure");

    if(pDevice==NULL) {return (SHWSCamera_Error) log.error("No opened device",ERR_SHWSCAMERA_NO_DEVICE);}

    try{
        pDevice->GetRemoteNode("ExposureTime")->SetDouble((double)exposure_us);
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_EXPOSURE);
    }
    try{
        exposure_us = (int)pDevice->GetRemoteNode("ExposureTime")->GetDouble();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_EXPOSURE);
    }
    log.printf("Exposure set to %i us", exposure_us);

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   23/09/2017
 *
 * Set size of packets of Ethernet
 *
 * @param [in] Nbytes
 *	Size of packets in bytes
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::setPacketSize(int Nbytes){
    UserInterface::Log log("SHWSCamera::setPacketSize");

    if(pDevice==NULL) {return (SHWSCamera_Error) log.error("No opened device",ERR_SHWSCAMERA_NO_DEVICE);}

    try{
        pDevice->GetRemoteNode("GevSCPSPacketSize")->SetInt(Nbytes);
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_PACKET_SIZE);
    }
    try{
        Nbytes = pDevice->GetRemoteNode("GevSCPSPacketSize")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_PACKET_SIZE);
    }
    log.printf("Packet size set to %i bytes", Nbytes);

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   23/09/2017
 *
 * Set delay between each pack transmitted over ethernet
 *
 * @param [in] Ntics
 *	Number of clock tics between each packet
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::setPacketDelay(int Ntics){
    UserInterface::Log log("SHWSCamera::setPacketDelay");

    if(pDevice==NULL) {return (SHWSCamera_Error) log.error("No opened device",ERR_SHWSCAMERA_NO_DEVICE);}

    try{
        pDevice->GetRemoteNode("GevSCPD")->SetInt(Ntics);
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_PACKET_DELAY);
    }
    try{
        Ntics = pDevice->GetRemoteNode("GevSCPD")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_PACKET_DELAY);
    }
    log.printf("Packet delay set to %i tics", Ntics);

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   23/09/2017
 *
 * Get timeout
 *
 * @param [out] timeout_ms
 *	Capture timeout in ms
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::getTimeout(int & timeout_ms){
    UserInterface::Log log("SHWSCamera::getTimeout");

    timeout_ms = _timeout;
    log.printf("Timeout = %i ms", timeout_ms);

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   23/09/2017
 *
 * Get maximum number of capture retries due to incomplete image or timeout
 *
 * @param [in] retry_max
 *	Number of retries
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::getRetryNumber(int & retry_max){
    UserInterface::Log log("SHWSCamera::getRetryNumber");

    retry_max = _retry_max;
    log.printf("Number of capture retries = %i", retry_max);

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   24/09/2017
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
SHWSCamera_Error SHWSCamera::getROI(int & offsetX_px, int & offsetY_px, int & width_px, int & height_px){
    UserInterface::Log log("SHWSCamera::getROI");

    if(pDevice==NULL) {return (SHWSCamera_Error) log.error("No opened device",ERR_SHWSCAMERA_NO_DEVICE);}

    try{
        offsetX_px = pDevice->GetRemoteNode("OffsetX")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_OFFSETX);
    }
    log.printf("1. ROI horizontal offset = %i px", offsetX_px);

    try{
        offsetY_px = pDevice->GetRemoteNode("OffsetY")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_OFFSETY);
    }
    log.printf("2. ROI vertical offset = %i px", offsetY_px);

    try{
        width_px = pDevice->GetRemoteNode("Width")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_WIDTH);
    }
    log.printf("3. ROI width = %i px", width_px);

    try{
        height_px = pDevice->GetRemoteNode("Height")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_HEIGHT);
    }
    log.printf("4. ROI height = %i px", height_px);

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   24/09/2017
 *
 * Get horizontal offset of ROI
 *
 * @param [out] offsetX_px
 *	Horizontal offset in pixels
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::getOffsetX(int & offsetX_px){
    UserInterface::Log log("SHWSCamera::getOffsetX");

    if(pDevice==NULL) {return (SHWSCamera_Error) log.error("No opened device",ERR_SHWSCAMERA_NO_DEVICE);}

    try{
        offsetX_px = pDevice->GetRemoteNode("OffsetX")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_OFFSETX);
    }
    log.printf("ROI horizontal offset = %i px", offsetX_px);

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   24/09/2017
 *
 * Get vertical offset of ROI
 *
 * @param [out] offsetY_px
 *	Vertical offset in pixels
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::getOffsetY(int & offsetY_px){
    UserInterface::Log log("SHWSCamera::getOffsetY");

    if(pDevice==NULL) {return (SHWSCamera_Error) log.error("No opened device",ERR_SHWSCAMERA_NO_DEVICE);}

    try{
        offsetY_px = pDevice->GetRemoteNode("OffsetY")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_SET_OFFSETY);
    }
    log.printf("ROI vertical offset = %i px", offsetY_px);

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   24/09/2017
 *
 * Get width of ROI
 *
 * @param [out] width_px
 *	Width of the image in pixels
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::getWidth(int & width_px){
    UserInterface::Log log("SHWSCamera::getWidth");

    if(pDevice==NULL) {return (SHWSCamera_Error) log.error("No opened device",ERR_SHWSCAMERA_NO_DEVICE);}

    try{
        width_px = pDevice->GetRemoteNode("Width")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_WIDTH);
    }
    log.printf("ROI width = %i px", width_px);

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   24/09/2017
 *
 * Get height of ROI
 *
 * @param [out] height_px
 *	Height of the image in pixels
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::getHeight(int & height_px){
    UserInterface::Log log("SHWSCamera::getHeight");

    if(pDevice==NULL) {return (SHWSCamera_Error) log.error("No opened device",ERR_SHWSCAMERA_NO_DEVICE);}

    try{
        height_px = pDevice->GetRemoteNode("Height")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_HEIGHT);
    }
    log.printf("ROI height = %i px", height_px);

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   24/09/2017
 *
 * Get gain
 *
 * @param [out] gain_dB
 *	Gain in dB
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::getGain(float & gain_dB){
    UserInterface::Log log("SHWSCamera::getGain");

    if(pDevice==NULL) {return (SHWSCamera_Error) log.error("No opened device",ERR_SHWSCAMERA_NO_DEVICE);}

    try{
        gain_dB = pDevice->GetRemoteNode("Gain")->GetDouble();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_GAIN);
    }
    log.printf("Gain = %f", gain_dB);

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   24/09/2017
 *
 * Get exposure
 *
 * @param [out] exposure_us
 *	Exposure in us
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::getExposure(int & exposure_us){
    UserInterface::Log log("SHWSCamera::getExposure");

    if(pDevice==NULL) {return (SHWSCamera_Error) log.error("No opened device",ERR_SHWSCAMERA_NO_DEVICE);}

    try{
        exposure_us = (int)pDevice->GetRemoteNode("ExposureTime")->GetDouble();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_EXPOSURE);
    }
    log.printf("Exposure = %i us", exposure_us);

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   23/09/2017
 *
 * Get size of packets of Ethernet
 *
 * @param [in] Nbytes
 *	Size of packets in bytes
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::getPacketSize(int & Nbytes){
    UserInterface::Log log("SHWSCamera::getPacketSize");

    if(pDevice==NULL) {return (SHWSCamera_Error) log.error("No opened device",ERR_SHWSCAMERA_NO_DEVICE);}

    try{
        Nbytes = pDevice->GetRemoteNode("GevSCPSPacketSize")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_PACKET_SIZE);
    }
    log.printf("Packet size = %i bytes", Nbytes);

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   23/09/2017
 *
 * Get delay between each pack transmitted over ethernet
 *
 * @param [in] Ntics
 *	Number of clock tics between each packet
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::getPacketDelay(int & Ntics){
    UserInterface::Log log("SHWSCamera::getPacketDelay");

    if(pDevice==NULL) {return (SHWSCamera_Error) log.error("No opened device",ERR_SHWSCAMERA_NO_DEVICE);}

    try{
        Ntics = pDevice->GetRemoteNode("GevSCPD")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        return (SHWSCamera_Error) log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("Packet delay = %i tics", Ntics);

    return (SHWSCamera_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   26/09/2017
 *
 * Get all the telemetry for debugging
 *
 * @param [out] telemetry
 *	Telemetry structure
 ******************************************************************************/
SHWSCamera_Error SHWSCamera::getTelemetry(SHWSCamera_Telemetry & telemetry){
    UserInterface::Log log("SHWSCamera::getPacketDelay");

    try{
        sprintf(telemetry.DeviceVendorName, "%s" ,(char *)(pDevice->GetRemoteNode("DeviceVendorName")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("DeviceVendorName = %s ", telemetry.DeviceVendorName);

    try{
        sprintf(telemetry.DeviceModelName, "%s" ,(char *)(pDevice->GetRemoteNode("DeviceModelName")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("DeviceModelName = %s ", telemetry.DeviceModelName);

    try{
        sprintf(telemetry.DeviceManufacturerInfo, "%s" ,(char *)(pDevice->GetRemoteNode("DeviceManufacturerInfo")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("DeviceManufacturerInfo = %s ", telemetry.DeviceManufacturerInfo);

    try{
        sprintf(telemetry.DeviceVersion, "%s" ,(char *)(pDevice->GetRemoteNode("DeviceVersion")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("DeviceVersion = %s ", telemetry.DeviceVersion);

    try{
        sprintf(telemetry.DeviceFirmwareVersion, "%s" ,(char *)(pDevice->GetRemoteNode("DeviceFirmwareVersion")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("DeviceFirmwareVersion = %s ", telemetry.DeviceFirmwareVersion);

    try{
        telemetry.DeviceSFNCVersionMajor = pDevice->GetRemoteNode("DeviceSFNCVersionMajor")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("DeviceSFNCVersionMajor = %i ", telemetry.DeviceSFNCVersionMajor);

    try{
        telemetry.DeviceSFNCVersionMinor = pDevice->GetRemoteNode("DeviceSFNCVersionMinor")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("DeviceSFNCVersionMinor = %i ", telemetry.DeviceSFNCVersionMinor);

    try{
        telemetry.DeviceSFNCVersionSubMinor = pDevice->GetRemoteNode("DeviceSFNCVersionSubMinor")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("DeviceSFNCVersionSubMinor = %i ", telemetry.DeviceSFNCVersionSubMinor);

    try{
        sprintf(telemetry.DeviceUserID, "%s" ,(char *)(pDevice->GetRemoteNode("DeviceUserID")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("DeviceUserID = %s ", telemetry.DeviceUserID);

    try{
        sprintf(telemetry.DeviceReset, "%s" ,(char *)(pDevice->GetRemoteNode("DeviceReset")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("DeviceReset = %s ", telemetry.DeviceReset);

    try{
        telemetry.SensorWidth = pDevice->GetRemoteNode("SensorWidth")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("SensorWidth = %i ", telemetry.SensorWidth);

    try{
        telemetry.SensorHeight = pDevice->GetRemoteNode("SensorHeight")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("SensorHeight = %i ", telemetry.SensorHeight);

    try{
        telemetry.WidthMax = pDevice->GetRemoteNode("WidthMax")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("WidthMax = %i ", telemetry.WidthMax);

    try{
        telemetry.HeightMax = pDevice->GetRemoteNode("HeightMax")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("HeightMax = %i ", telemetry.HeightMax);

    try{
        telemetry.Width = pDevice->GetRemoteNode("Width")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("Width = %i ", telemetry.Width);

    try{
        telemetry.Height = pDevice->GetRemoteNode("Height")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("Height = %i ", telemetry.Height);

    try{
        telemetry.OffsetX = pDevice->GetRemoteNode("OffsetX")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("OffsetX = %i ", telemetry.OffsetX);

    try{
        telemetry.OffsetY = pDevice->GetRemoteNode("OffsetY")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("OffsetY = %i ", telemetry.OffsetY);

    try{
        telemetry.BinningHorizontal = pDevice->GetRemoteNode("BinningHorizontal")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("BinningHorizontal = %i ", telemetry.BinningHorizontal);

    try{
        telemetry.BinningVertical = pDevice->GetRemoteNode("BinningVertical")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("BinningVertical = %i ", telemetry.BinningVertical);

    try{
        telemetry.ReverseX = pDevice->GetRemoteNode("ReverseX")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("ReverseX = %i ", telemetry.ReverseX);

    try{
        telemetry.ReverseY = pDevice->GetRemoteNode("ReverseY")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("ReverseY = %i ", telemetry.ReverseY);

    try{
        sprintf(telemetry.PixelFormat, "%s" ,(char *)(pDevice->GetRemoteNode("PixelFormat")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("PixelFormat = %s ", telemetry.PixelFormat);

    try{
        sprintf(telemetry.TestImageSelector, "%s" ,(char *)(pDevice->GetRemoteNode("TestImageSelector")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("TestImageSelector = %s ", telemetry.TestImageSelector);

    try{
        sprintf(telemetry.AcquisitionMode, "%s" ,(char *)(pDevice->GetRemoteNode("AcquisitionMode")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("AcquisitionMode = %s ", telemetry.AcquisitionMode);

    try{
        telemetry.AcquisitionFrameRate = pDevice->GetRemoteNode("AcquisitionFrameRate")->GetDouble();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("AcquisitionFrameRate = %f ", telemetry.AcquisitionFrameRate);

    try{
        sprintf(telemetry.TriggerSelector, "%s" ,(char *)(pDevice->GetRemoteNode("TriggerSelector")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("TriggerSelector = %s ", telemetry.TriggerSelector);

    try{
        sprintf(telemetry.TriggerMode, "%s" ,(char *)(pDevice->GetRemoteNode("TriggerMode")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("TriggerMode = %s ", telemetry.TriggerMode);

    try{
        sprintf(telemetry.TriggerSource, "%s" ,(char *)(pDevice->GetRemoteNode("TriggerSource")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("TriggerSource = %s ", telemetry.TriggerSource);

    try{
        sprintf(telemetry.TriggerActivation, "%s" ,(char *)(pDevice->GetRemoteNode("TriggerActivation")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("TriggerActivation = %s ", telemetry.TriggerActivation);

    try{
        sprintf(telemetry.TriggerOverlap, "%s" ,(char *)(pDevice->GetRemoteNode("TriggerOverlap")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("TriggerOverlap = %s ", telemetry.TriggerOverlap);

    try{
        telemetry.TriggerDelay = pDevice->GetRemoteNode("TriggerDelay")->GetDouble();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("TriggerDelay = %f ", telemetry.TriggerDelay);

    try{
        sprintf(telemetry.ExposureMode, "%s" ,(char *)(pDevice->GetRemoteNode("ExposureMode")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("ExposureMode = %s ", telemetry.ExposureMode);

    try{
        telemetry.ExposureTime = pDevice->GetRemoteNode("ExposureTime")->GetDouble();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("ExposureTime = %f ", telemetry.ExposureTime);

    try{
        sprintf(telemetry.LineSelector, "%s" ,(char *)(pDevice->GetRemoteNode("LineSelector")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("LineSelector = %s ", telemetry.LineSelector);

    try{
        sprintf(telemetry.LineMode, "%s" ,(char *)(pDevice->GetRemoteNode("LineMode")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("LineMode = %s ", telemetry.LineMode);

    try{
        telemetry.LineInverter = pDevice->GetRemoteNode("LineInverter")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("LineInverter = %i ", telemetry.LineInverter);

    try{
        telemetry.LineStatus = pDevice->GetRemoteNode("LineStatus")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("LineStatus = %i ", telemetry.LineStatus);

    try{
        telemetry.LineStatusAll = pDevice->GetRemoteNode("LineStatusAll")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("LineStatusAll = %i ", telemetry.LineStatusAll);

    try{
        sprintf(telemetry.LineSource, "%s" ,(char *)(pDevice->GetRemoteNode("LineSource")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("LineSource = %s ", telemetry.LineSource);

    try{
        sprintf(telemetry.UserOutputSelector, "%s" ,(char *)(pDevice->GetRemoteNode("UserOutputSelector")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("UserOutputSelector = %s ", telemetry.UserOutputSelector);

    try{
        telemetry.UserOutputValue = pDevice->GetRemoteNode("UserOutputValue")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("UserOutputValue = %i ", telemetry.UserOutputValue);

    try{
        telemetry.UserOutputValueAll = pDevice->GetRemoteNode("UserOutputValueAll")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("UserOutputValueAll = %i ", telemetry.UserOutputValueAll);

    try{
        sprintf(telemetry.TimerSelector, "%s" ,(char *)(pDevice->GetRemoteNode("TimerSelector")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("TimerSelector = %s ", telemetry.TimerSelector);

    try{
        telemetry.TimerDuration = pDevice->GetRemoteNode("TimerDuration")->GetDouble();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("TimerDuration = %f ", telemetry.TimerDuration);

    try{
        telemetry.TimerDelay = pDevice->GetRemoteNode("TimerDelay")->GetDouble();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("TimerDelay = %f ", telemetry.TimerDelay);

    try{
        sprintf(telemetry.TimerTriggerSource, "%s" ,(char *)(pDevice->GetRemoteNode("TimerTriggerSource")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("TimerTriggerSource = %s ", telemetry.TimerTriggerSource);

    try{
        sprintf(telemetry.TimerTriggerActivation, "%s" ,(char *)(pDevice->GetRemoteNode("TimerTriggerActivation")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("TimerTriggerActivation = %s ", telemetry.TimerTriggerActivation);

    try{
        sprintf(telemetry.EventSelector, "%s" ,(char *)(pDevice->GetRemoteNode("EventSelector")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("EventSelector = %s ", telemetry.EventSelector);

    try{
        sprintf(telemetry.EventNotification, "%s" ,(char *)(pDevice->GetRemoteNode("EventNotification")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("EventNotification = %s ", telemetry.EventNotification);

    try{
        sprintf(telemetry.GainSelector, "%s" ,(char *)(pDevice->GetRemoteNode("GainSelector")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GainSelector = %s ", telemetry.GainSelector);

    try{
        telemetry.Gain = pDevice->GetRemoteNode("Gain")->GetDouble();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("Gain = %f ", telemetry.Gain);

    try{
        sprintf(telemetry.BlackLevelSelector, "%s" ,(char *)(pDevice->GetRemoteNode("BlackLevelSelector")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("BlackLevelSelector = %s ", telemetry.BlackLevelSelector);

    try{
        telemetry.BlackLevel = pDevice->GetRemoteNode("BlackLevel")->GetDouble();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("BlackLevel = %f ", telemetry.BlackLevel);

    try{
        telemetry.BlackLevelRaw = pDevice->GetRemoteNode("BlackLevelRaw")->GetDouble();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("BlackLevelRaw = %f ", telemetry.BlackLevelRaw);

    try{
        telemetry.Gamma = pDevice->GetRemoteNode("Gamma")->GetDouble();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("Gamma = %f ", telemetry.Gamma);

    try{
        sprintf(telemetry.LUTSelector, "%s" ,(char *)(pDevice->GetRemoteNode("LUTSelector")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("LUTSelector = %s ", telemetry.LUTSelector);

    try{
        telemetry.LUTEnable = pDevice->GetRemoteNode("LUTEnable")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("LUTEnable = %i ", telemetry.LUTEnable);

    try{
        telemetry.LUTIndex = pDevice->GetRemoteNode("LUTIndex")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("LUTIndex = %i ", telemetry.LUTIndex);

    try{
        telemetry.LUTValue = pDevice->GetRemoteNode("LUTValue")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("LUTValue = %i ", telemetry.LUTValue);

    try{
        telemetry.TLParamsLocked = pDevice->GetRemoteNode("TLParamsLocked")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("TLParamsLocked = %i ", telemetry.TLParamsLocked);

    try{
        telemetry.PayloadSize = pDevice->GetRemoteNode("PayloadSize")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("PayloadSize = %i ", telemetry.PayloadSize);

    try{
        telemetry.GevVersionMajor = pDevice->GetRemoteNode("GevVersionMajor")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevVersionMajor = %i ", telemetry.GevVersionMajor);

    try{
        telemetry.GevVersionMinor = pDevice->GetRemoteNode("GevVersionMinor")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevVersionMinor = %i ", telemetry.GevVersionMinor);

    try{
        telemetry.GevDeviceModeIsBigEndian = pDevice->GetRemoteNode("GevDeviceModeIsBigEndian")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevDeviceModeIsBigEndian = %i ", telemetry.GevDeviceModeIsBigEndian);

    try{
        sprintf(telemetry.GevDeviceModeCharacterSet, "%s" ,(char *)(pDevice->GetRemoteNode("GevDeviceModeCharacterSet")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevDeviceModeCharacterSet = %s ", telemetry.GevDeviceModeCharacterSet);

    try{
        telemetry.GevInterfaceSelector = pDevice->GetRemoteNode("GevInterfaceSelector")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevInterfaceSelector = %i ", telemetry.GevInterfaceSelector);

    try{
        telemetry.GevMACAddress = pDevice->GetRemoteNode("GevMACAddress")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevMACAddress = %i ", telemetry.GevMACAddress);

    try{
        sprintf(telemetry.GevSupportedOptionSelector, "%s" ,(char *)(pDevice->GetRemoteNode("GevSupportedOptionSelector")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevSupportedOptionSelector = %s ", telemetry.GevSupportedOptionSelector);

    try{
        telemetry.GevSupportedOption = pDevice->GetRemoteNode("GevSupportedOption")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevSupportedOption = %i ", telemetry.GevSupportedOption);

    try{
        telemetry.GevCurrentIPConfigurationLLA = pDevice->GetRemoteNode("GevCurrentIPConfigurationLLA")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevCurrentIPConfigurationLLA = %i ", telemetry.GevCurrentIPConfigurationLLA);

    try{
        telemetry.GevCurrentIPConfigurationDHCP = pDevice->GetRemoteNode("GevCurrentIPConfigurationDHCP")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevCurrentIPConfigurationDHCP = %i ", telemetry.GevCurrentIPConfigurationDHCP);

    try{
        telemetry.GevCurrentIPConfigurationPersistentIP = pDevice->GetRemoteNode("GevCurrentIPConfigurationPersistentIP")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevCurrentIPConfigurationPersistentIP = %i ", telemetry.GevCurrentIPConfigurationPersistentIP);

    try{
        telemetry.GevCurrentIPAddress = pDevice->GetRemoteNode("GevCurrentIPAddress")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevCurrentIPAddress = %i ", telemetry.GevCurrentIPAddress);

    try{
        telemetry.GevCurrentSubnetMask = pDevice->GetRemoteNode("GevCurrentSubnetMask")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevCurrentSubnetMask = %i ", telemetry.GevCurrentSubnetMask);

    try{
        telemetry.GevCurrentDefaultGateway = pDevice->GetRemoteNode("GevCurrentDefaultGateway")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevCurrentDefaultGateway = %i ", telemetry.GevCurrentDefaultGateway);

    try{
        sprintf(telemetry.GevFirstURL, "%s" ,(char *)(pDevice->GetRemoteNode("GevFirstURL")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevFirstURL = %s ", telemetry.GevFirstURL);

    try{
        sprintf(telemetry.GevSecondURL, "%s" ,(char *)(pDevice->GetRemoteNode("GevSecondURL")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevSecondURL = %s ", telemetry.GevSecondURL);

    try{
        telemetry.GevNumberOfInterfaces = pDevice->GetRemoteNode("GevNumberOfInterfaces")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevNumberOfInterfaces = %i ", telemetry.GevNumberOfInterfaces);

    try{
        telemetry.GevPersistentIPAddress = pDevice->GetRemoteNode("GevPersistentIPAddress")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevPersistentIPAddress = %i ", telemetry.GevPersistentIPAddress);

    try{
        telemetry.GevPersistentSubnetMask = pDevice->GetRemoteNode("GevPersistentSubnetMask")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevPersistentSubnetMask = %i ", telemetry.GevPersistentSubnetMask);

    try{
        telemetry.GevPersistentDefaultGateway = pDevice->GetRemoteNode("GevPersistentDefaultGateway")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevPersistentDefaultGateway = %i ", telemetry.GevPersistentDefaultGateway);

    try{
        telemetry.GevLinkSpeed = pDevice->GetRemoteNode("GevLinkSpeed")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevLinkSpeed = %i ", telemetry.GevLinkSpeed);

    try{
        telemetry.GevMessageChannelCount = pDevice->GetRemoteNode("GevMessageChannelCount")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevMessageChannelCount = %i ", telemetry.GevMessageChannelCount);

    try{
        telemetry.GevStreamChannelCount = pDevice->GetRemoteNode("GevStreamChannelCount")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevStreamChannelCount = %i ", telemetry.GevStreamChannelCount);

    try{
        telemetry.GevHeartbeatTimeout = pDevice->GetRemoteNode("GevHeartbeatTimeout")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevHeartbeatTimeout = %i ", telemetry.GevHeartbeatTimeout);

    try{
        telemetry.GevTimestampTickFrequency = pDevice->GetRemoteNode("GevTimestampTickFrequency")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevTimestampTickFrequency = %i ", telemetry.GevTimestampTickFrequency);

    try{
        telemetry.GevTimestampValue = pDevice->GetRemoteNode("GevTimestampValue")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevTimestampValue = %i ", telemetry.GevTimestampValue);

    try{
        telemetry.GevGVCPPendingAck = pDevice->GetRemoteNode("GevGVCPPendingAck")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevGVCPPendingAck = %i ", telemetry.GevGVCPPendingAck);

    try{
        telemetry.GevGVCPHeartbeatDisable = pDevice->GetRemoteNode("GevGVCPHeartbeatDisable")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevGVCPHeartbeatDisable = %i ", telemetry.GevGVCPHeartbeatDisable);

    try{
        telemetry.GevGVCPPendingTimeout = pDevice->GetRemoteNode("GevGVCPPendingTimeout")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevGVCPPendingTimeout = %i ", telemetry.GevGVCPPendingTimeout);

    try{
        sprintf(telemetry.GevCCP, "%s" ,(char *)(pDevice->GetRemoteNode("GevCCP")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevCCP = %s ", telemetry.GevCCP);

    try{
        telemetry.GevPrimaryApplicationSocket = pDevice->GetRemoteNode("GevPrimaryApplicationSocket")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevPrimaryApplicationSocket = %i ", telemetry.GevPrimaryApplicationSocket);

    try{
        telemetry.GevPrimaryApplicationIPAddress = pDevice->GetRemoteNode("GevPrimaryApplicationIPAddress")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevPrimaryApplicationIPAddress = %i ", telemetry.GevPrimaryApplicationIPAddress);

    try{
        telemetry.GevMCPHostPort = pDevice->GetRemoteNode("GevMCPHostPort")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevMCPHostPort = %i ", telemetry.GevMCPHostPort);

    try{
        telemetry.GevMCDA = pDevice->GetRemoteNode("GevMCDA")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevMCDA = %i ", telemetry.GevMCDA);

    try{
        telemetry.GevMCTT = pDevice->GetRemoteNode("GevMCTT")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevMCTT = %i ", telemetry.GevMCTT);

    try{
        telemetry.GevMCRC = pDevice->GetRemoteNode("GevMCRC")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevMCRC = %i ", telemetry.GevMCRC);

    try{
        telemetry.GevStreamChannelSelector = pDevice->GetRemoteNode("GevStreamChannelSelector")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevStreamChannelSelector = %i ", telemetry.GevStreamChannelSelector);

    try{
        telemetry.GevSCPInterfaceIndex = pDevice->GetRemoteNode("GevSCPInterfaceIndex")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevSCPInterfaceIndex = %i ", telemetry.GevSCPInterfaceIndex);

    try{
        telemetry.GevSCPHostPort = pDevice->GetRemoteNode("GevSCPHostPort")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevSCPHostPort = %i ", telemetry.GevSCPHostPort);

    try{
        telemetry.GevSCPSFireTestPacket = pDevice->GetRemoteNode("GevSCPSFireTestPacket")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevSCPSFireTestPacket = %i ", telemetry.GevSCPSFireTestPacket);

    try{
        telemetry.GevSCPSDoNotFragment = pDevice->GetRemoteNode("GevSCPSDoNotFragment")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevSCPSDoNotFragment = %i ", telemetry.GevSCPSDoNotFragment);

    try{
        telemetry.GevSCPSBigEndian = pDevice->GetRemoteNode("GevSCPSBigEndian")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevSCPSBigEndian = %i ", telemetry.GevSCPSBigEndian);

    try{
        telemetry.GevSCPSPacketSize = pDevice->GetRemoteNode("GevSCPSPacketSize")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevSCPSPacketSize = %i ", telemetry.GevSCPSPacketSize);

    try{
        telemetry.GevSCPD = pDevice->GetRemoteNode("GevSCPD")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevSCPD = %i ", telemetry.GevSCPD);

    try{
        telemetry.GevSCDA = pDevice->GetRemoteNode("GevSCDA")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("GevSCDA = %i ", telemetry.GevSCDA);

    try{
        sprintf(telemetry.UserSetSelector, "%s" ,(char *)(pDevice->GetRemoteNode("UserSetSelector")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("UserSetSelector = %s ", telemetry.UserSetSelector);

    try{
        sprintf(telemetry.UserSetDefaultSelector, "%s" ,(char *)(pDevice->GetRemoteNode("UserSetDefaultSelector")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("UserSetDefaultSelector = %s ", telemetry.UserSetDefaultSelector);

    try{
        telemetry.ChunkModeActive = pDevice->GetRemoteNode("ChunkModeActive")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("ChunkModeActive = %i ", telemetry.ChunkModeActive);

    try{
        sprintf(telemetry.ChunkSelector, "%s" ,(char *)(pDevice->GetRemoteNode("ChunkSelector")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("ChunkSelector = %s ", telemetry.ChunkSelector);

    try{
        telemetry.ChunkEnable = pDevice->GetRemoteNode("ChunkEnable")->GetBool();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("ChunkEnable = %i ", telemetry.ChunkEnable);

    try{
        telemetry.ActionSelector = pDevice->GetRemoteNode("ActionSelector")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("ActionSelector = %i ", telemetry.ActionSelector);

    try{
        telemetry.ActionGroupMask = pDevice->GetRemoteNode("ActionGroupMask")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("ActionGroupMask = %i ", telemetry.ActionGroupMask);

    try{
        telemetry.ActionGroupKey = pDevice->GetRemoteNode("ActionGroupKey")->GetInt();
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("ActionGroupKey = %i ", telemetry.ActionGroupKey);

    try{
        sprintf(telemetry.DeviceID, "%s" ,(char *)(pDevice->GetRemoteNode("DeviceID")->GetValue()));
    }
    catch (BGAPI2::Exceptions::IException& ex){
        status = SHWSCAMERA_ERROR;
        log.error(ex.GetErrorDescription(), ERR_SHWSCAMERA_GET_TELEMETRY);
    }
    log.printf("DeviceID = %s ", telemetry.DeviceID);

    return (SHWSCamera_Error) log.success();
}
