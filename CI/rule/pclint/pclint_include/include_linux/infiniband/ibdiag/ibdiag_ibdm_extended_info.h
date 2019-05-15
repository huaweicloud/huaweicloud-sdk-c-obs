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


#ifndef IBDIAG_IBDM_EXTENDED_INFO_H
#define IBDIAG_IBDM_EXTENDED_INFO_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string>
#include <list>
#include <vector>
using namespace std;

#include <infiniband/ibdm/Fabric.h>
#include <infiniband/ibis/packets/packets_layouts.h>

#include "ibdiag_types.h"


/*****************************************************/
typedef enum special_speed {
    NONE = 0,
    EXTENDED = 1,
    MLNX_EXTENDED = 2
} special_speed_t;


static inline const char * supspeed2char(const u_int8_t s, special_speed_t speed_type = NONE)
{
    if (speed_type == NONE) {
        switch (s) {
        case 1:     return("2.5");              // SDR
        case 2:     return("5");                // DDR
        case 3:     return("2.5 or 5");         // SDR or DDR
        case 4:     return("10");               // QDR
        case 5:     return("2.5 or 10");        // SDR or QDR
        case 6:     return("5 or 10");          // DDR or QDR
        case 7:     return("2.5 or 5 or 10");   // SDR or DDR or QDR
        default:    return("UNKNOWN");
        }
    } else if (speed_type == EXTENDED) {
        switch (s) {
        case 1:     return("14");               // FDR
        case 2:     return("25");               // EDR
        case 3:     return("14 or 25");         // FDR or EDR
        /*
         * case 4:  HDR
         * case 5:  FDR or HDR
         * case 6:  EDR or HDR
         * case 7:  EDR or FDR or HDR
         * case 8:  NDR
         * case 9:  FDR or NDR
         * case 10: EDR or NDR
         * case 11: FDR or EDR or NDR
         * case 12: HDR or NDR
         * case 13: FDR or HDR or NDR
         * case 14: EDR or HDR or NDR
         * case 15: FDR or EDR or HDR or NDR
         */
        default:    return("UNKNOWN");
        }
    } else {    //speed_type == MLNX_EXTENDED
        switch (s) {
        case 1:     return("FDR10");            // FDR10
        /*
         * 2-255: Reserved
         */
        default:    return("UNKNOWN");
        }
    }
};

static inline const char * supwidth2char(const u_int8_t w)
{
    switch (w) {
    case 1:     return("1x");
    case 2:     return("4x");
    case 3:     return("1x or 4x");
    case 4:     return("8x");
    case 5:     return("1x or 8x");
    case 6:     return("4x or 8x");
    case 7:     return("1x or 4x or 8x");
    case 8:     return("12x");
    case 9:     return("1x or 12x");
    case 10:    return("4x or 12x");
    case 11:    return("1x or 4x or 12x");
    case 12:    return("8x or 12x");
    case 13:    return("1x or 8x or 12x");
    case 14:    return("4x or 8x or 12x");
    case 15:    return("1x or 4x or 8x or 12x");
    default:    return("UNKNOWN");
    }
};


IBLinkSpeed CalcFinalSpeed(u_int8_t speed1, u_int8_t speed2,
        special_speed_t speed_type);
IBLinkWidth CalcFinalWidth(u_int8_t width1, u_int8_t width2);
u_int64_t CalcLinkRate(IBLinkWidth link_width, IBLinkSpeed link_speed);


static inline bool IsValidEnableSpeed(u_int8_t link_speed_en, u_int8_t link_speed_sup)
{
    return (((u_int8_t)link_speed_en | (u_int8_t)link_speed_sup) == (u_int8_t)link_speed_sup);
}
static inline bool IsValidEnableWidth(u_int8_t link_width_en, u_int8_t link_width_sup)
{
    return (((u_int8_t)link_width_en | (u_int8_t)link_width_sup) == (u_int8_t)link_width_sup);
}


/*****************************************************/
class IBDMExtendedInfo {
private:
    //members
    string last_error;

    vector_p_node   nodes_vector;
    vector_p_port   ports_vector;

    list_p_sm_info_obj                  sm_info_obj_list;

    vector_p_smp_node_info              smp_node_info_vector;           // by node index
    vector_p_smp_switch_info            smp_switch_info_vector;         // by node index
    vector_p_smp_port_info              smp_port_info_vector;           // by port index
    vector_p_smp_mlnx_ext_port_info     smp_mlnx_ext_port_info_vector;  // by port index
    vector_p_vs_general_info            vs_general_info_vector;         // by node index
    vector_p_pm_cap_mask                pm_cap_mask_vector;             // by node index
    vector_p_pm_info_obj                pm_info_obj_vector;             // by port index

    vector_v_smp_pkey_tbl				smp_pkey_tbl_v_vector;			// by port index, block index
	vector_v_smp_guid_tbl				smp_guid_tbl_v_vector;			// by port index, block index

    //methods
    void SetLastError(const char *fmt, ...);

