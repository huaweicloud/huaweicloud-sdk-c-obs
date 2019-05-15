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


#ifndef IBDIAG_FABRIC_ERRS_H
#define IBDIAG_FABRIC_ERRS_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string>
#include <list>
#include <iostream>
using namespace std;

#include <infiniband/ibdm/Fabric.h>

#include "ibdiag_types.h"


/*****************************************************/
/*
 * NOTE: All the following defines must not contain ','
 */
#define SCOPE_CLUSTER   "CLUSTER"
#define SCOPE_SYSTEM    "SYSTEM"
#define SCOPE_NODE      "NODE"
#define SCOPE_PORT      "PORT"

/* event names */
#define FER_DUPLICATED_NODE_GUID    "DUPLICATED_NODE_GUID"
#define FER_DUPLICATED_PORT_GUID    "DUPLICATED_PORT_GUID"

#define FER_NODE_NOT_RESPOND            "NODE_NO_RESPONSE"
#define FER_NODE_WRONG_CONFIG           "NODE_CONFIG_WRONG"
#define FER_NODE_DUPLICATED_NODE_DESC   "NODE_DUPLICATED_NODE_DESC"
#define FER_NODE_WRONG_FW_VER           "NODE_WRONG_FW_VERSION"
#define FER_NODE_NOT_SUPPORT_CAP        "NODE_NOT_SUPPORT_CAPABILTY"

#define FER_SM_NOT_FOUND        "SM_NOT_FOUND_MASTER"
#define FER_SM_UNKNOWN_STATE    "SM_UNKNOWN_STATE"
#define FER_SM_MANY_EXISTS      "SM_FOUND_MANY_MASTERS"
#define FER_SM_NOT_CORRECT      "SM_NOT_CORRECT_MASTER"

#define FER_PM_COUNTER_OVERFLOW         "PM_COUNTER_OVERFLOW"
#define FER_PM_COUNTER_EXCEED_THRESH    "PM_COUNTER_EXCEEDS_THRESHOLD"
#define FER_PM_COUNTER_NOT_SUPPORTED    "PM_COUNTER_NOT_SUPPORTED"
#define FER_PM_COUNTER_INCREASED        "PM_COUNTER_INCREASED"

#define FER_BER_EXCEED_THRESH   "BER_EXCEEDS_THRESHOLD"
#define FER_BER_NO_RCV_DATA     "BER_NO_RCV_DATA"
#define FER_BER_IS_ZERO         "BER_VALUE_ZERO"

#define FER_PORT_ZERO_LID           "PORT_LID_ZERO"
#define FER_PORT_DUPLICATED_LID     "PORT_DUPLICATED_LID"
#define FER_PORT_NOT_RESPOND        "PORT_NO_RESPONSE"
#define FER_PORT_WRONG_CONFIG       "PORT_CONFIG_WRONG"
#define FER_PORT_NOT_SUPPORT_CAP    "PORT_NOT_SUPPORT_CAPABILTY"

#define FER_DIRECT_ROUTE    "BAD_LINK"

#define FER_LINK_LOGICAL_STATE_WRONG        "LINK_WRONG_LOGICAL_STATE"
#define FER_LINK_LOGICAL_STATE_NOT_ACTIVE   "LINK_LOGICAL_STATE_NOT_ACTIVE"
#define FER_LINK_LOGICAL_DIFFERENT_SPEED    "LINK_DIFFERENT_SPEED"
#define FER_LINK_LOGICAL_UNEXPECTED_SPEED   "LINK_UNEXPECTED_SPEED"
#define FER_LINK_LOGICAL_DIFFERENT_WIDTH    "LINK_DIFFERENT_WIDTH"
#define FER_LINK_LOGICAL_UNEXPECTED_WIDTH   "LINK_UNEXPECTED_WIDTH"
#define FER_LINK_LOGICAL_AUTONEG_ERR        "LINK_AUTONEG_ERR"

#define FER_PKEY_MISMATCH   "PKEY_MISMATCH"

/*****************************************************/
typedef list < class FabricErrGeneral * >           list_p_fabric_general_err;


/*****************************************************/
void DumpCSVFabricErrorListTable(list_p_fabric_general_err& list_errors, ofstream &sout, string name);
void CleanFabricErrorsList(list_p_fabric_general_err& list_errors);
string DescToCsvDesc(string desc);


