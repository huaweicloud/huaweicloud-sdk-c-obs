/*
 * Copyright (c) 2004-2012 Mellanox Technologies LTD. All rights reserved.
 *
 * This software is available to you under the terms of the
 * OpenIB.org BSD license included below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */


#ifndef IBIS_TYPES_H_
#define IBIS_TYPES_H_

#include <stdlib.h>

#include <infiniband/misc/tool_trace/tool_trace.h>

#include "packets/packets_layouts.h"


/****************************************************/
/* new types */
typedef struct direct_route {
    DirRPath_Block_Element path;
    u_int8_t length;
} direct_route_t;


typedef int (*pack_data_func_t)(void *data_to_pack,
                                u_int8_t *packed_buffer);
typedef void (*unpack_data_func_t)(void *data_to_unpack,
                                    u_int8_t *unpacked_buffer);
typedef void (*dump_data_func_t)(void *data_to_dump,
                                    FILE * out_port);

typedef struct local_port {
    u_int64_t guid;
    u_int16_t lid;
    u_int8_t logical_state;
} local_port_t;


/****************************************************/
/* General Macros */
#ifndef CLEAR_STRUCT
    #define CLEAR_STRUCT(n) memset(&(n), 0, sizeof(n))
#endif

#define arrsize(a) (sizeof(a)/sizeof(a[0]))

#ifndef IN
    #define IN
#endif
#ifndef OUT
    #define OUT
#endif
#ifndef INOUT
    #define INOUT
#endif


/****************************************************/
/* Ibis defines + enums */
#define IBIS_TIMEOUT                500     /* 0.5 second */
#define IBIS_RETRIES                2       /* 2 tries */
#define IBIS_ERR_SIZE               4096    /* 4096 bytes */
#define IBIS_MAX_CAS                32
#define IBIS_MAX_PORTS_PER_CA       2
#define IBIS_MAX_LOCAL_PORTS        96      /* IBIS_MAX_CAS * (IBIS_MAX_PORTS_PER_CA + 1) = 96 */


/****************************************************/
/* Infiniband defines + enums */
#define IBIS_IB_BASE_VERSION                0x1
#define IBIS_IB_MAD_SIZE                    256
#define IBIS_IB_MAX_MAD_CLASSES             256
#define IBIS_IB_DEFAULT_QP1_QKEY            0x80010000

#define IBIS_IB_MAD_SMP_LFT_NUM_BLOCKS          64
#define IBIS_IB_MAD_SMP_MFT_NUM_BLOCKS          32
#define IBIS_IB_MAD_SMP_MFT_PORT_MASK_SIZE      16
#define IBIS_IB_MAD_SMP_PKEY_TABLE_NUM_BLOCKS   32
#define IBIS_IB_MAD_SMP_GUIDS_TABLE_NUM_BLOCKS  8

#define IBIS_IB_PORT_CAP_HAS_EXT_SPEEDS     0x00004000

#define IBIS_IB_PORT_PHY_STATE_ACTIVE       5

#define IBIS_IB_SM_STATE_NOT_ACTIVE         0
#define IBIS_IB_SM_STATE_DISCOVER           1
#define IBIS_IB_SM_STATE_STANDBY            2
#define IBIS_IB_SM_STATE_MASTER             3

//#define IBIS_IB_DEFAULT_SUBN_PREFIX  0xfe80000000000000ULL
//#define IBIS_IB_DEFAULT_QP1_QKEY 0x80010000
//#define IBIS_IB_VENDOR_RANGE1_DATA_OFFS  24
//#define IBIS_IB_VENDOR_RANGE1_DATA_SIZE  (IB_MAD_SIZE - IB_VENDOR_RANGE1_DATA_OFFS)
//#define IBIS_IB_VENDOR_RANGE2_DATA_OFFS  40
//#define IBIS_IB_VENDOR_RANGE2_DATA_SIZE  (IB_MAD_SIZE - IB_VENDOR_RANGE2_DATA_OFFS)


