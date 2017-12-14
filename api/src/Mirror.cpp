/***************************************************************************//**
 * @file	Mirror.cpp
 * @brief	Source file to operate the Mirrors
 *
 * This file contains all the implementations for the functions defined in:
 * api/include/Mirror.hpp
 *
 * @author	Thibaud Talon
 * @date	11/15/2017
 *******************************************************************************/

#include "Mirror.hpp"
#include "UserInterface.hpp"

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   11/15/2017
 *
 * Connect the Mirror
 *
 * @param [in] _addr64
 *	MAC address of the Mirror XBee
 * @param [in] *_xbee
 *	Pointer to the XBee object
 ******************************************************************************/
Mirror_Error Mirror::connect(uint64_t _addr64, XBee *_xbee){
    UserInterface::Log log("Mirror::connect");
    try
    {
        status = MIRROR_OFF;

        // 1. Check XBee connection
        log.printf("1. Check XBee connection");
        if ((*_xbee).status == XBEE_OFF){
            if((*_xbee).reset()){
                status = MIRROR_ERROR;
                return (Mirror_Error) log.error("Cannot connect XBee", ERR_MIRROR_XBEE_CONNECT);
            }
        }
        xbee = _xbee;

        // 2. Connect node
        log.printf("2. Connect node");
        log.printf("Address = 0x%lx Hz",_addr64);
        if((*xbee).connectNode(node, _addr64)){
            status = MIRROR_ERROR;
            return (Mirror_Error) log.error("Cannot connect node", ERR_MIRROR_XBEENODE_CONNECT);
        }

        // 3. Ping
        Mirror_Error error = ping();
        if(error){
            status = MIRROR_ERROR;
            return (Mirror_Error) log.error("Error pinging", ERR_MIRROR_CONNECT_PING);
        }

        status = MIRROR_ON;
        return (Mirror_Error) log.success();
    }
    catch(  const std::exception& e  ){
        status = MIRROR_ERROR;
        return (Mirror_Error) log.error(e.what(), ERR_MIRROR_CONNECT_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   11/15/2017
 *
 * Disconnect the Mirror
 *
 ******************************************************************************/
Mirror_Error Mirror::disconnect(void){
    UserInterface::Log log("Mirror::disconnect");
    try
    {
        if((*xbee).disconnectNode(node)){
            status = MIRROR_ERROR;
            return (Mirror_Error) log.error("Cannot connect node", ERR_MIRROR_XBEENODE_DISCONNECT);
        }

        status = MIRROR_OFF;

        return (Mirror_Error) log.success();
    }
    catch(  const std::exception& e  ){
        status = MIRROR_ERROR;
        return (Mirror_Error) log.error(e.what(), ERR_MIRROR_DISCONNECT_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   11/15/2017
 *
 * Send command to the Mirror
 *
 * @param [in] command
 *	Index of the command
 * @param [in] action
 *	Action: 0 = Add task, 1 = Delete task, 2 = Get status
 * @param [in] period
 *	Period of task: 0 = execute once, >0 = execute task every period*0.1 sec
 * @param [in] data
 *	Data with command
 * @param [in] timeout
 *	Timeout to receive acknowledgment
 ******************************************************************************/
Mirror_Error Mirror::command(uint8_t cmd, uint8_t action, uint8_t period, uint32_t data, unsigned int timeout = MIRROR_TIMEOUT){
    UserInterface::Log log("Mirror::command");
    try
    {
        int message_len = 8;

        log.printf("1. Format message");
        unsigned char buffer[message_len];
        log.printf("Command = %i", cmd);
        buffer[0] = cmd;
        log.printf("Action = %i", action);
        buffer[1] = action;
        log.printf("Period = %i", period);
        buffer[2] = period;
        buffer[3] = (data >> 24) & 0xFF;
        buffer[4] = (data >> 16) & 0xFF;
        buffer[5] = (data >> 8) & 0xFF;
        buffer[6] = (data) & 0xFF;


        log.printf("2. Calculate checksum");
        int checksum = 0;
        for(int II=0; II<message_len-1; II++) checksum += buffer[II];
        checksum &= 0xFF;
        buffer[7] = 0xFF - checksum;

        log.printf("3. Send Message = %x %x %x %x %x %x %x %x", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);
        if((*xbee).send(node, buffer, message_len)){
            status = MIRROR_ERROR;
            return (Mirror_Error) log.error("Error sending message", ERR_MIRROR_SEND);
        }


        log.printf("4. Receive acknowledgment");
        unsigned char *msg;
        int len;
        if((*xbee).receive(node, &msg, &len, timeout)){
            status = MIRROR_ERROR;
            return (Mirror_Error) log.error("Error receiving message", ERR_MIRROR_RECEIVE);
        }
        log.printf("Length of received message = %i", len);
        if (len < message_len){
            status = MIRROR_ERROR;
            return (Mirror_Error) log.error("Wrong length of received message",ERR_MIRROR_RECEIVED_LEN);
        }
        log.printf("Received message = %x %x %x %x %x %x %x %x", msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6], msg[7]);


        log.printf("5. Checksum verification");
        checksum = 0;
        for(int II=len-6; II<len; II++) checksum += msg[II];
        if ((checksum & 0xFF) != 0xFF){
            status = MIRROR_ERROR;
            return (Mirror_Error) log.error("Checksum incorrect",ERR_MIRROR_RECEIVED_CHECKSUM);
        }


        log.printf("6. Error checking");
        if(msg[0] != cmd){
            status = MIRROR_ERROR;
            return (Mirror_Error) log.error("Error in received command", ERR_MIRROR_RECEIVED_COMMAND);
        }
        Mirror_Error error = (Mirror_Error) msg[6];
        if (error) return (Mirror_Error) log.error("Error in excecuting command", error);


        return (Mirror_Error) log.success();
    }
    catch(  const std::exception& e  ){
        status = MIRROR_ERROR;
        return (Mirror_Error) log.error(e.what(), ERR_MIRROR_COMMAND_FATAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   11/15/2017
 *
 * Ping the Mirror
 *
 ******************************************************************************/
Mirror_Error Mirror::ping(void){
    UserInterface::Log log("Mirror::ping");
    try
    {
        Mirror_Error error = command(255, 0, 0, 0, 1);
        if(error == OK_MIRROR) return (Mirror_Error) log.success();
        if (error == 1){
            return (Mirror_Error) log.error("Mirror in bootloader mode", ERR_MIRROR_PING_BOOTLOADER);
        }
        else{
            status = MIRROR_ERROR;
            return (Mirror_Error) log.error("Unknown error", ERR_MIRROR_PING_UNKNOWN);
        }
    }
    catch(  const std::exception& e  ){
        status = MIRROR_ERROR;
        return (Mirror_Error) log.error(e.what(), ERR_MIRROR_PING_CRITICAL);
    }
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   11/15/2017
 *
 * Write to the Mirror register
 *
 * @param [in] address
 *	Address to write in the register
 * @param [in] value
 *	Value to write in the register
 ******************************************************************************/
Mirror_Error Mirror::writeRegister(uint8_t address, int32_t value){
    return command(address, 0, 0, value, 1);
}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   11/15/2017
 *
 * Read the Mirror register
 *
 * @param [in] address
 *	Address to read in the register
 * @param [out] value
 *	Value read from to the register
 ******************************************************************************/
Mirror_Error Mirror::readRegister(uint8_t address, int32_t &value){
    UserInterface::Log log("Mirror::readRegister");
    try
    {
        int message_len = 8;

        log.printf("1. Format message");
        unsigned char buffer[message_len];
        buffer[0] = 150;
        buffer[1] = 0;
        buffer[2] = 0;
        buffer[3] = 0;
        buffer[4] = 0;
        buffer[5] = 0;
        buffer[6] = address;


        log.printf("2. Calculate checksum");
        int checksum = 0;
        for(int II=0; II<message_len-1; II++) checksum += buffer[II];
        checksum &= 0xFF;
        buffer[7] = 0xFF - checksum;

        log.printf("3. Send Message = %x %x %x %x %x %x %x %x", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);
        if((*xbee).send(node, buffer, message_len)){
            status = MIRROR_ERROR;
            return (Mirror_Error) log.error("Error sending message", ERR_MIRROR_SEND);
        }


        log.printf("4. Receive acknowledgment");
        unsigned char *msg;
        int len;
        if((*xbee).receive(node, &msg, &len, 1)){
            status = MIRROR_ERROR;
            return (Mirror_Error) log.error("Error receiving message", ERR_MIRROR_RECEIVE);
        }
        log.printf("Length of received message = %i", len);
        if (len < message_len){
            status = MIRROR_ERROR;
            return (Mirror_Error) log.error("Wrong length of received message",ERR_MIRROR_RECEIVED_LEN);
        }
        log.printf("Received message = %x %x %x %x %x %x %x %x", msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6], msg[7]);


        log.printf("5. Checksum verification");
        checksum = 0;
        for(int II=len-6; II<len; II++) checksum += msg[II];
        if ((checksum & 0xFF) != 0xFF){
            status = MIRROR_ERROR;
            return (Mirror_Error) log.error("Checksum incorrect",ERR_MIRROR_RECEIVED_CHECKSUM);
        }


        log.printf("6. Error checking");
        if(msg[0] != address){
            status = MIRROR_ERROR;
            return (Mirror_Error) log.error("Error in received command", ERR_MIRROR_RECEIVED_COMMAND);
        }
        value = (int32_t)((uint32_t)msg[3]<<24) | ((uint32_t)msg[4]<<16) | ((uint32_t)msg[5]<<8) | ((uint32_t)msg[6]);

        return (Mirror_Error) log.success();
    }
    catch(  const std::exception& e  ){
        status = MIRROR_ERROR;
        return (Mirror_Error) log.error(e.what(), ERR_MIRROR_READREGISTER_FATAL);
    }

}

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   11/15/2017
 *
 * Move a picomotor
 *
 * @param [in] picoID
 *	ID of the picomotor: 0 = top-left, 1 = top-right, 2 = bottom
 * @param [in] distance
 *	Distance to move (units depends on mode)
 * @param [in] mode
 *	Mode: 0 = move by ticks (open loop),
 *            1 = move by intervals (feedback)
 *            2 = move by nm (precise positionning)
 ******************************************************************************/
Mirror_Error Mirror::movePicomotor(PicomotorID picoID, int32_t distance, PicomotorMode mode){
    UserInterface::Log log("Mirror::movePicomotor");
    try
    {
        Mirror_Error error;

        // 1. Turn ON picomotor HV
        log.printf("1. Turn ON picomotor HV");
        error = command(163, 0, 0, 0, 10);
        if (error){
            status = MIRROR_ERROR;
            return (Mirror_Error) log.error("Error turning ON picomotor HV", ERR_MIRROR_MOVEPICOMOTOR_HV_ON);
        }

        // 2. Move picomotor
        char * mode_text;
        if (mode == moveByTicks) mode_text = "ticks";
        if (mode == moveByInterval) mode_text = "intervals";
        if (mode == moveByAbsolutePosition) mode_text = "nm";
        log.printf("2. Move picomotor by %i %s", distance, mode_text);
        uint8_t cmd = 180 + 10*picoID + mode;
        int time = 0.005*distance;
        if (mode == moveByInterval) time *= 30;
        time += 5;
        error = command(cmd, 0, 0, (uint32_t)distance, time);
        if (error){
            status = MIRROR_ERROR;
            log.error("Error turning ON picomotor HV", ERR_MIRROR_MOVEPICOMOTOR);
        }

        // 3. Turn OFF picomotor HV
        error = command(164, 0, 0, 0, 1);
        if (error){
            status = MIRROR_ERROR;
            return (Mirror_Error) log.error("Error turning OFF picomotor HV", ERR_MIRROR_MOVEPICOMOTOR_HV_OFF);
        }

        return (Mirror_Error) log.success();
    }
    catch(  const std::exception& e  ){
        status = MIRROR_ERROR;
        return (Mirror_Error) log.error(e.what(), ERR_MIRROR_MOVEPICOMOTOR_FATAL);
    }
}