/*****************************************************/
/*****************************************************/
/*****************************************************/
class FabricErrGeneral {
protected:
    string scope;
    string description;
    string err_desc;
public:
    FabricErrGeneral(): scope("UNKNOWN"), description("UNKNOWN"), err_desc("UNKNOWN") {}
    virtual ~FabricErrGeneral() {}

    virtual string GetCSVErrorLine() = 0;
    virtual string GetErrorLine() = 0;
};


/*****************************************************/
/*****************************************************/
/*****************************************************/
class FabricErrGuid : public FabricErrGeneral {
protected:
    IBNode *p_node;
    string direct_route_to_node_str;
    u_int64_t duplicated_guid;
public:
    FabricErrGuid(IBNode *p_node, string direct_route_to_node_str, u_int64_t duplicated_guid) :
        FabricErrGeneral(), p_node(p_node),
        direct_route_to_node_str(direct_route_to_node_str), duplicated_guid(duplicated_guid) {}
    ~FabricErrGuid() {}

    virtual string GetCSVErrorLine() = 0;

    inline string GetErrorLine() {
        return (this->description);
    }
};


class FabricErrDuplicatedNodeGuid : public FabricErrGuid {
public:
    FabricErrDuplicatedNodeGuid(IBNode *p_node, string direct_route_to_node_str, u_int64_t duplicated_guid);
    ~FabricErrDuplicatedNodeGuid() {}

    string GetCSVErrorLine();
};


class FabricErrDuplicatedPortGuid : public FabricErrGuid {
public:
    FabricErrDuplicatedPortGuid(IBNode *p_node, string direct_route_to_node_str, u_int64_t duplicated_guid);
    ~FabricErrDuplicatedPortGuid() {}

    string GetCSVErrorLine();
};


/*****************************************************/
/*****************************************************/
/*****************************************************/
class FabricErrNode : public FabricErrGeneral {
protected:
    IBNode *p_node;
public:
    FabricErrNode(IBNode *p_node) : FabricErrGeneral(), p_node(p_node) {}
    ~FabricErrNode() {}

    string GetCSVErrorLine();
    string GetErrorLine();
};


class FabricErrNodeNotRespond : public FabricErrNode {
public:
    FabricErrNodeNotRespond(IBNode *p_node, string desc);
    ~FabricErrNodeNotRespond() {}
};


class FabricErrNodeWrongConfig : public FabricErrNode {
public:
    FabricErrNodeWrongConfig(IBNode *p_node, string desc);
    ~FabricErrNodeWrongConfig() {}
};


class FabricErrNodeDuplicatedNodeDesc : public FabricErrNode {
public:
    FabricErrNodeDuplicatedNodeDesc(IBNode *p_node);
    ~FabricErrNodeDuplicatedNodeDesc() {}
};


class FabricErrNodeWrongFWVer : public FabricErrNode {
public:
    FabricErrNodeWrongFWVer(IBNode *p_node, string desc);
    ~FabricErrNodeWrongFWVer() {}
};


class FabricErrNodeNotSupportCap : public FabricErrNode {
public:
    FabricErrNodeNotSupportCap(IBNode *p_node, string desc);
    ~FabricErrNodeNotSupportCap() {}
};


/*****************************************************/
/*****************************************************/
/*****************************************************/
class FabricErrSM : public FabricErrGeneral {
protected:
    sm_info_obj_t *p_sm_obj;
public:
    FabricErrSM(sm_info_obj_t *p_sm_obj) : FabricErrGeneral(), p_sm_obj(p_sm_obj) {}
    ~FabricErrSM() {}

    string GetCSVErrorLine();
    string GetErrorLine();
};


class FabricErrSMNotFound : public FabricErrSM {
public:
    FabricErrSMNotFound(sm_info_obj_t *p_sm_obj);
    ~FabricErrSMNotFound() {}
};


class FabricErrSMUnknownState : public FabricErrSM {
public:
    FabricErrSMUnknownState(sm_info_obj_t *p_sm_obj);
    ~FabricErrSMUnknownState() {}
};


class FabricErrSMManyExists : public FabricErrSM {
public:
    FabricErrSMManyExists(sm_info_obj_t *p_sm_obj);
    ~FabricErrSMManyExists() {}
};


class FabricErrSMNotCorrect : public FabricErrSM {
public:
    FabricErrSMNotCorrect(sm_info_obj_t *p_sm_obj);
    ~FabricErrSMNotCorrect() {}
};


/*****************************************************/
/*****************************************************/
/*****************************************************/
class FabricErrPM : public FabricErrGeneral {
protected:
    IBPort *p_port;
public:
    FabricErrPM(IBPort *p_port) : FabricErrGeneral(), p_port(p_port) {}
    ~FabricErrPM() {}