enum MAD_CLASSES {
    IBIS_IB_CLASS_SMI = 0x1,
    IBIS_IB_CLASS_SMI_DIRECT = 0x81,
    IBIS_IB_CLASS_SA = 0x3,
    IBIS_IB_CLASS_PERFORMANCE = 0x4,
    IBIS_IB_CLASS_BOARD_MGMT = 0x5,
    IBIS_IB_CLASS_DEVICE_MGMT = 0x6,
    IBIS_IB_CLASS_CM = 0x7,
    IBIS_IB_CLASS_SNMP = 0x8,
    IBIS_IB_CLASS_VENDOR_RANGE1_START = 0x9,
    IBIS_IB_CLASS_VENDOR_RANGE1_END = 0x0f,
    IBIS_IB_CLASS_CC= 0x21,
    IBIS_IB_CLASS_VENDOR_RANGE2_START = 0x30,
    IBIS_IB_CLASS_VENDOR_RANGE2_END = 0x4f,
    IBIS_IB_CLASS_VENDOR_MELLANOX = 0x0a
};


enum MAD_METHODS {
    IBIS_IB_MAD_METHOD_GET = 0x1,
    IBIS_IB_MAD_METHOD_SET = 0x2,
    IBIS_IB_MAD_METHOD_GET_RESPONSE = 0x81,
    IBIS_IB_MAD_METHOD_SEND = 0x3,
    IBIS_IB_MAD_METHOD_TRAP = 0x5,
    IBIS_IB_MAD_METHOD_TRAP_REPRESS = 0x7,
    IBIS_IB_MAD_METHOD_REPORT = 0x6,
    IBIS_IB_MAD_METHOD_REPORT_RESPONSE = 0x86,
    IBIS_IB_MAD_METHOD_GET_TABLE = 0x12,
    IBIS_IB_MAD_METHOD_GET_TABLE_RESPONSE = 0x92,
    IBIS_IB_MAD_METHOD_GET_TRACE_TABLE = 0x13,
    IBIS_IB_MAD_METHOD_GET_TRACE_TABLE_RESPONSE = 0x93,
    IBIS_IB_MAD_METHOD_GETMULTI = 0x14,
    IBIS_IB_MAD_METHOD_GETMULTI_RESPONSE = 0x94,
    IBIS_IB_MAD_METHOD_DELETE = 0x15,
    IBIS_IB_MAD_METHOD_DELETE_RESPONSE = 0x95,
    IBIS_IB_MAD_RESPONSE = 0x80
};


enum MAD_DATA_OFFSETS {
    IBIS_IB_DATA_OFFSET_SMP = 64,
    IBIS_IB_LOG_DATA_OFFSET_CC = 32,
	IBIS_IB_MGT_DATA_OFFSET_CC = 64,
	IBIS_IB_DATA_OFFSET_PERFORMANCE = 64,
    IBIS_IB_DATA_OFFSET_VENDOR_MELLANOX = 32
};

enum MAD_STATUS {
    IBIS_MAD_STATUS_SUCCESS = 0x0000,
    IBIS_MAD_STATUS_BUSY = 0x0001,
    IBIS_MAD_STATUS_REDIRECT = 0x0002,
    IBIS_MAD_STATUS_UNSUP_CLASS_VER = 0x0004,
    IBIS_MAD_STATUS_UNSUP_METHOD = 0x0008,
    IBIS_MAD_STATUS_UNSUP_METHOD_ATTR = 0x000C,
    IBIS_MAD_STATUS_INVALID_FIELD = 0x001C,
    IBIS_MAD_STATUS_SEND_FAILED = 0x00fc,       //umad_send failure
    IBIS_MAD_STATUS_RECV_FAILED = 0x00fd,       //umad_recv failure
    IBIS_MAD_STATUS_TIMEOUT = 0x00fe,           //timeout expired
    IBIS_MAD_STATUS_GENERAL_ERR = 0x00ff        //general error
};

enum RMPP_TYPE_ENUM {
    IBIS_IB_RMPP_TYPE_NONE,
    IBIS_IB_RMPP_TYPE_DATA,
    IBIS_IB_RMPP_TYPE_ACK,
    IBIS_IB_RMPP_TYPE_STOP,
    IBIS_IB_RMPP_TYPE_ABORT
};