    template <class OBJ_VEC_TYPE, class OBJ_TYPE>
    void addPtrToVec(OBJ_VEC_TYPE& vector_obj, OBJ_TYPE *p_obj);

    template <class OBJ_VEC_TYPE, class OBJ_TYPE>
    OBJ_TYPE * getPtrFromVec(OBJ_VEC_TYPE& vector_obj, u_int32_t create_index);

    template <class OBJ_VEC_TYPE, class OBJ_TYPE, class DATA_VEC_TYPE, class DATA_TYPE>
    int addDataToVec(OBJ_VEC_TYPE& vector_obj,
            OBJ_TYPE *p_obj,
            DATA_VEC_TYPE& vector_data,
            DATA_TYPE& data);

    template <class OBJ_VEC_TYPE, class OBJ_TYPE, class DATA_VEC_TYPE, class DATA_TYPE>
    int addDataToVecInVec(OBJ_VEC_TYPE& vector_obj,
            OBJ_TYPE *p_obj,
            DATA_VEC_TYPE& vec_of_vectors,
            u_int32_t data_idx,
            DATA_TYPE& data);

    template <class OBJ_VEC_TYPE, class OBJ_TYPE>
    OBJ_TYPE * getPtrFromVecInVec(OBJ_VEC_TYPE& vec_of_vectors,
            u_int32_t idx1,
            u_int32_t idx2);

public:
    //methods
    void CleanUpInternalDB();

    IBDMExtendedInfo();
    ~IBDMExtendedInfo();

    const char* GetLastError();

    inline u_int32_t getNodesVectorSize() { return this->nodes_vector.size(); }
    inline u_int32_t getPortsVectorSize() { return this->ports_vector.size(); }

    IBNode * getNodePtr(u_int32_t node_index);
    IBPort * getPortPtr(u_int32_t port_index);

    //Returns: SUCCESS_CODE / ERR_CODE_INCORRECT_ARGS / ERR_CODE_NO_MEM
    int addSMPSMInfoObj(IBPort *p_port, struct SMP_SMInfo &smpSMInfo);
    int addSMPNodeInfo(IBNode *p_node, struct SMP_NodeInfo &smpNodeInfo);
    int addSMPSwitchInfo(IBNode *p_node, struct SMP_SwitchInfo &smpSwitchInfo);
    int addSMPPortInfo(IBPort *p_port, struct SMP_PortInfo &smpPortInfo);
    int addSMPMlnxExtPortInfo(IBPort *p_port, struct SMP_MlnxExtPortInfo &smpMlnxExtPortInfo);
    int addVSGeneralInfo(IBNode *p_node, struct VendorSpec_GeneralInfo &vsGeneralInfo);
    int addPMCapMask(IBNode *p_node, u_int16_t pm_cap_mask);
    int addPMPortCounters(IBPort *p_port, struct PM_PortCounters &pmPortCounters);
    int addPMPortCountersExtended(IBPort *p_port, struct PM_PortCountersExtended &pmPortCountersExtended);
    int addSMPPKeyTable(IBPort *p_port, struct SMP_PKeyTable &smpPKeyTable, u_int32_t block_idx);
    int addSMPGUIDInfo(IBPort *p_port, struct SMP_GUIDInfo &smpGUIDInfo, u_int32_t block_idx);

    SMP_NodeInfo*               getSMPNodeInfo(u_int32_t node_index);
    SMP_SwitchInfo*             getSMPSwitchInfo(u_int32_t node_index);
    SMP_PortInfo*               getSMPPortInfo(u_int32_t port_index);
    SMP_MlnxExtPortInfo*        getSMPMlnxExtPortInfo(u_int32_t port_index);
    VendorSpec_GeneralInfo*     getVSGeneralInfo(u_int32_t node_index);
    u_int16_t*                  getPMCapMask(u_int32_t node_index);
    PM_PortCounters*            getPMPortCounters(u_int32_t port_index);
    PM_PortCountersExtended*    getPMPortCountersExtended(u_int32_t port_index);
    SMP_PKeyTable *				getSMPPKeyTable(u_int32_t port_index, u_int32_t block_idx);
    SMP_GUIDInfo * 				getSMPGUIDInfo(u_int32_t port_index, u_int32_t block_idx);

    inline list_p_sm_info_obj& getSMPSMInfoListRef() { return this->sm_info_obj_list; }
    inline vector_p_smp_node_info& getSMPNodeInfoVectorRef() { return this->smp_node_info_vector; }
    inline vector_p_smp_switch_info& getSMPSwitchInfoVectorRef() { return this->smp_switch_info_vector; }
    inline vector_p_smp_port_info& getSMPPortInfoVectorRef() { return this->smp_port_info_vector; }
    inline vector_p_vs_general_info& getVSGeneralInfoVectorRef() { return this->vs_general_info_vector; }
    inline vector_p_pm_cap_mask& getPMCapMaskVectorRef() { return this->pm_cap_mask_vector; }
    inline vector_p_pm_info_obj& getPMInfoObjVectorRef() { return this->pm_info_obj_vector; }
};


#endif          /* IBDIAG_IBDM_EXTENDED_INFO_H */

