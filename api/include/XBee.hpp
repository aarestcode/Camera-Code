/***************************************************************************//**
 * @file	XBee.hpp
 * @brief	Header file to operate the XBee
 *
 * This header file contains all the required definitions and function prototypes
 * through which to control the AAReST camera XBee
 *
 * @author	Thibaud Talon
 * @date	25/09/2017
 *******************************************************************************/

#ifndef XBEE_H
#define XBEE_H

#include <xbee.h>
#include <stdint.h>

#define XBEE_MAX_NODES 4

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   25/09/2017
 *
 * Parameters
 ******************************************************************************/
#ifndef OK
#define OK 0
#endif

enum XBee_Error{
    OK_XBEE = 0,

    ERR_XBEE_CONNECT_USB,
    ERR_XBEE_CONNECT_FATAL,
    ERR_XBEE_DISCONNECT,
    ERR_XBEE_DISCONNECT_FATAL,

    ERR_XBEE_NO_DEVICE,

    ERR_XBEE_TOO_MANY_NODES,
    ERR_XBEE_NEW_NODE,
    ERR_XBEE_NODE_DATASET,
    ERR_XBEE_CONNECT_NODE_FATAL,

    ERR_XBEE_END_NODE,
    ERR_XBEE_DISCONNECT_NODE_FATAL,

    ERR_XBEE_NO_CONNECTION,

    ERR_XBEE_SEND_LEN,
    ERR_XBEE_SEND,
    ERR_XBEE_SEND_FATAL,

    ERR_XBEE_RECEIVE_TIMEOUT,
    ERR_XBEE_RECEIVE_PKT,
    ERR_XBEE_FREE_PKT,
    ERR_XBEE_RECEIVE_FATAL,

    ERR_XBEE_PURGE,
    ERR_XBEE_PURGE_FATAL,

    ERR_XBEE_AT_ERROR,
    ERR_XBEE_AT_MISMATCH,
    ERR_XBEE_TELEMETRY_FATAL,
};

enum XBee_Status{
    XBEE_ON = 0,
    XBEE_OFF = 1,
    XBEE_ERROR = 2,
};

struct XBeeNode{
    XBee_Status status;
    uint64_t address;
    int countTx;
    int countRx;

    xbee_err error; // Last error
    struct xbee_con *con;
};

/***************************************************************************//**
 * @author Thibaud Talon
 * @date   28/09/2017
 *
 * Telemetry
 ******************************************************************************/
struct XBee_Telemetry{
    uint64_t ID;
    uint16_t SC;
    uint8_t SD;
    uint8_t ZS;
    uint8_t NJ;
    uint64_t OP;
    uint16_t OI;
    uint8_t CH;
    int NC;
    uint32_t SH;
    uint32_t SL;
    uint16_t MY;
    uint32_t DH;
    uint32_t DL;
    char NI[20];
    uint8_t NH;
    uint8_t BH;
    uint8_t AR;
    uint32_t DD;
    uint8_t NT;
    uint8_t NO;
    uint16_t NP;
    uint8_t CR;
    uint8_t PL;
    uint8_t PM;
    uint8_t PP;
    uint8_t EE;
    uint8_t EO;
    uint8_t BD;
    uint8_t NB;
    uint8_t SB;
    uint8_t D7;
    uint8_t D6;
    uint8_t AP;
    uint8_t AO;
    uint8_t SP;
    uint16_t SN;
    uint8_t D0;
    uint8_t D1;
    uint8_t D2;
    uint8_t D3;
    uint8_t D4;
    uint8_t D5;
    uint8_t P0;
    uint8_t P1;
    uint8_t P2;
    uint16_t PR;
    uint8_t LT;
    uint8_t RP;
    uint8_t DO;
    uint16_t IR;
    uint16_t IC;
    uint16_t Vp;
    uint16_t VR;
    uint16_t HV;
    uint8_t AI;
    uint8_t DB;
    uint16_t V;
};
struct XBeeNode_Telemetry{
    uint64_t ID;
    uint16_t SC;
    uint8_t SD;
    uint8_t ZS;
    uint8_t NJ;
    uint16_t NW;
    uint8_t JV;
    uint8_t JN;
    uint64_t OP;
    uint16_t OI;
    uint8_t CH;
    int NC;
    uint32_t SH;
    uint32_t SL;
    uint16_t MY;
    uint32_t DH;
    uint32_t DL;
    char NI[20];
    uint8_t NH;
    uint8_t BH;
    uint8_t AR;
    uint32_t DD;
    uint8_t NT;
    uint8_t NO;
    uint16_t NP;
    uint8_t CR;
    uint8_t SE;
    uint8_t DE;
    uint16_t CI;
    uint8_t PL;
    uint8_t PM;
    uint8_t PP;
    uint8_t EE;
    uint8_t EO;
    uint8_t BD;
    uint8_t NB;
    uint8_t SB;
    uint8_t RO;
    uint8_t D7;
    uint8_t D6;
    uint16_t CT;
    uint16_t GT;
    uint8_t CC;
    uint8_t SM;
    uint16_t SN;
    uint8_t SO;
    uint8_t SP;
    uint16_t ST;
    uint16_t PO;
    uint8_t D0;
    uint8_t D1;
    uint8_t D2;
    uint8_t D3;
    uint8_t D4;
    uint8_t D5;
    uint8_t P0;
    uint8_t P1;
    uint8_t P2;
    uint16_t PR;
    uint8_t LT;
    uint8_t RP;
    uint8_t DO;
    uint16_t IR;
    uint16_t IC;
    uint16_t Vp;
    uint16_t VR;
    uint16_t HV;
    uint8_t AI;
    uint8_t DB;
    uint16_t V;
};
/***************************************************************************//**
 * @author Thibaud Talon
 * @date   25/09/2017
 *
 * Class
 ******************************************************************************/
class XBee {
public:
    XBee_Status status; // Status of Xbee
    xbee_err error; // Last error

    XBee(void) {status = XBEE_OFF;} // Create object without connecting
    XBee (int baudrate) {connect(baudrate);} // Create object and connect
    ~XBee() {disconnect();} // Destruct the object safely
    XBee_Error connect(int baudrate); // Connect the XBee module
    XBee_Error disconnect(void); // Disconnect the XBee module
    XBee_Error reset(void); // Reset the connection to the XBee module

    XBee_Error connectNode(XBeeNode & handle, uint64_t addr64);
    XBee_Error disconnectNode(XBeeNode & handle);
    XBee_Error resetNode(XBeeNode & handle);

    XBee_Error send(XBeeNode & handle, const unsigned char *msg, unsigned int len);
    XBee_Error receive(XBeeNode & handle, unsigned char **msg, int *len, int timeout);
    XBee_Error purge(XBeeNode & handle);

    XBee_Error getNodesCount(int & nodes_count);  // Get current number of connected nodes
    XBee_Error getBaudrate(int & baudrate);  // Get baudrate to Camera XBee

    XBee_Error getTelemetry(XBee_Telemetry & telemetry);
    XBee_Error getNodeTelemetry(XBeeNode & handle, XBeeNode_Telemetry & telemetry);

private:
    int _baudrate;
    XBeeNode nodes[XBEE_MAX_NODES]; // Pointer to all the created connections to remote XBees
    struct xbee *xbee;
};

#endif
