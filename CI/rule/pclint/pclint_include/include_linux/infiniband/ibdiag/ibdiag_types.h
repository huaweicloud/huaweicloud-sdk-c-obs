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


#ifndef IBDIAG_TYPES_H_
#define IBDIAG_TYPES_H_

#include <stdlib.h>
#include <vector>
#include <list>
#include <map>

#include <infiniband/ibdm/Fabric.h>
#include <infiniband/ibis/ibis_types.h>
#include <infiniband/ibis/packets/packets_layouts.h>
#include <infiniband/misc/tool_trace/tool_trace.h>


/****************************************************/
/* new types */
typedef struct progress_bar_nodes {
    u_int32_t nodes_found;
    u_int32_t sw_found;
    u_int32_t ca_found;
} progress_bar_nodes_t;
typedef void (*progress_func_discovered_t)(progress_bar_nodes_t *p_progress_discovered);
typedef void (*progress_func_nodes_t)(progress_bar_nodes_t *p_progress, progress_bar_nodes_t *p_target_result);

typedef struct progress_bar_ports {
    u_int64_t ports_found;
} progress_bar_ports_t;
typedef void (*progress_func_ports_t)(progress_bar_ports_t *p_progress, progress_bar_ports_t *p_target_result);


typedef struct sm_info_obj {
    struct SMP_SMInfo smp_sm_info;
    IBPort *p_port;
} sm_info_obj_t;

typedef struct pm_info_obj {
    struct PM_PortCounters *p_port_counters;
    struct PM_PortCountersExtended *p_extended_port_counters;
} pm_info_obj_t;

typedef struct fw_version_obj {
    u_int32_t major;
    u_int32_t minor;
    u_int32_t sub_minor;

    fw_version_obj& operator=(const struct VendorSpec_GeneralInfo& other) {
        major = other.FWInfo.Extended_Major;
        minor = other.FWInfo.Extended_Minor;
        sub_minor = other.FWInfo.Extended_SubMinor;
        return *this;
    }
    bool operator==(const struct fw_version_obj& other) {
        return (major == other.major && minor == other.minor && sub_minor == other.sub_minor);
    }
    bool operator!=(const struct fw_version_obj& other) {
        return (!(*this == other));
    }
    bool operator>(const struct fw_version_obj& other) {
        u_int32_t x[3] = {major, minor, sub_minor};
        const u_int32_t y[3] = {other.major, other.minor, other.sub_minor};
        for (int i = 0; i < 3; i++) {
            if (x[i] > y[i])
                return true;
            if (x[i] < y[i])
                return false;
        }
        return false;
    }
    bool operator<(const struct fw_version_obj& other) {
        return (!(*this > other));
    }
} fw_version_obj_t;


typedef vector < IBNode * >                                     vector_p_node;
typedef vector < IBPort * >                                     vector_p_port;
typedef vector < struct SMP_NodeInfo * >                        vector_p_smp_node_info;
typedef vector < struct SMP_SwitchInfo * >                      vector_p_smp_switch_info;
typedef vector < struct SMP_PortInfo * >                        vector_p_smp_port_info;
typedef vector < struct SMP_MlnxExtPortInfo * >                 vector_p_smp_mlnx_ext_port_info;
typedef vector < struct VendorSpec_GeneralInfo * >              vector_p_vs_general_info;
typedef vector < u_int16_t * >                                  vector_p_pm_cap_mask;
typedef vector < pm_info_obj_t * >                              vector_p_pm_info_obj;
typedef list < IBNode * >                                       list_p_node;
typedef list < IBPort * >                                       list_p_port;
typedef list < sm_info_obj_t * >                                list_p_sm_info_obj;
typedef list < direct_route_t  * >                              list_p_direct_route;
typedef list < string >                                         list_string;
typedef list < u_int8_t >                                       list_uint8;
typedef map < u_int64_t, list_p_direct_route >                  map_guid_list_p_direct_route;
typedef map < u_int16_t, list_p_port >                          map_lid_list_p_port;
typedef map < u_int32_t,  struct VendorSpec_GeneralInfo * >     map_devid_p_vs_general_info;
typedef map < string,  u_int64_t >                              map_str_uint64;
typedef map < u_int8_t,  u_int64_t >                            map_uint8_uint64;
typedef vector < struct SMP_PKeyTable * >                       vector_p_smp_pkey_tbl;
typedef vector < vector_p_smp_pkey_tbl >                        vector_v_smp_pkey_tbl;
typedef vector < struct SMP_GUIDInfo * >                        vector_p_smp_guid_tbl;
typedef vector < vector_p_smp_guid_tbl >                        vector_v_smp_guid_tbl;


