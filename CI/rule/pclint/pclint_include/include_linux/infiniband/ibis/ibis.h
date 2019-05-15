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


#ifndef IBIS_H_
#define IBIS_H_


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string>
using namespace std;

#include "ibis_types.h"


/****************************************************/
const char * get_ibis_version();


/****************************************************/
class Ibis {
private:
    ////////////////////
    //members
    ////////////////////
    string dev_name;
    int port_num;

    enum {NOT_INITILIAZED, NOT_SET_PORT, READY} ibis_status;
    string last_error;

    void *p_umad_buffer_send;       /* buffer for send mad - sync buffer */
    void *p_umad_buffer_recv;       /* buffer for recv mad - sync buffer */
    u_int8_t *p_pkt_send;           /* mad pkt to send */
    u_int8_t *p_pkt_recv;           /* mad pkt was received */
    u_int64_t mads_counter;         /* number of mads we sent already */

    int umad_port_id;									/* file descriptor returned by umad_open() */
    int umad_agents_by_class[IBIS_IB_MAX_MAD_CLASSES];  /* array to map class --> agent */
    int timeout;
    int retries;

    ////////////////////
    //methods
    ////////////////////
    void SetLastError(const char *fmt, ...);
    bool IsLegalMgmtClass(int mgmt_class);

    // Returns: 0 - success / 1 - error
    int Bind();
    int RecvMad(int mgmt_class, int umad_timeout);
    int SendMad(int mgmt_class, int umad_timeout, int umad_retries);