    string GetCSVErrorLine();
    virtual string GetErrorLine();
};


class FabricErrPMCounterOverflow : public FabricErrPM {
public:
    FabricErrPMCounterOverflow(IBPort *p_port, string counter_name, u_int64_t overflow_value);
    ~FabricErrPMCounterOverflow() {}

    string GetErrorLine() { return (this->description); }
};


class FabricErrPMCounterExceedThreshold : public FabricErrPM {
public:
    FabricErrPMCounterExceedThreshold(IBPort *p_port, string counter_name,
            u_int64_t expected_value, u_int64_t actual_value);
    ~FabricErrPMCounterExceedThreshold() {}

    string GetErrorLine() { return (this->description); }
};


class FabricErrPMCounterNotSupported : public FabricErrPM {
public:
    FabricErrPMCounterNotSupported(IBPort *p_port, string counter_name);
    ~FabricErrPMCounterNotSupported() {}

    string GetErrorLine() { return (this->description); }
};


class FabricErrPMErrCounterIncreased : public FabricErrPM {
public:
    FabricErrPMErrCounterIncreased(IBPort *p_port, string counter_name,
            u_int64_t expected_value, u_int64_t actual_value);
    ~FabricErrPMErrCounterIncreased() {}
};


/*****************************************************/
/*****************************************************/
/*****************************************************/
typedef list < FabricErrPM * > list_p_pm_err;


class FabricErrPMCountersAll : public FabricErrGeneral {
private:
    IBPort *p_port;
    string err_line;
    string csv_err_line;
public:
    FabricErrPMCountersAll(IBPort *p_port, list_p_pm_err& pm_errors);
    ~FabricErrPMCountersAll() {}

    string GetCSVErrorLine();
    string GetErrorLine();
};


/*****************************************************/
/*****************************************************/
/*****************************************************/
class FabricErrBER : public FabricErrGeneral {
protected:
    IBPort *p_port;
public:
    FabricErrBER(IBPort *p_port) : FabricErrGeneral(), p_port(p_port) {}
    ~FabricErrBER() {}

    virtual string GetCSVErrorLine();
    virtual string GetErrorLine();
};


class FabricErrBERExceedThreshold : public FabricErrBER {
public:
    FabricErrBERExceedThreshold(IBPort *p_port, u_int64_t expected_value, u_int64_t actual_value);
    ~FabricErrBERExceedThreshold() {}
};


class FabricErrBERNoRcvData : public FabricErrBER {
public:
    FabricErrBERNoRcvData(IBPort *p_port);
    ~FabricErrBERNoRcvData() {}
};


class FabricErrBERIsZero : public FabricErrBER {
public:
    FabricErrBERIsZero(IBPort *p_port);
    ~FabricErrBERIsZero() {}
};


/*****************************************************/
/*****************************************************/
/*****************************************************/
class FabricErrPort : public FabricErrGeneral {
protected:
    IBPort *p_port;
public:
    FabricErrPort(IBPort *p_port) : FabricErrGeneral(), p_port(p_port) {}
    ~FabricErrPort() {}

    string GetCSVErrorLine();
    string GetErrorLine();
};


class FabricErrPortZeroLid : public FabricErrPort {
public:
    FabricErrPortZeroLid(IBPort *p_port);
    ~FabricErrPortZeroLid() {}
};


class FabricErrPortDuplicatedLid : public FabricErrPort {
public:
    FabricErrPortDuplicatedLid(IBPort *p_port);
    ~FabricErrPortDuplicatedLid() {}
};


class FabricErrPortNotRespond : public FabricErrPort {
public:
    FabricErrPortNotRespond(IBPort *p_port, string desc);
    ~FabricErrPortNotRespond() {}
};


class FabricErrPortWrongConfig : public FabricErrPort {
public:
    FabricErrPortWrongConfig(IBPort *p_port, string desc);
    ~FabricErrPortWrongConfig() {}
};


class FabricErrPortNotSupportCap : public FabricErrPort {
public:
    FabricErrPortNotSupportCap(IBPort *p_port, string desc);
    ~FabricErrPortNotSupportCap() {}
};


/*****************************************************/
/*****************************************************/
/*****************************************************/
class FabricErrDR : public FabricErrGeneral {
protected:
    string direct_route_str;
public:
    FabricErrDR(string direct_route_str);
    ~FabricErrDR() {}