enum SMP_ATTR_ID {
    IBIS_IB_ATTR_SMP_NODE_DESC = 0x10,
    IBIS_IB_ATTR_SMP_NODE_INFO = 0x11,
    IBIS_IB_ATTR_SMP_SWITCH_INFO = 0x12,
    IBIS_IB_ATTR_SMP_GUID_INFO = 0x14,
    IBIS_IB_ATTR_SMP_PORT_INFO = 0x15,
    IBIS_IB_ATTR_SMP_PKEY_TBL = 0x16,
    IBIS_IB_ATTR_SMP_SLVL_TABLE = 0x17,
    IBIS_IB_ATTR_SMP_VL_ARBITRATION = 0x18,
    IBIS_IB_ATTR_SMP_LINEARFORWTBL = 0x19,
    IBIS_IB_ATTR_SMP_MULTICASTFORWTBL = 0x1b,
    IBIS_IB_ATTR_SMP_LINKSPEEDWIDTHPAIRSTBL = 0x1c,
    IBIS_IB_ATTR_SMP_VENDORMADSTBL = 0x1d,
    IBIS_IB_ATTR_SMP_SMINFO = 0x20,
    IBIS_IB_ATTR_SMP_MLNXEXTPORTINFO = 0xff90,          /* Mellanox vendor specific MAD -
                                                                Extended port info */
    IBIS_IB_ATTR_SMP_LAST
};


enum PERFORMANCE_MANAGEMENT_ATTR_ID {
    IBIS_IB_ATTR_PERF_MANAGEMENT_PORT_SAMPLES_CONTROL = 0x10,
    IBIS_IB_ATTR_PERF_MANAGEMENT_PORT_SAMPLES_RESULT = 0x11,
    IBIS_IB_ATTR_PERF_MANAGEMENT_PORT_COUNTERS = 0x12,
    IBIS_IB_ATTR_PERF_MANAGEMENT_PORT_XMIT_DISCARD_DETAILS = 0x16,
    IBIS_IB_ATTR_PERF_MANAGEMENT_PORT_COUNTERS_EXT = 0x1D,
    IBIS_IB_ATTR_PERF_MANAGEMENT_PORT_XMIT_DATA_SL = 0x36,
    IBIS_IB_ATTR_PERF_MANAGEMENT_PORT_RCV_DATA_SL = 0x37,
    IBIS_IB_ATTR_PERF_MANAGEMENT_LAST
};


enum VENDOR_MELLANOX_ATTR_ID {
    IBIS_IB_ATTR_VENDOR_SPEC_MELLANOX_GENERAL_INFO = 0x17,
    IBIS_IB_ATTR_VENDOR_SPEC_MELLANOX_LAST
};

enum CONGESTION_CONTROL_ATTR_ID {
    IBIS_IB_ATTR_CONGESTION_CONT_CONG_INFO = 0x11,
    IBIS_IB_ATTR_CONGESTION_CONT_CONG_KEY_INFO = 0x12,
	IBIS_IB_ATTR_CONGESTION_CONT_CONG_LOG = 0x13,
	IBIS_IB_ATTR_CONGESTION_CONT_SW_CONG_SETTING = 0x14,
	IBIS_IB_ATTR_CONGESTION_CONT_SW_PORT_CONG_SETTING = 0x15,
	IBIS_IB_ATTR_CONGESTION_CONT_CA_CONG_SETTING = 0x16,
	IBIS_IB_ATTR_CONGESTION_CONT_CC_TBL = 0x17,
	IBIS_IB_ATTR_CONGESTION_CONT_TIME_STAMP = 0x18,
	IBIS_IB_ATTR_CONGESTION_CONT_LAST
};

enum IB_ATTR_ID {
	IBIS_IB_ATTR_CLASS_PORT_INFO = 0x1,
	IBIS_IB_ATTR_NOTICE = 0x2,
	IBIS_IB_ATTR_LAST
};


/****************************************************/
/* log Macros */
#ifdef DEBUG
    #define IBIS_LOG(level, fmt, ...) TT_LOG(TT_LOG_MODULE_IBIS, level, fmt, ## __VA_ARGS__);
    #define IBIS_ENTER TT_ENTER( TT_LOG_MODULE_IBIS );
    #define IBIS_RETURN(rc) { TT_EXIT( TT_LOG_MODULE_IBIS );    \
                                return (rc); }
    #define IBIS_RETURN_VOID { TT_EXIT( TT_LOG_MODULE_IBIS );   \
                                return; }
#else           /*def DEBUG */
    #define IBIS_LOG(level, fmt, ...)
    #define IBIS_ENTER
    #define IBIS_RETURN(rc) return (rc);
    #define IBIS_RETURN_VOID return;
#endif          /*def DEBUG */


#endif          /* not defined IBIS_TYPES_H_ */
