/***************************************************************************//**
 * @file	XBee.cpp
 * @brief	Source file to operate the camera XBee
 *
 * This file contains all the implementations for the functions defined in:
 * api/include/XBee.hpp
 *
 * @author	Thibaud Talon
 * @date	25/09/2017
 *******************************************************************************/

#include <stdint.h>
#include <strings.h>
#include <stdlib.h>     /* atoll */
#include <xbee.h>
#include "XBee.hpp"
#include "UserInterface.hpp"

#define XBEE_MAX_MESSAGE_LENGTH 256

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Connect the XBee module
 *
 * @param [in] baudrate
 *	baudrate of the USB connection
 ******************************************************************************/
XBee_Error XBee::connect(int baudrate){
    UserInterface::Log log("XBee::connect");
    try
    {
        // 1. Initialize parameters
        log.printf("1. Initialize parameters");
        log.printf("Baudrate = %i Hz",baudrate);
        _baudrate = baudrate;
        xbee = NULL;
        status = XBEE_OFF;
        for (int i = 0; i < XBEE_MAX_NODES; i++) {
            nodes[i].con = NULL;
            nodes[i].status = XBEE_OFF;
        }

        // 2. Connect the the XBee device
        log.printf("2. Connect the the XBee device");
        error = xbee_setup(&xbee, "xbeeZB", "/dev/ttyUSB0", _baudrate);
        if (error != XBEE_ENONE){
            error = xbee_setup(&xbee, "xbeeZB", "/dev/ttyUSB1", _baudrate);
            if (error != XBEE_ENONE) {return (XBee_Error) log.error(xbee_errorToStr(error), ERR_XBEE_CONNECT_USB);}
        }

        status = XBEE_ON;
        log.printf("XBee status = %i", (int)status);

        for (int II = 0; II < XBEE_MAX_NODES; II++) nodes[II].con = NULL;

        return (XBee_Error) log.success();
    }
    catch(  const std::exception& e  ){
        status = XBEE_ERROR;
        log.printf("XBee stqtus = %i", (int)status);
        return (XBee_Error) log.error(e.what(), ERR_XBEE_CONNECT_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Disconnect the XBee module
 *
 ******************************************************************************/
XBee_Error XBee::disconnect(void){
    UserInterface::Log log("XBee::disconnect");
    try
    {
        log.printf("1. Disconnect all nodes");
        for (int i = 0; i < XBEE_MAX_NODES; i++) {
            if (nodes[i].con) disconnectNode(nodes[i]);
        }

        log.printf("2. Disconnect XBee");
        if (xbee){
            error = xbee_shutdown(xbee);
            if ( error != XBEE_ENONE ) {return (XBee_Error) log.error(xbee_errorToStr(error),ERR_XBEE_DISCONNECT);}
        }

        status = XBEE_OFF;

        return (XBee_Error) log.success();
    }
    catch(  const std::exception& e  ){
        status = XBEE_ERROR;
        return (XBee_Error) log.error(e.what(), ERR_XBEE_DISCONNECT_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Reset the connection to the XBee module
 *
 ******************************************************************************/
XBee_Error XBee::reset(void){
    disconnect();
    return connect(_baudrate);
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Connect to a remote XBee node
 *
 * @param [out] handle
 *	Handle of the remote node
 * @param [in] addr64
 *	64-bit MAC address of the remote XBee node
 ******************************************************************************/
XBee_Error XBee::connectNode(XBeeNode & handle, uint64_t addr64){
    UserInterface::Log log("XBee::connectNode");
    try
    {
        if (!xbee) return (XBee_Error) log.error("XBee not set up", ERR_XBEE_NO_DEVICE);

        // 1. Register connection
        log.printf("1. Register connection");
        int con_index = 0;
        while((nodes[con_index].con != NULL) && (con_index < XBEE_MAX_NODES)) con_index++;
        if (con_index == XBEE_MAX_NODES){return (XBee_Error) log.error("Maximum number of nodes reached", ERR_XBEE_TOO_MANY_NODES);}
        nodes[con_index] = handle;

        handle.status = XBEE_OFF;

        // 2. Setup MAC address
        log.printf("2. Setup MAC address = 0x%lx", addr64);
        struct xbee_conAddress address;
        memset(&address, 0, sizeof(address));
        address.addr64_enabled = 1;
        for (int i = 0; i < 8; i++) {
            memcpy(&address.addr64[7-i], (char *) &addr64 + i, 1);
        }
        handle.address = addr64;

        // 3. Connect to remote node
        log.printf("3. Connect to remote node");
        handle.error = xbee_conNew(xbee, &handle.con, "Data", &address);
        if (handle.error != XBEE_ENONE) {
            status = XBEE_ERROR;
            error = handle.error;
            return (XBee_Error) log.error(xbee_errorToStr(handle.error),ERR_XBEE_NEW_NODE);
        }

        // 4. Set connection data
        log.printf("4. Set connection data");
        handle.error = xbee_conDataSet(handle.con, xbee, NULL);
        if (handle.error != XBEE_ENONE) {
            status = XBEE_ERROR;
            error = handle.error;
            return (XBee_Error) log.error(xbee_errorToStr(handle.error),ERR_XBEE_NODE_DATASET);
        }
        handle.countTx = 0;
        handle.countRx = 0;
        handle.status = XBEE_ON;

        // 5. Purge connection
        log.printf("5. Purge connection");
        XBee_Error ret;
        if ( ret = purge(handle)) {
            handle.status = XBEE_ERROR;
            status = XBEE_ERROR;
            return (XBee_Error) log.error("Connection could not be purged", ret);
        }

        return (XBee_Error) log.success();
    }
    catch(  const std::exception& e  ){
        status = XBEE_ERROR;
        handle.status = XBEE_ERROR;
        return (XBee_Error) log.error(e.what(), ERR_XBEE_CONNECT_NODE_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Disconnect a remote XBee node
 *
 * @param [in] handle
 *	Handle of the remote node
 ******************************************************************************/
XBee_Error XBee::disconnectNode(XBeeNode & handle){
    UserInterface::Log log("XBee::disconnectNode");
    try
    {
        if (!xbee) return (XBee_Error) log.error("XBee not set up", ERR_XBEE_NO_DEVICE);
        if (!handle.con){
            log.printf("No connection to this node");
            return (XBee_Error) log.success();
        }

        log.printf("Close remote connection");
        handle.error = xbee_conEnd(handle.con);
        if (handle.error != XBEE_ENONE) {
            status = XBEE_ERROR;
            handle.status = XBEE_ERROR;
            error = handle.error;
            return (XBee_Error) log.error(xbee_errorToStr(handle.error),ERR_XBEE_END_NODE);
        }
        handle.con = NULL;
        handle.status = XBEE_OFF;

        return (XBee_Error) log.success();
    }
    catch(  const std::exception& e  ){
        status = XBEE_ERROR;
        handle.status = XBEE_ERROR;
        return (XBee_Error) log.error(e.what(), ERR_XBEE_DISCONNECT_NODE_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Reset the connection to a remote XBee node
 *
 * @param [in] handle
 *	Handle of the remote node
 ******************************************************************************/
XBee_Error XBee::resetNode(XBeeNode & handle){
    disconnectNode(handle);
    return connectNode(handle, handle.address);
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Send message to a remote XBee node
 *
 * @param [in] handle
 *	Handle of the remote node
 * @param [in] msg
 *	Message to send (char array)
 * @param [in] len
 *	Length of the message
 ******************************************************************************/
XBee_Error XBee::send(XBeeNode & handle, const unsigned char *msg, unsigned int len){
    UserInterface::Log log("XBee::send");
    try
    {
        if (!handle.con) return (XBee_Error) log.error("Remote XBee not connected", ERR_XBEE_NO_CONNECTION);

        log.printf("1. Check inputs");
        if (len > XBEE_MAX_MESSAGE_LENGTH) return (XBee_Error) log.error("Too much data to send", ERR_XBEE_SEND_LEN);

        log.printf("2. Transmit data");
        handle.error = xbee_connTx(handle.con, NULL, msg, len);
        if (handle.error != XBEE_ENONE){
            handle.status = XBEE_ERROR;
            status = XBEE_ERROR;
            error = handle.error;
            return (XBee_Error) log.error(xbee_errorToStr(handle.error),ERR_XBEE_SEND);
        }

        return (XBee_Error) log.success();
    }
    catch(  const std::exception& e  ){
        status = XBEE_ERROR;
        handle.status = XBEE_ERROR;
        return (XBee_Error) log.error(e.what(), ERR_XBEE_SEND_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 *
 * Recieve a message from a remote XBee node
 *
 * @param [in] handle
 *	Handle of the remote node
 * @param [out] msg
 *	Message to send (char array)
 * @param [out] len
 *	Length of the message
 * @param [in] timeout (optional)
 *	Maximum time to wait for the message
 ******************************************************************************/
XBee_Error XBee::receive(XBeeNode & handle, unsigned char **msg, int *len, int timeout){
    UserInterface::Log log("XBee::receive");
    try
    {
        if (!handle.con) return (XBee_Error) log.error("Remote XBee not connected", ERR_XBEE_NO_CONNECTION);

        // 1. Receive data
        log.printf("1. Receive data with timeout = %i s", timeout);
        struct xbee_pkt *pkt;
        int counter = 0;
        while(((handle.error = xbee_conRxWait(handle.con, &pkt, NULL)) != XBEE_ENONE) && (counter < timeout)) log.printf("Counter = %i s",++counter);

        if(counter == timeout) {return (XBee_Error) log.error("Timeout",ERR_XBEE_RECEIVE_TIMEOUT);}

        *msg = (pkt)->data;
        *len = (pkt)->dataLen;

        if ((handle.error = xbee_pktFree(pkt)) != XBEE_ENONE) return (XBee_Error) log.error(xbee_errorToStr(handle.error),ERR_XBEE_FREE_PKT);

        // 2. Purge connection
        log.printf("2. Purge connection");
        XBee_Error ret;
        if ( ret = purge(handle)) {
            handle.status = XBEE_ERROR;
            status = XBEE_ERROR;
            return (XBee_Error) log.error("Connection could not be purged", ret);
        }

        return (XBee_Error) log.success();
    }
    catch(  const std::exception& e  ){
        status = XBEE_ERROR;
        handle.status = XBEE_ERROR;
        return (XBee_Error) log.error(e.what(), ERR_XBEE_RECEIVE_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 *
 * Purge connection to a remote XBee node
 *
 * @param [in] handle
 *	Handle of the remote node
 ******************************************************************************/
XBee_Error XBee::purge(XBeeNode & handle){
    UserInterface::Log log("XBee::purge");
    try
    {
        if (!handle.con){
            log.printf("No connection to this node");
            return (XBee_Error) log.success();
        }

        log.printf("Purge connection");
        handle.error = xbee_conPurge(handle.con);
        if (handle.error != XBEE_ENONE){
            handle.status = XBEE_ERROR;
            status = XBEE_ERROR;
            error = handle.error;
            return (XBee_Error) log.error(xbee_errorToStr(handle.error),ERR_XBEE_PURGE);
        }

        return (XBee_Error) log.success();
    }
    catch(  const std::exception& e  ){
        status = XBEE_ERROR;
        handle.status = XBEE_ERROR;
        return (XBee_Error) log.error(e.what(), ERR_XBEE_PURGE_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Get current number of connected remote XBee devices
 *
 * @param [out] nodes_count
 *	Number of connected remote XBee devices
 ******************************************************************************/
XBee_Error XBee::getNodesCount(int & nodes_count){
    UserInterface::Log log("XBee::getNodesCount");

    nodes_count = 0;
    for (int II = 0; II < XBEE_MAX_NODES; II++){
        if ( nodes[II].con ) nodes_count++;
    }
    log.printf("Number of connected remote XBee devices = %i", nodes_count);

    return (XBee_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   21/09/2017
 *
 * Get baudrate to camera XBee
 *
 * @param [out] baudrate
 *	Baudrate to camera XBee
 ******************************************************************************/
XBee_Error XBee::getBaudrate(int & baudrate){
    UserInterface::Log log("XBee::getBaudrate");

    baudrate = _baudrate;
    log.printf("Baudrate = %i bps", baudrate);

    return (XBee_Error) log.success();
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   29/09/2017
 *
 * Get telemetry from XBee
 *
 * @param [out] telemetry
 *	Telemetry structure
 ******************************************************************************/
XBee_Error XBee::getTelemetry(XBee_Telemetry & telemetry){
    UserInterface::Log log("XBee::getTelemetry");
    try
    {
        if (!xbee) return (XBee_Error) log.error("XBee not set up", ERR_XBEE_NO_DEVICE);

        struct xbee_con *local;

        // 1. Connect to local AT node
        log.printf("1. Connect to local AT node");
        error = xbee_conNew(xbee, &local, "Local AT", NULL);
        if (error != XBEE_ENONE) {
            status = XBEE_ERROR;
            return (XBee_Error) log.error(xbee_errorToStr(error),ERR_XBEE_NEW_NODE);
        }

        // 2. Purge local connection
        log.printf("2. Purge local connection");
        error = xbee_conPurge(local);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            return (XBee_Error) log.error(xbee_errorToStr(error),ERR_XBEE_PURGE);
        }

        unsigned char retVal;
        struct xbee_pkt *pkt;

        error = xbee_connTx(local, &retVal, (const unsigned char *)"ID", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "ID", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "ID");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.ID = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.ID = (telemetry.ID << 8) | pkt->data[q];
                log.printf("ID = 0x%lx", telemetry.ID);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"SC", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SC", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SC");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SC = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SC = (telemetry.SC << 8) | pkt->data[q];
                log.printf("SC = 0x%lx", telemetry.SC);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"SD", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SD", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SD");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SD = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SD = (telemetry.SD << 8) | pkt->data[q];
                log.printf("SD = 0x%lx", telemetry.SD);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"ZS", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "ZS", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "ZS");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.ZS = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.ZS = (telemetry.ZS << 8) | pkt->data[q];
                log.printf("ZS = 0x%lx", telemetry.ZS);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"NJ", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NJ", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NJ");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.NJ = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.NJ = (telemetry.NJ << 8) | pkt->data[q];
                log.printf("NJ = 0x%lx", telemetry.NJ);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"OP", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "OP", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "OP");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.OP = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.OP = (telemetry.OP << 8) | pkt->data[q];
                log.printf("OP = 0x%lx", telemetry.OP);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"OI", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "OI", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "OI");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.OI = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.OI = (telemetry.OI << 8) | pkt->data[q];
                log.printf("OI = 0x%lx", telemetry.OI);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"CH", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "CH", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "CH");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.CH = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.CH = (telemetry.CH << 8) | pkt->data[q];
                log.printf("CH = 0x%lx", telemetry.CH);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"NC", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NC", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NC");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.NC = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.NC = (telemetry.NC << 8) | pkt->data[q];
                log.printf("NC = 0x%lx", telemetry.NC);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"SH", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SH", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SH");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SH = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SH = (telemetry.SH << 8) | pkt->data[q];
                log.printf("SH = 0x%lx", telemetry.SH);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"SL", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SL", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SL");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SL = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SL = (telemetry.SL << 8) | pkt->data[q];
                log.printf("SL = 0x%lx", telemetry.SL);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"MY", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "MY", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "MY");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.MY = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.MY = (telemetry.MY << 8) | pkt->data[q];
                log.printf("MY = 0x%lx", telemetry.MY);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"DH", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "DH", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "DH");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.DH = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.DH = (telemetry.DH << 8) | pkt->data[q];
                log.printf("DH = 0x%lx", telemetry.DH);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"DL", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "DL", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "DL");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.DL = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.DL = (telemetry.DL << 8) | pkt->data[q];
                log.printf("DL = 0x%lx", telemetry.DL);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"NI", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NI", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NI");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                for (int q = 0; q < pkt->dataLen; q++) telemetry.NI[q] = pkt->data[q];
                log.printf("NI = %s", telemetry.NI);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"NH", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NH", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NH");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.NH = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.NH = (telemetry.NH << 8) | pkt->data[q];
                log.printf("NH = 0x%lx", telemetry.NH);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"BH", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "BH", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "BH");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.BH = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.BH = (telemetry.BH << 8) | pkt->data[q];
                log.printf("BH = 0x%lx", telemetry.BH);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"AR", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "AR", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "AR");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.AR = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.AR = (telemetry.AR << 8) | pkt->data[q];
                log.printf("AR = 0x%lx", telemetry.AR);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"DD", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "DD", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "DD");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.DD = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.DD = (telemetry.DD << 8) | pkt->data[q];
                log.printf("DD = 0x%lx", telemetry.DD);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"NT", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NT", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NT");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.NT = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.NT = (telemetry.NT << 8) | pkt->data[q];
                log.printf("NT = 0x%lx", telemetry.NT);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"NO", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NO", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NO");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.NO = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.NO = (telemetry.NO << 8) | pkt->data[q];
                log.printf("NO = 0x%lx", telemetry.NO);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"NP", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NP", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NP");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.NP = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.NP = (telemetry.NP << 8) | pkt->data[q];
                log.printf("NP = 0x%lx", telemetry.NP);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"CR", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "CR", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "CR");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.CR = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.CR = (telemetry.CR << 8) | pkt->data[q];
                log.printf("CR = 0x%lx", telemetry.CR);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"PL", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "PL", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "PL");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.PL = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.PL = (telemetry.PL << 8) | pkt->data[q];
                log.printf("PL = 0x%lx", telemetry.PL);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"PM", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "PM", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "PM");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.PM = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.PM = (telemetry.PM << 8) | pkt->data[q];
                log.printf("PM = 0x%lx", telemetry.PM);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"PP", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "PP", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "PP");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.PP = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.PP = (telemetry.PP << 8) | pkt->data[q];
                log.printf("PP = 0x%lx", telemetry.PP);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"EE", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "EE", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "EE");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.EE = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.EE = (telemetry.EE << 8) | pkt->data[q];
                log.printf("EE = 0x%lx", telemetry.EE);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"EO", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "EO", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "EO");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.EO = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.EO = (telemetry.EO << 8) | pkt->data[q];
                log.printf("EO = 0x%lx", telemetry.EO);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"BD", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "BD", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "BD");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.BD = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.BD = (telemetry.BD << 8) | pkt->data[q];
                log.printf("BD = 0x%lx", telemetry.BD);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"NB", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NB", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NB");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.NB = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.NB = (telemetry.NB << 8) | pkt->data[q];
                log.printf("NB = 0x%lx", telemetry.NB);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"SB", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SB", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SB");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SB = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SB = (telemetry.SB << 8) | pkt->data[q];
                log.printf("SB = 0x%lx", telemetry.SB);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"D7", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "D7", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "D7");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.D7 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.D7 = (telemetry.D7 << 8) | pkt->data[q];
                log.printf("D7 = 0x%lx", telemetry.D7);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"D6", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "D6", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "D6");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.D6 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.D6 = (telemetry.D6 << 8) | pkt->data[q];
                log.printf("D6 = 0x%lx", telemetry.D6);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"AP", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "AP", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "AP");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.AP = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.AP = (telemetry.AP << 8) | pkt->data[q];
                log.printf("AP = 0x%lx", telemetry.AP);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"AO", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "AO", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "AO");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.AO = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.AO = (telemetry.AO << 8) | pkt->data[q];
                log.printf("AO = 0x%lx", telemetry.AO);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"SP", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SP", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SP");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SP = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SP = (telemetry.SP << 8) | pkt->data[q];
                log.printf("SP = 0x%lx", telemetry.SP);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"SN", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SN", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SN");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SN = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SN = (telemetry.SN << 8) | pkt->data[q];
                log.printf("SN = 0x%lx", telemetry.SN);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"D0", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "D0", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "D0");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.D0 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.D0 = (telemetry.D0 << 8) | pkt->data[q];
                log.printf("D0 = 0x%lx", telemetry.D0);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"D1", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "D1", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "D1");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.D1 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.D1 = (telemetry.D1 << 8) | pkt->data[q];
                log.printf("D1 = 0x%lx", telemetry.D1);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"D2", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "D2", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "D2");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.D2 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.D2 = (telemetry.D2 << 8) | pkt->data[q];
                log.printf("D2 = 0x%lx", telemetry.D2);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"D3", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "D3", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "D3");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.D3 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.D3 = (telemetry.D3 << 8) | pkt->data[q];
                log.printf("D3 = 0x%lx", telemetry.D3);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"D4", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "D4", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "D4");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.D4 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.D4 = (telemetry.D4 << 8) | pkt->data[q];
                log.printf("D4 = 0x%lx", telemetry.D4);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"D5", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "D5", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "D5");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.D5 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.D5 = (telemetry.D5 << 8) | pkt->data[q];
                log.printf("D5 = 0x%lx", telemetry.D5);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"P0", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "P0", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "P0");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.P0 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.P0 = (telemetry.P0 << 8) | pkt->data[q];
                log.printf("P0 = 0x%lx", telemetry.P0);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"P1", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "P1", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "P1");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.P1 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.P1 = (telemetry.P1 << 8) | pkt->data[q];
                log.printf("P1 = 0x%lx", telemetry.P1);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"P2", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "P2", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "P2");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.P2 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.P2 = (telemetry.P2 << 8) | pkt->data[q];
                log.printf("P2 = 0x%lx", telemetry.P2);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"PR", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "PR", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "PR");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.PR = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.PR = (telemetry.PR << 8) | pkt->data[q];
                log.printf("PR = 0x%lx", telemetry.PR);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"LT", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "LT", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "LT");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.LT = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.LT = (telemetry.LT << 8) | pkt->data[q];
                log.printf("LT = 0x%lx", telemetry.LT);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"RP", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "RP", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "RP");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.RP = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.RP = (telemetry.RP << 8) | pkt->data[q];
                log.printf("RP = 0x%lx", telemetry.RP);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"DO", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "DO", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "DO");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.DO = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.DO = (telemetry.DO << 8) | pkt->data[q];
                log.printf("DO = 0x%lx", telemetry.DO);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"IR", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "IR", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "IR");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.IR = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.IR = (telemetry.IR << 8) | pkt->data[q];
                log.printf("IR = 0x%lx", telemetry.IR);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"IC", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "IC", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "IC");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.IC = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.IC = (telemetry.IC << 8) | pkt->data[q];
                log.printf("IC = 0x%lx", telemetry.IC);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"V+", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "V+", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "V+");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.Vp = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.Vp = (telemetry.Vp << 8) | pkt->data[q];
                log.printf("V+ = 0x%lx", telemetry.Vp);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"VR", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "VR", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "VR");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.VR = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.VR = (telemetry.VR << 8) | pkt->data[q];
                log.printf("VR = 0x%lx", telemetry.VR);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"HV", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "HV", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "HV");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.HV = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.HV = (telemetry.HV << 8) | pkt->data[q];
                log.printf("HV = 0x%lx", telemetry.HV);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"AI", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "AI", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "AI");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.AI = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.AI = (telemetry.AI << 8) | pkt->data[q];
                log.printf("AI = 0x%lx", telemetry.AI);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"DB", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "DB", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "DB");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.DB = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.DB = (telemetry.DB << 8) | pkt->data[q];
                log.printf("DB = 0x%lx", telemetry.DB);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(local, &retVal, (const unsigned char *)"%V", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(local, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "%V", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "%V");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.V = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.V = (telemetry.V << 8) | pkt->data[q];
                log.printf("%V = 0x%lx", telemetry.V);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        // 3. Close local connection
        log.printf("3. Close local connection");
        error = xbee_conEnd(local);
        if (error != XBEE_ENONE) {
            status = XBEE_ERROR;
            return (XBee_Error) log.error(xbee_errorToStr(error),ERR_XBEE_END_NODE);
        }

        return (XBee_Error) log.success();
    }
    catch(  const std::exception& e  ){
        status = XBEE_ERROR;
        return (XBee_Error) log.error(e.what(), ERR_XBEE_TELEMETRY_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   29/09/2017
 *
 * Get telemetry from XBee
 *
 * @param [in] handle
 *	Handle of the remote node
 * @param [out] telemetry
 *	Telemetry structure
 ******************************************************************************/
XBee_Error XBee::getNodeTelemetry(XBeeNode & handle, XBeeNode_Telemetry & telemetry){
    UserInterface::Log log("XBee::getNodeTelemetry");
    try
    {
        if (!xbee) return (XBee_Error) log.error("XBee not set up", ERR_XBEE_NO_DEVICE);
        if (!handle.con) return (XBee_Error) log.error("Remote XBee not connected", ERR_XBEE_NO_CONNECTION);

        struct xbee_con *remote;

        // 1. Setup MAC address
        log.printf("1. Setup MAC address = 0x%0lx", handle.address);
        struct xbee_conAddress address;
        memset(&address, 0, sizeof(address));
        address.addr64_enabled = 1;
        for (int i = 0; i < 8; i++) {
            memcpy(&address.addr64[7-i], (char *) &handle.address + i, 1);
        }

        // 2. Connect to remote AT node
        log.printf("2. Connect to remote AT node");
        error = xbee_conNew(xbee, &remote, "Remote AT", &address);
        if (error != XBEE_ENONE) {
            status = XBEE_ERROR;
            return (XBee_Error) log.error(xbee_errorToStr(error),ERR_XBEE_NEW_NODE);
        }

        // 3. Purge remote connection
        log.printf("3. Purge remote connection");
        error = xbee_conPurge(remote);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            return (XBee_Error) log.error(xbee_errorToStr(error),ERR_XBEE_PURGE);
        }

        unsigned char retVal;
        struct xbee_pkt *pkt;

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"ID", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "ID", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "ID");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.ID = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.ID = (telemetry.ID << 8) | pkt->data[q];
                log.printf("ID = 0x%lx", telemetry.ID);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"SC", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SC", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SC");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SC = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SC = (telemetry.SC << 8) | pkt->data[q];
                log.printf("SC = 0x%lx", telemetry.SC);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"SD", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SD", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SD");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SD = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SD = (telemetry.SD << 8) | pkt->data[q];
                log.printf("SD = 0x%lx", telemetry.SD);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"ZS", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "ZS", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "ZS");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.ZS = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.ZS = (telemetry.ZS << 8) | pkt->data[q];
                log.printf("ZS = 0x%lx", telemetry.ZS);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"NJ", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NJ", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NJ");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.NJ = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.NJ = (telemetry.NJ << 8) | pkt->data[q];
                log.printf("NJ = 0x%lx", telemetry.NJ);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"NW", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NW", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NW");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.NW = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.NW = (telemetry.NW << 8) | pkt->data[q];
                log.printf("NW = 0x%lx", telemetry.NW);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"JV", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "JV", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "JV");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.JV = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.JV = (telemetry.JV << 8) | pkt->data[q];
                log.printf("JV = 0x%lx", telemetry.JV);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"JN", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "JN", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "JN");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.JN = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.JN = (telemetry.JN << 8) | pkt->data[q];
                log.printf("JN = 0x%lx", telemetry.JN);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"OP", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "OP", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "OP");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.OP = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.OP = (telemetry.OP << 8) | pkt->data[q];
                log.printf("OP = 0x%lx", telemetry.OP);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"OI", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "OI", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "OI");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.OI = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.OI = (telemetry.OI << 8) | pkt->data[q];
                log.printf("OI = 0x%lx", telemetry.OI);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"CH", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "CH", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "CH");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.CH = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.CH = (telemetry.CH << 8) | pkt->data[q];
                log.printf("CH = 0x%lx", telemetry.CH);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"NC", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NC", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NC");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.NC = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.NC = (telemetry.NC << 8) | pkt->data[q];
                log.printf("NC = 0x%lx", telemetry.NC);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"SH", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SH", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SH");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SH = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SH = (telemetry.SH << 8) | pkt->data[q];
                log.printf("SH = 0x%lx", telemetry.SH);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"SL", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SL", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SL");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SL = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SL = (telemetry.SL << 8) | pkt->data[q];
                log.printf("SL = 0x%lx", telemetry.SL);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"MY", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "MY", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "MY");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.MY = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.MY = (telemetry.MY << 8) | pkt->data[q];
                log.printf("MY = 0x%lx", telemetry.MY);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"DH", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "DH", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "DH");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.DH = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.DH = (telemetry.DH << 8) | pkt->data[q];
                log.printf("DH = 0x%lx", telemetry.DH);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"DL", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "DL", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "DL");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.DL = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.DL = (telemetry.DL << 8) | pkt->data[q];
                log.printf("DL = 0x%lx", telemetry.DL);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"NI", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NI", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NI");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                for (int q = 0; q < pkt->dataLen; q++) telemetry.NI[q] = pkt->data[q];
                log.printf("NI = %s", telemetry.NI);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"NH", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NH", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NH");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.NH = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.NH = (telemetry.NH << 8) | pkt->data[q];
                log.printf("NH = 0x%lx", telemetry.NH);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"BH", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "BH", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "BH");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.BH = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.BH = (telemetry.BH << 8) | pkt->data[q];
                log.printf("BH = 0x%lx", telemetry.BH);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"AR", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "AR", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "AR");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.AR = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.AR = (telemetry.AR << 8) | pkt->data[q];
                log.printf("AR = 0x%lx", telemetry.AR);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"DD", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "DD", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "DD");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.DD = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.DD = (telemetry.DD << 8) | pkt->data[q];
                log.printf("DD = 0x%lx", telemetry.DD);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"NT", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NT", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NT");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.NT = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.NT = (telemetry.NT << 8) | pkt->data[q];
                log.printf("NT = 0x%lx", telemetry.NT);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"NO", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NO", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NO");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.NO = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.NO = (telemetry.NO << 8) | pkt->data[q];
                log.printf("NO = 0x%lx", telemetry.NO);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"NP", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NP", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NP");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.NP = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.NP = (telemetry.NP << 8) | pkt->data[q];
                log.printf("NP = 0x%lx", telemetry.NP);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"CR", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "CR", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "CR");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.CR = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.CR = (telemetry.CR << 8) | pkt->data[q];
                log.printf("CR = 0x%lx", telemetry.CR);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"SE", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SE", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SE");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SE = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SE = (telemetry.SE << 8) | pkt->data[q];
                log.printf("SE = 0x%lx", telemetry.SE);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"DE", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "DE", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "DE");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.DE = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.DE = (telemetry.DE << 8) | pkt->data[q];
                log.printf("DE = 0x%lx", telemetry.DE);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"CI", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "CI", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "CI");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.CI = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.CI = (telemetry.CI << 8) | pkt->data[q];
                log.printf("CI = 0x%lx", telemetry.CI);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"PL", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "PL", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "PL");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.PL = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.PL = (telemetry.PL << 8) | pkt->data[q];
                log.printf("PL = 0x%lx", telemetry.PL);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"PM", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "PM", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "PM");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.PM = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.PM = (telemetry.PM << 8) | pkt->data[q];
                log.printf("PM = 0x%lx", telemetry.PM);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"PP", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "PP", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "PP");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.PP = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.PP = (telemetry.PP << 8) | pkt->data[q];
                log.printf("PP = 0x%lx", telemetry.PP);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"EE", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "EE", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "EE");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.EE = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.EE = (telemetry.EE << 8) | pkt->data[q];
                log.printf("EE = 0x%lx", telemetry.EE);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"EO", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "EO", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "EO");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.EO = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.EO = (telemetry.EO << 8) | pkt->data[q];
                log.printf("EO = 0x%lx", telemetry.EO);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"BD", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "BD", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "BD");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.BD = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.BD = (telemetry.BD << 8) | pkt->data[q];
                log.printf("BD = 0x%lx", telemetry.BD);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"NB", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "NB", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "NB");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.NB = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.NB = (telemetry.NB << 8) | pkt->data[q];
                log.printf("NB = 0x%lx", telemetry.NB);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"SB", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SB", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SB");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SB = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SB = (telemetry.SB << 8) | pkt->data[q];
                log.printf("SB = 0x%lx", telemetry.SB);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"RO", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "RO", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "RO");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.RO = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.RO = (telemetry.RO << 8) | pkt->data[q];
                log.printf("RO = 0x%lx", telemetry.RO);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"D7", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "D7", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "D7");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.D7 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.D7 = (telemetry.D7 << 8) | pkt->data[q];
                log.printf("D7 = 0x%lx", telemetry.D7);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"D6", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "D6", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "D6");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.D6 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.D6 = (telemetry.D6 << 8) | pkt->data[q];
                log.printf("D6 = 0x%lx", telemetry.D6);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"CT", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "CT", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "CT");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.CT = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.CT = (telemetry.CT << 8) | pkt->data[q];
                log.printf("CT = 0x%lx", telemetry.CT);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"GT", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "GT", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "GT");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.GT = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.GT = (telemetry.GT << 8) | pkt->data[q];
                log.printf("GT = 0x%lx", telemetry.GT);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"CC", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "CC", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "CC");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.CC = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.CC = (telemetry.CC << 8) | pkt->data[q];
                log.printf("CC = 0x%lx", telemetry.CC);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"SM", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SM", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SM");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SM = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SM = (telemetry.SM << 8) | pkt->data[q];
                log.printf("SM = 0x%lx", telemetry.SM);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"SN", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SN", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SN");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SN = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SN = (telemetry.SN << 8) | pkt->data[q];
                log.printf("SN = 0x%lx", telemetry.SN);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"SO", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SO", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SO");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SO = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SO = (telemetry.SO << 8) | pkt->data[q];
                log.printf("SO = 0x%lx", telemetry.SO);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"SP", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "SP", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "SP");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.SP = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.SP = (telemetry.SP << 8) | pkt->data[q];
                log.printf("SP = 0x%lx", telemetry.SP);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"ST", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "ST", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "ST");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.ST = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.ST = (telemetry.ST << 8) | pkt->data[q];
                log.printf("ST = 0x%lx", telemetry.ST);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"PO", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "PO", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "PO");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.PO = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.PO = (telemetry.PO << 8) | pkt->data[q];
                log.printf("PO = 0x%lx", telemetry.PO);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"D0", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "D0", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "D0");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.D0 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.D0 = (telemetry.D0 << 8) | pkt->data[q];
                log.printf("D0 = 0x%lx", telemetry.D0);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"D1", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "D1", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "D1");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.D1 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.D1 = (telemetry.D1 << 8) | pkt->data[q];
                log.printf("D1 = 0x%lx", telemetry.D1);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"D2", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "D2", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "D2");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.D2 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.D2 = (telemetry.D2 << 8) | pkt->data[q];
                log.printf("D2 = 0x%lx", telemetry.D2);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"D3", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "D3", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "D3");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.D3 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.D3 = (telemetry.D3 << 8) | pkt->data[q];
                log.printf("D3 = 0x%lx", telemetry.D3);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"D4", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "D4", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "D4");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.D4 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.D4 = (telemetry.D4 << 8) | pkt->data[q];
                log.printf("D4 = 0x%lx", telemetry.D4);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"D5", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "D5", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "D5");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.D5 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.D5 = (telemetry.D5 << 8) | pkt->data[q];
                log.printf("D5 = 0x%lx", telemetry.D5);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"P0", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "P0", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "P0");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.P0 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.P0 = (telemetry.P0 << 8) | pkt->data[q];
                log.printf("P0 = 0x%lx", telemetry.P0);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"P1", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "P1", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "P1");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.P1 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.P1 = (telemetry.P1 << 8) | pkt->data[q];
                log.printf("P1 = 0x%lx", telemetry.P1);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"P2", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "P2", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "P2");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.P2 = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.P2 = (telemetry.P2 << 8) | pkt->data[q];
                log.printf("P2 = 0x%lx", telemetry.P2);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"PR", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "PR", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "PR");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.PR = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.PR = (telemetry.PR << 8) | pkt->data[q];
                log.printf("PR = 0x%lx", telemetry.PR);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"LT", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "LT", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "LT");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.LT = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.LT = (telemetry.LT << 8) | pkt->data[q];
                log.printf("LT = 0x%lx", telemetry.LT);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"RP", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "RP", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "RP");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.RP = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.RP = (telemetry.RP << 8) | pkt->data[q];
                log.printf("RP = 0x%lx", telemetry.RP);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"DO", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "DO", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "DO");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.DO = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.DO = (telemetry.DO << 8) | pkt->data[q];
                log.printf("DO = 0x%lx", telemetry.DO);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"IR", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "IR", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "IR");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.IR = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.IR = (telemetry.IR << 8) | pkt->data[q];
                log.printf("IR = 0x%lx", telemetry.IR);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"IC", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "IC", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "IC");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.IC = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.IC = (telemetry.IC << 8) | pkt->data[q];
                log.printf("IC = 0x%lx", telemetry.IC);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"V+", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "V+", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "V+");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.Vp = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.Vp = (telemetry.Vp << 8) | pkt->data[q];
                log.printf("V+ = 0x%lx", telemetry.Vp);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"VR", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "VR", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "VR");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.VR = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.VR = (telemetry.VR << 8) | pkt->data[q];
                log.printf("VR = 0x%lx", telemetry.VR);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"HV", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "HV", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "HV");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.HV = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.HV = (telemetry.HV << 8) | pkt->data[q];
                log.printf("HV = 0x%lx", telemetry.HV);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"AI", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "AI", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "AI");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.AI = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.AI = (telemetry.AI << 8) | pkt->data[q];
                log.printf("AI = 0x%lx", telemetry.AI);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"DB", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "DB", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "DB");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.DB = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.DB = (telemetry.DB << 8) | pkt->data[q];
                log.printf("DB = 0x%lx", telemetry.DB);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }

        error = xbee_connTx(remote, &retVal, (const unsigned char *)"%V", 2);
        if (error != XBEE_ENONE){
            status = XBEE_ERROR;
            log.printf("retVal = %i",retVal);
            log.error(xbee_errorToStr(error),ERR_XBEE_SEND);
        }
        else if (retVal == 0){
            error = xbee_conRx(remote, &pkt, NULL);
            if (error != XBEE_ENONE){
                status = XBEE_ERROR;
                log.error(xbee_errorToStr(error),ERR_XBEE_RECEIVE_PKT);
            }
            else if (pkt->status != 0){
                status = XBEE_ERROR;
                log.printf("pkt status = %d", pkt->status);
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_ERROR);
            }
            else if (strncasecmp((const char*)pkt->atCommand, "%V", 2)){
                status = XBEE_ERROR;
                log.printf("pkt command = %s (should be %s)", pkt->atCommand, "%V");
                log.error(xbee_errorToStr(error),ERR_XBEE_AT_MISMATCH);
            }
            else if (pkt->dataLen > 0){
                telemetry.V = pkt->data[0];
                for (int q = 1; q < pkt->dataLen; q++) telemetry.V = (telemetry.V << 8) | pkt->data[q];
                log.printf("%V = 0x%lx", telemetry.V);
            }
            if ((error = xbee_pktFree(pkt)) != XBEE_ENONE) log.error(xbee_errorToStr(error),ERR_XBEE_FREE_PKT);
        }



        // 4. Close remote connection
        log.printf("4. Close remote connection");
        error = xbee_conEnd(remote);
        if (error != XBEE_ENONE) {
            status = XBEE_ERROR;
            return (XBee_Error) log.error(xbee_errorToStr(error),ERR_XBEE_END_NODE);
        }

        return (XBee_Error) log.success();
    }
    catch(  const std::exception& e  ){
        status = XBEE_ERROR;
        return (XBee_Error) log.error(e.what(), ERR_XBEE_TELEMETRY_FATAL);
    }
}
