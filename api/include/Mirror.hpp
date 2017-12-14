/***************************************************************************//**
 * @file	Mirror.hpp
 * @brief	Header file to operate the AAReST Mirrors
 *
 * This header file contains all the required definitions and function prototypes
 * through which to control the AAReST Mirrors
 *
 * @author	Thibaud Talon
 * @date	25/09/2017
 *******************************************************************************/
#ifndef MIRROR_H
#define MIRROR_H

#include <math.h>
#include <opencv2/core/core.hpp>
#include "XBee.hpp"

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   25/09/2017
 *
 * Parameters
 ******************************************************************************/
#ifndef OK
#define OK 0
#endif

#define MIRROR_TIMEOUT 30

enum Mirror_Error{
    OK_MIRROR = 0,

    ERR_MIRROR_ = 255,

    ERR_MIRROR_XBEE_CONNECT,
    ERR_MIRROR_XBEENODE_CONNECT,
    ERR_MIRROR_CONNECT_PING,
    ERR_MIRROR_CONNECT_FATAL,
    ERR_MIRROR_XBEENODE_DISCONNECT,
    ERR_MIRROR_DISCONNECT_FATAL,
    ERR_MIRROR_SEND,
    ERR_MIRROR_RECEIVE,
    ERR_MIRROR_RECEIVED_LEN,
    ERR_MIRROR_RECEIVED_CHECKSUM,
    ERR_MIRROR_RECEIVED_COMMAND,
    ERR_MIRROR_COMMAND_FATAL,
    ERR_MIRROR_PING_BOOTLOADER,
    ERR_MIRROR_PING_UNKNOWN,
    ERR_MIRROR_PING_CRITICAL,
    ERR_MIRROR_READREGISTER_FATAL,
    ERR_MIRROR_MOVEPICOMOTOR_HV_ON,
    ERR_MIRROR_MOVEPICOMOTOR,
    ERR_MIRROR_MOVEPICOMOTOR_HV_OFF,
    ERR_MIRROR_MOVEPICOMOTOR_FATAL,

};

enum Mirror_Status{
    MIRROR_ON = 0,
    MIRROR_OFF = 1,
    MIRROR_ERROR = 2,
};

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   25/09/2017
 *
 * Picomotors
 ******************************************************************************/
enum PicomotorID{
    picomotor1,
    picomotor2,
    picomotor3
};
enum PicomotorMode{
    moveByTicks,
    moveByInterval,
    moveByAbsolutePosition,
};

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   25/09/2017
 *
 * Voltage object
 ******************************************************************************/
#define DM_REFRESH_TIME 10 // Refresh time of an electrode in ms
class voltage_t {
public:
  double voltage;
  unsigned int next_time_ms;
  unsigned int time_ms;

  voltage_t(const double _voltage = 0, const unsigned int _next_time_ms = DM_REFRESH_TIME, const unsigned int _time_ms = DM_REFRESH_TIME){
      voltage = _voltage;
      next_time_ms = _next_time_ms;
      time_ms = _time_ms;
  }
} ;



/***************************************************************************//**
 * @author Thibaud Talon
 * @date   25/09/2017
 *
 * Class
 ******************************************************************************/
class Mirror {
  public:
    Mirror_Status status; // Status of mirror

    Mirror(void) {status = MIRROR_OFF;}
    Mirror(uint64_t _addr64, XBee * _xbee) {connect(_addr64, _xbee);}
    ~Mirror(void) {disconnect();}
    Mirror_Error connect(uint64_t _addr64, XBee *_xbee);
    Mirror_Error disconnect(void);

    Mirror_Error command(uint8_t command, uint8_t action, uint8_t period, uint32_t data, unsigned int timeout);

    /* APPLICATION */
    Mirror_Error ping(void);
    Mirror_Error writeRegister(uint8_t address, int32_t value);
    Mirror_Error readRegister(uint8_t address, int32_t &value);
    Mirror_Error movePicomotor(PicomotorID picoID, int32_t distance, PicomotorMode mode);
//    Mirror_Error actuateElectrode (int electrode_id, voltage_t voltageCommand);
//    Mirror_Error readElectrode (int electrode_id, voltage_t &voltageCommand);

//    Mirror_Error loadCode(int partition, const char* binaryFile);

    /* BOOTLOADER */
//    Mirror_Error bootloaderPing(void);
//    Mirror_Error bootloaderUartSpareInit(int baudrate);
//    Mirror_Error bootloaderUartXBeeInit(int baudrate);
//    Mirror_Error bootloaderI2CInit(int frequency);
//    Mirror_Error bootloaderI2CTest(uint8_t address);
//    Mirror_Error bootloaderReadRegister(void /*To change*/);
//    Mirror_Error bootloaderReadFlash(uint32_t &Dword);
//    Mirror_Error bootloaderReadEEPROM(int partition, uint32_t &Dword);
//    Mirror_Error bootloaderStart(void);
//    Mirror_Error bootloaderLoadCodeFromEEPROM(int partition);
//    Mirror_Error bootloaderLoadCodeFromUART(const char * binaryFile);

private:
    XBee * xbee;
    XBeeNode node;
};

#endif