    /*
     * Returns: mad status[bits: 0-15] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int DoRPC(int mgmt_class);

    ////////////////////
    //methods mads
    ////////////////////
    void DumpMadData(dump_data_func_t dump_func, void *mad_obj);
    void CommonMadHeaderBuild(struct MAD_Header_Common *mad_header,
            int mgmt_class,
            u_int8_t method,
            u_int16_t attribute_id,
            u_int32_t attribute_modifier);
    int GetMgmtClassVersion(int mgmt_class);

    ////////////////////
    //smp class methods
    ////////////////////
    void SMPHeaderDirectRoutedBuild(struct MAD_Header_SMP_Direct_Routed *smp_direct_route_header,
            u_int8_t method,
            u_int16_t attribute_id,
            u_int32_t attribute_modifier,
            u_int8_t direct_path_len);

    ////////////////////
    //mellanox methods
    ////////////////////
    bool IsSupportIB(void *type);
    bool IsIBDevice(void *arr, unsigned arr_size, u_int16_t dev_id);

public:
    ////////////////////
    //methods
    ////////////////////
    Ibis();
    ~Ibis();

    inline bool IsInit() { return (this->ibis_status != NOT_INITILIAZED); };
    inline bool IsReady() { return (this->ibis_status == READY); };
    inline void SetTimeout(int timeout_value) { this->timeout = timeout_value; };
    inline void SetNumOfRetries(int retries_value) { this->retries = retries_value; };
    inline u_int64_t GetNewTID() { return ++this->mads_counter;	}
    inline u_int8_t *GetSendMadPktPtr() { return this->p_pkt_send; };
    inline u_int8_t *GetRecvMadPktPtr() { return this->p_pkt_recv; };

    const char* GetLastError();
    string ConvertDirPathToStr(direct_route_t *p_curr_direct_route);
    string ConvertMadStatusToStr(u_int16_t status);

    // Returns: 0 - success / 1 - error
    int Init();
    int SetPort(const char* device_name, int port_num);
    int SetPort(u_int64_t port_guid);   //guid is BE
    int SetSendMadAddr(int d_lid, int d_qp, int sl, int qkey);
    int GetAllLocalPortGUIDs(OUT local_port_t local_ports_array[IBIS_MAX_LOCAL_PORTS],
            OUT u_int32_t *p_local_ports_num);

    ////////////////////
    //methods mads
    ////////////////////
    /*
     * Returns: mad status[bits: 0-15] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int MadGetSet(u_int16_t lid,
            u_int32_t d_qp,
            u_int8_t sl,
            u_int32_t qkey,
            u_int8_t mgmt_class,
            u_int8_t method,
            u_int16_t attribute_id,
            u_int32_t attribute_modifier,
            u_int8_t data_offset,
            void *p_class_data,
            void *p_attribute_data,
            const pack_data_func_t pack_class_data_func,
            const unpack_data_func_t unpack_class_data_func,
            const dump_data_func_t dump_class_data_func,
            const pack_data_func_t pack_attribute_data_func,
            const unpack_data_func_t unpack_attribute_data_func,
            const dump_data_func_t dump_attribute_data_func);

    ////////////////////
    //smp class methods
    ////////////////////
    /*
     * Returns: mad status[bits: 0-15] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int SMPMadGetSetByLid(u_int16_t lid,
            u_int8_t method,
            u_int16_t attribute_id,
            u_int32_t attribute_modifier,
            void *p_smp_attribute_data,
            const pack_data_func_t smp_pack_attribute_data_func,
            const unpack_data_func_t smp_unpack_attribute_data_func,
            const dump_data_func_t smp_dump_attribute_data_func);
    int SMPMadGetSetByDirect(direct_route_t *p_direct_route,
            u_int8_t method,
            u_int16_t attribute_id,
            u_int32_t attribute_modifier,
            void *p_smp_attribute_data,
            const pack_data_func_t smp_pack_attribute_data_func,
            const unpack_data_func_t smp_unpack_attribute_data_func,
            const dump_data_func_t smp_dump_attribute_data_func);

    /*
     * Returns: mad status[bits: 0-7] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int SMPPortInfoMadGetByLid(u_int16_t lid,
            u_int32_t port_number,
            struct SMP_PortInfo *p_port_info);
    int SMPMlnxExtPortInfoMadGetByLid(u_int16_t lid,
            u_int32_t port_number,
            struct SMP_MlnxExtPortInfo *p_mlnx_ext_port_info);
    int SMPSwitchInfoMadGetByLid(u_int16_t lid,
            struct SMP_SwitchInfo *p_switch_info);
    int SMPNodeInfoMadGetByLid(u_int16_t lid,
            struct SMP_NodeInfo *p_node_info);
    int SMPNodeDescMadGetByLid(u_int16_t lid,
            struct SMP_NodeDesc *p_node_desc);
    int SMPSMInfoMadGetByLid(u_int16_t lid,
            struct SMP_SMInfo *p_sm_info);
    int SMPLinearForwardingTableGetByLid(u_int16_t lid,
            u_int32_t lid_to_port_block_num,
            struct SMP_LinearForwardingTable *p_linear_forwarding_table);
    int SMPMulticastForwardingTableGetByLid(u_int16_t lid,
            u_int8_t port_group,
            u_int32_t lid_to_port_block_num,
            struct SMP_MulticastForwardingTable *p_multicast_forwarding_table);
    int SMPPkeyTableGetByLid(u_int16_t lid,
            u_int16_t port_num,
            u_int16_t block_num,
            struct SMP_PKeyTable *p_pkey_table);
    int SMPGUIDInfoTableGetByLid(u_int16_t lid,
            u_int32_t block_num,
            struct SMP_GUIDInfo *p_guid_info);

    int SMPPortInfoMadGetByDirect(direct_route_t *p_direct_route,
            u_int32_t port_number,
            struct SMP_PortInfo *p_port_info);
    int SMPMlnxExtPortInfoMadGetByDirect(direct_route_t *p_direct_route,
            u_int32_t port_number,
            struct SMP_MlnxExtPortInfo *p_mlnx_ext_port_info);
    int SMPSwitchInfoMadGetByDirect(direct_route_t *p_direct_route,
            struct SMP_SwitchInfo *p_switch_info);
    int SMPNodeInfoMadGetByDirect(direct_route_t *p_direct_route,
            struct SMP_NodeInfo *p_node_info);
    int SMPNodeDescMadGetByDirect(direct_route_t *p_direct_route,
            struct SMP_NodeDesc *p_node_desc);
    int SMPSMInfoMadGetByDirect(direct_route_t *p_direct_route,
            struct SMP_SMInfo *p_sm_info);
    int SMPLinearForwardingTableGetByDirect(direct_route_t *p_direct_route,
            u_int32_t lid_to_port_block_num,
            struct SMP_LinearForwardingTable *p_linear_forwarding_table);
    int SMPMulticastForwardingTableGetByDirect(direct_route_t *p_direct_route,
            u_int8_t port_group,
            u_int32_t lid_to_port_block_num,
            struct SMP_MulticastForwardingTable *p_multicast_forwarding_table);
    int SMPPKeyTableGetByDirect(direct_route_t *p_direct_route,
            u_int16_t port_num,
            u_int16_t block_num,
            struct SMP_PKeyTable *p_pkey_table);
    int SMPGUIDInfoTableGetByDirect(direct_route_t *p_direct_route,
            u_int32_t block_num,
            struct SMP_GUIDInfo *p_guid_info);

    ////////////////////
    //pm class methods
    ////////////////////
    /*
     * Returns: mad status[bits: 0-15] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int PMMadGetSet(u_int16_t lid,
            u_int8_t method,
            u_int16_t attribute_id,
            u_int32_t attribute_modifier,
            void *p_pm_attribute_data,
            const pack_data_func_t pm_pack_attribute_data_func,
            const unpack_data_func_t pm_unpack_attribute_data_func,
            const dump_data_func_t pm_dump_attribute_data_func);

    /*
     * Returns: mad status[bits: 0-7] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int PMClassPortInfoGet(u_int16_t lid,
            struct IB_ClassPortInfo *p_ib_class_port_info);
    int PMPortCountersGet(u_int16_t lid,
            u_int32_t port_number,
            struct PM_PortCounters *p_port_counters);
    int PMPortCountersSet(u_int16_t lid,
            struct PM_PortCounters *p_port_counters);
    int PMPortCountersClear(u_int16_t lid,
            u_int32_t port_number);
    int PMPortCountersExtendedGet(u_int16_t lid,
            u_int32_t port_number,
            struct PM_PortCountersExtended *p_port_counters);
    int PMPortCountersExtendedSet(u_int16_t lid,
            struct PM_PortCountersExtended *p_port_counters);
    int PMPortCountersExtendedClear(u_int16_t lid,
            u_int32_t port_number);

    ////////////////////
    //vs class methods
    ////////////////////
    /*
     * Returns: mad status[bits: 0-15] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int VSMadGetSet(u_int16_t lid,
            u_int8_t method,
            u_int16_t attribute_id,
            u_int32_t attribute_modifier,
            void *p_vs_attribute_data,
            const pack_data_func_t vs_pack_attribute_data_func,
            const unpack_data_func_t vs_unpack_attribute_data_func,
            const dump_data_func_t vs_dump_attribute_data_func);

    /*
     * Returns: mad status[bits: 0-7] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int VSGeneralInfoGet(u_int16_t lid, struct VendorSpec_GeneralInfo *p_general_info);

    ////////////////////
    //cc class methods
    ////////////////////
    /*
     * Returns: mad status[bits: 0-15] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int CCMadGetSet(u_int16_t lid,
            u_int8_t sl,
            u_int8_t method,
            u_int16_t attribute_id,
            u_int32_t attribute_modifier,
            u_int64_t cc_key,
            void *p_cc_log_attribute_data,
            void *p_cc_mgt_attribute_data,
            const pack_data_func_t cc_pack_attribute_data_func,
            const unpack_data_func_t cc_unpack_attribute_data_func,
            const dump_data_func_t cc_dump_attribute_data_func);

    /*
     * Returns: mad status[bits: 0-7] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int CCClassPortInfoGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct IB_ClassPortInfo *p_ib_class_port_info);
    int CCClassPortInfoSet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct IB_ClassPortInfo *p_ib_class_port_info);
    int CCCongestionInfoGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_CongestionInfo *p_cc_congestion_info);
    int CCCongestionKeyInfoGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_CongestionKeyInfo *p_cc_congestion_key_info);
    int CCCongestionKeyInfoSet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_CongestionKeyInfo *p_cc_congestion_key_info);
    int CCCongestionLogSwitchGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_CongestionLogSwitch *p_cc_congestion_log_sw);
    int CCCongestionLogCAGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_CongestionLogCA *p_cc_congestion_log_ca);
    int CCSwitchCongestionSettingGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_SwitchCongestionSetting *p_cc_sw_congestion_setting);
    int CCSwitchCongestionSettingSet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_SwitchCongestionSetting *p_cc_sw_congestion_setting);
    int CCSwitchPortCongestionSettingGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            u_int8_t block_idx,
            struct CC_SwitchPortCongestionSetting *p_cc_sw_port_congestion_setting);
    int CCSwitchPortCongestionSettingSet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            u_int8_t block_idx,
            struct CC_SwitchPortCongestionSetting *p_cc_sw_port_congestion_setting);
    int CCCACongestionSettingGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_CACongestionSetting *p_cc_ca_congestion_setting);
    int CCCACongestionSettingSet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_CACongestionSetting *p_cc_ca_congestion_setting);
    int CCCongestionControlTableGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            u_int8_t block_idx,
            struct CC_CongestionControlTable *p_cc_congestion_control_table);
    int CCCongestionControlTableSet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            u_int8_t block_idx,
            struct CC_CongestionControlTable *p_cc_congestion_control_table);
    int CCTimeStampGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_TimeStamp *p_cc_time_stamp);

    ////////////////////
    //mellanox methods
    ////////////////////
    bool IsVenMellanox(u_int32_t vendor_id);
    //switch
    bool IsDevAnafa(u_int16_t dev_id);
    bool IsDevShaldag(u_int16_t dev_id);
    bool IsDevSwitchXIB(u_int16_t dev_id);
    //bridge
    bool IsDevBridgeXIB(u_int16_t dev_id);
    //hca
    bool IsDevTavor(u_int16_t dev_id);
    bool IsDevSinai(u_int16_t dev_id);
    bool IsDevArbel(u_int16_t dev_id);
    bool IsDevConnectX_1IB(u_int16_t dev_id);
    bool IsDevConnectX_2IB(u_int16_t dev_id);
    bool IsDevConnectX_3IB(u_int16_t dev_id);
    bool IsDevConnectXIB(u_int16_t dev_id);
};

#endif	/* IBIS_H_ */