    string GetCSVErrorLine();

    inline string GetErrorLine() {
        return (this->description);
    }
};


/*****************************************************/
/*****************************************************/
/*****************************************************/
class FabricErrLink : public FabricErrGeneral {
protected:
    IBPort *p_port1;
    IBPort *p_port2;
public:
    FabricErrLink(IBPort *p_port1, IBPort *p_port2) :
        FabricErrGeneral(), p_port1(p_port1), p_port2(p_port2) {}
    ~FabricErrLink() {}

    string GetCSVErrorLine();
    string GetErrorLine();
};


class FabricErrLinkLogicalStateWrong : public FabricErrLink {
public:
    FabricErrLinkLogicalStateWrong(IBPort *p_port1, IBPort *p_port2);
    ~FabricErrLinkLogicalStateWrong() {}
};


class FabricErrLinkLogicalStateNotActive : public FabricErrLink {
public:
    FabricErrLinkLogicalStateNotActive(IBPort *p_port1, IBPort *p_port2);
    ~FabricErrLinkLogicalStateNotActive() {}
};


class FabricErrLinkDifferentSpeed : public FabricErrLink {
public:
    FabricErrLinkDifferentSpeed(IBPort *p_port1, IBPort *p_port2);
    ~FabricErrLinkDifferentSpeed() {}
};


class FabricErrLinkUnexpectedSpeed : public FabricErrLink {
public:
    FabricErrLinkUnexpectedSpeed(IBPort *p_port1, IBPort *p_port2, string desc);
    ~FabricErrLinkUnexpectedSpeed() {}
};


class FabricErrLinkDifferentWidth : public FabricErrLink {
public:
    FabricErrLinkDifferentWidth(IBPort *p_port1, IBPort *p_port2);
    ~FabricErrLinkDifferentWidth() {}
};


class FabricErrLinkUnexpectedWidth : public FabricErrLink {
public:
    FabricErrLinkUnexpectedWidth(IBPort *p_port1, IBPort *p_port2, string desc);
    ~FabricErrLinkUnexpectedWidth() {}
};


class FabricErrLinkAutonegError : public FabricErrLink {
public:
    FabricErrLinkAutonegError(IBPort *p_port1, IBPort *p_port2, string desc);
    ~FabricErrLinkAutonegError() {}
};

/****************************************************/
/****************************************************/
/****************************************************/
class FabricErrPKeyMismatch : public FabricErrGeneral {
protected:
    IBPort *p_port1;
    IBPort *p_port2;
public:
    FabricErrPKeyMismatch(IBPort *p_port1, IBPort *p_port2,
            string port1_pkey_str, string port2_pkey_str);
    ~FabricErrPKeyMismatch() {}

    string GetCSVErrorLine();
    string GetErrorLine();
};


/****************************************************/
/****************************************************/
/****************************************************/
class FabricErrAGUID : public FabricErrGeneral {
protected:
    IBPort *p_port;
    string guid_owner_name;
    u_int64_t duplicated_guid;
    string guid_type;
public:
    FabricErrAGUID(IBPort *p_port, string owner_name,
            u_int64_t guid, string guid_type);
    ~FabricErrAGUID() {}

    string GetCSVErrorLine();
    string GetErrorLine();
};

class FabricErrAGUIDPortGuidDuplicated : public FabricErrAGUID {
public:
    FabricErrAGUIDPortGuidDuplicated(IBPort *p_port1, IBPort *p_port2,
            u_int64_t guid):
                FabricErrAGUID(p_port1, p_port2->getName(), guid, "port GUID") {}
    ~FabricErrAGUIDPortGuidDuplicated() {}
};

class FabricErrAGUIDNodeGuidDuplicated : public FabricErrAGUID {
public:
    FabricErrAGUIDNodeGuidDuplicated(IBPort *p_port, IBNode *p_node,
            u_int64_t guid):
                FabricErrAGUID(p_port, p_node->getName(), guid, "node GUID") {}
    ~FabricErrAGUIDNodeGuidDuplicated() {}
};

class FabricErrAGUIDSysGuidDuplicated : public FabricErrAGUID {
public:
    FabricErrAGUIDSysGuidDuplicated(IBPort *p_port, IBSystem *p_system,
            u_int64_t guid):
                FabricErrAGUID(p_port, p_system->name, guid, "system GUID") {}
    ~FabricErrAGUIDSysGuidDuplicated() {}
};


#endif          /* IBDIAG_FABRIC_ERRS_H */