/****************************************************/
/* sections defines */
#define SECTION_NODES       "NODES"
#define SECTION_PORTS       "PORTS"
#define SECTION_LINKS       "LINKS"
#define SECTION_PM_INFO     "PM_INFO"
#define SECTION_SM_INFO     "SM_INFO"
#define SECTION_NODES_INFO  "NODES_INFO"
#define SECTION_AGUID		"AGUID"
#define SECTION_PKEY		"PKEY"


/****************************************************/
/* defines */
#define IBDIAG_ERR_SIZE                             4096            /* 4096 bytes */
#define IBDIAG_MAX_HOPS                             64              /* maximum hops for discovery BFS */
#define IBDIAG_MAX_SUPPORTED_NODE_PORTS             64              /* for lft and mft retrieve */
#define IBDIAG_BER_THRESHOLD_OPPOSITE_VAL           0xe8d4a51000    /* 10^12 */

enum OVERFLOW_VALUES {
    OVERFLOW_VAL_NONE   = 0x0,
    OVERFLOW_VAL_4_BIT  = 0xf,
    OVERFLOW_VAL_8_BIT  = 0xff,
    OVERFLOW_VAL_16_BIT = 0xffff,
    OVERFLOW_VAL_32_BIT = 0xffffffff,
    OVERFLOW_VAL_64_BIT = 0xffffffffffffffffULL
};

enum IBDIAG_RETURN_CODES {
    IBDIAG_SUCCESS_CODE = 0x0,
    IBDIAG_ERR_CODE_FABRIC_ERROR = 0x1,
    IBDIAG_ERR_CODE_NO_MEM = 0x3,
    IBDIAG_ERR_CODE_DB_ERR = 0x4,
    IBDIAG_ERR_CODE_IBDM_ERR = 0x5,
    IBDIAG_ERR_CODE_INIT_FAILED = 0x6,
    IBDIAG_ERR_CODE_NOT_READY = 0x7,
    IBDIAG_ERR_CODE_IO_ERR = 0x8,
    IBDIAG_ERR_CODE_CHECK_FAILED = 0x9,
    IBDIAG_ERR_CODE_EXCEEDS_MAX_HOPS = 0x10,
    IBDIAG_ERR_CODE_DUPLICATED_GUID = 0x11,
    IBDIAG_ERR_CODE_INCORRECT_ARGS = 0x12,
    IBDIAG_ERR_CODE_DISCOVERY_NOT_SUCCESS = 0x13,
    IBDIAG_ERR_CODE_TRY_TO_DISCONNECT_CONNECTED_PORT = 0x14,
    IBDIAG_ERR_CODE_LAST
};


/****************************************************/
/* general defines */
#ifndef CLEAR_STRUCT
    #define CLEAR_STRUCT(n) memset(&(n), 0, sizeof(n))
#endif
#define OFFSET_OF(type, field) ((unsigned long) &(((type *) 0)->field))

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
/* log Macros */
#ifdef DEBUG
    #define IBDIAG_LOG(level, fmt, ...) TT_LOG(TT_LOG_MODULE_IBDIAG, level, fmt, ## __VA_ARGS__);
    #define IBDIAG_ENTER TT_ENTER( TT_LOG_MODULE_IBDIAG );
    #define IBDIAG_RETURN(rc) { TT_EXIT( TT_LOG_MODULE_IBDIAG );  \
                                return (rc); }
    #define IBDIAG_RETURN_VOID { TT_EXIT( TT_LOG_MODULE_IBDIAG );   \
                                    return; }
#else           /*def DEBUG */
    #define IBDIAG_LOG(level, fmt, ...)
    #define IBDIAG_ENTER
    #define IBDIAG_RETURN(rc) return (rc);
    #define IBDIAG_RETURN_VOID return;
#endif          /*def DEBUG */

#ifndef WIN32
    #define IBDIAG_LOG_NAME "/tmp/ibutils2_ibdiag.log"
#else
    #define IBDIAG_LOG_NAME "\\tmp\\ibutils2_ibdiag.log"
#endif      /* ndef WIN32 */


#endif          /* not defined IBDIAG_TYPES_H_ */
