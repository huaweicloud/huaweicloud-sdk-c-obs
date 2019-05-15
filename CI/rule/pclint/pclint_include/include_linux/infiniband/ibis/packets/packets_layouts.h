
/*                  - Mellanox Confidential and Proprietary -
 *
 *  Copyright (C) 2010-2011, Mellanox Technologies Ltd.  ALL RIGHTS RESERVED.
 *
 *  Except as specifically permitted herein, no portion of the information,
 *  including but not limited to object code and source code, may be reproduced,
 *  modified, distributed, republished or otherwise exploited in any form or by
 *  any means for any purpose without the prior written permission of Mellanox
 *  Technologies Ltd. Use of software subject to the terms and conditions
 *  detailed in the file "LICENSE.txt".
 *
 */
 

/***
 *** This file was generated at "2012-02-07 18:09:30"
 *** by:
 ***    > /swgwork/wasim/svnroot/svn.tools/trunk/EAT.ME/adabe_plugins/adb2c/adb2pack.py --input adb/packets.adb -p packets
 ***/
#ifndef PACKETS_LAYOUTS_H
#define PACKETS_LAYOUTS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifndef ADABE2C_UTILS
#define ADABE2C_UTILS
//for htonl etc...
#if defined(_WIN32) || defined(_WIN64)
    #include<Winsock2.h>
#else   /* Linux */
    #include<arpa/inet.h>
#endif  /* Windows */


/************************************/
/************************************/
/************************************/
/* Endianess Defines */
#if __BYTE_ORDER == __LITTLE_ENDIAN
    #define PLATFORM_MEM "Little Endianess"
    #define _LITTLE_ENDIANESS
#else           /* __BYTE_ORDER == __BIG_ENDIAN */
    #define PLATFORM_MEM "Big Endianess"
    #define _BIG_ENDIANESS
#endif


/* Bit manipulation macros */

/* MASK generate a bit mask S bits width */
//#define MASK32(S)     ( ((u_int32_t) ~0L) >> (32-(S)) )
#define MASK8(S)        ( ((u_int8_t) ~0) >> (8-(S)) )

/* BITS generate a bit mask with bits O+S..O set (assumes 32 / 8 bit integer) */
//#define BITS32(O,S)   ( MASK32(S) << (O) )
#define BITS8(O,S)      ( MASK8(S) << (O) )

/* EXTRACT32/8 macro extracts S bits from (u_int32_t/u_int8_t)W with offset O
 * and shifts them O places to the right (right justifies the field extracted) */
//#define EXTRACT32(W,O,S)  ( ((W)>>(O)) & MASK32(S) )
#define EXTRACT8(W,O,S)     ( ((W)>>(O)) & MASK8(S) )


/* INSERT32/8 macro inserts S bits with offset O from field F into word W (u_int32_t/u_int8_t) */
//#define INSERT32(W,F,O,S)     ((W)= ( ( (W) & (~BITS32(O,S)) ) | (((F) & MASK32(S))<<(O)) ))
#define INSERT8(W,F,O,S)        ((W)= ( ( (W) & (~BITS8(O,S)) ) | (((F) & MASK8(S))<<(O)) ))

//#define INSERTF_32(W,O1,F,O2,S)   (INSERT32(W, EXTRACT32(F, O2, S), O1, S) )
#define INSERTF_8(W,O1,F,O2,S)      (INSERT8(W, EXTRACT8(F, O2, S), O1, S) )


#define PTR_64_OF_BUFF(buf, offset)     ((u_int64_t*)((u_int8_t*)(buf) + (offset)))
#define PTR_32_OF_BUFF(buf, offset)     ((u_int32_t*)((u_int8_t*)(buf) + (offset)))
#define PTR_8_OF_BUFF(buf, offset)      ((u_int8_t*)((u_int8_t*)(buf) + (offset)))
#define FIELD_64_OF_BUFF(buf, offset)   (*PTR_64_OF_BUFF(buf, offset))
#define FIELD_32_OF_BUFF(buf, offset)   (*PTR_32_OF_BUFF(buf, offset))
#define FIELD_8_OF_BUFF(buf, offset)    (*PTR_8_OF_BUFF(buf, offset))
#define DWORD_N(buf, n)                 FIELD_32_OF_BUFF((buf), (n) * 4)
#define BYTE_N(buf, n)                  FIELD_8_OF_BUFF((buf), (n))


#define MIN(a, b)   ((a) < (b) ? (a) : (b))


#define CPU_TO_BE32(x)  htonl(x)
#define BE32_TO_CPU(x)  ntohl(x)
#define CPU_TO_BE16(x)  htons(x)
#define BE16_TO_CPU(x)  ntohs(x)
#ifdef _LITTLE_ENDIANESS
    #define CPU_TO_BE64(x) (((u_int64_t)htonl((u_int32_t)((x) & 0xffffffff)) << 32) |                             ((u_int64_t)htonl((u_int32_t)((x >> 32) & 0xffffffff))))

    #define BE64_TO_CPU(x) (((u_int64_t)ntohl((u_int32_t)((x) & 0xffffffff)) << 32) |                             ((u_int64_t)ntohl((u_int32_t)((x >> 32) & 0xffffffff))))
#else
    #define CPU_TO_BE64(x) (x)
    #define BE64_TO_CPU(x) (x)
#endif


/* define macros to the architecture of the CPU */
#if defined(__linux) || defined(__FreeBSD__)             /* __linux || __FreeBSD__ */
#   if defined(__i386__)
#       define ARCH_x86
#   elif defined(__x86_64__)
#       define ARCH_x86_64
#   elif defined(__ia64__)
#       define ARCH_ia64
#   elif defined(__PPC64__)
#       define ARCH_ppc64
#   elif defined(__PPC__)
#       define ARCH_ppc
#   else
#       error Unknown CPU architecture using the linux OS
#   endif
#elif defined(__MINGW32__) || defined(__MINGW64__)		/* Windows MINGW */
#   if defined(__MINGW32__)
#       define ARCH_x86
#   elif defined(__MINGW64__)
#       define ARCH_x86_64
#   else
#       error Unknown CPU architecture using the windows-mingw OS
#   endif
#elif defined(_WIN32) || defined(_WIN64)                /* Windows */
#   if defined(_WIN32)
#       define ARCH_x86
#   elif defined(_WIN64)
#       define ARCH_x86_64
#   else
#       error Unknown CPU architecture using the windows OS
#   endif
#else                                                   /* Unknown */
#   error Unknown OS
#endif


/* define macros for print fields */
#define U32D_FMT    "%u"
#define U32H_FMT    "0x%08x"
#define UH_FMT		"0x%x"
#define STR_FMT     "%s"
#define U16H_FMT    "0x%04x"
#define U8H_FMT     "0x%02x"
#if defined (ARCH_ia64) || defined(ARCH_x86_64) || defined(ARCH_ppc64)
#    define U64D_FMT    "%lu"
#    define U64H_FMT    "0x%016lx"
#	 define U48H_FMT	"0x%012lx" 
#elif defined(ARCH_x86) || defined(ARCH_ppc)
#    define U64D_FMT    "%llu"
#    define U64H_FMT    "0x%016llx"
#	 define U48H_FMT	"0x%012llx" 
#else
#   error Unknown architecture
#endif  /* ARCH */


#if !defined(_WIN32) && !defined(_WIN64)    		/* Linux */
    #include <sys/types.h>
#elif defined(__MINGW32__) || defined(__MINGW64__) 	/* windows - mingw */
    #include <stdint.h>                                                                                                                                           
    #ifndef   MFT_TOOLS_VARS                                                                                                                                      
		#define MFT_TOOLS_VARS                                                                                                                                    
		typedef uint8_t  u_int8_t;                                                                                                                                
		typedef uint16_t u_int16_t;                                                                                                                               
		typedef uint32_t u_int32_t;                                                                                                                               
		typedef uint64_t u_int64_t;                                                                                                                               
	#endif
#else   											/* Windows */
    typedef __int8                  int8_t;
    typedef unsigned __int8         u_int8_t;
    typedef __int16                 int16_t;
    typedef unsigned __int16        u_int16_t;
    typedef __int32                 int32_t;
    typedef unsigned __int32        u_int32_t;
    typedef __int64                 int64_t;
    typedef unsigned __int64        u_int64_t;
#endif


/************************************/
/************************************/
/************************************/
struct attr_format
{
    const char* name;
    const char* val;
};

struct enum_format
{
    const char*     name;
    int			    val;
};

struct field_format
{
    const char*             full_name;
    const char*             desc;
    int                     offs;
    int                     size;
    int                     enums_len;
    struct enum_format*     enums;
    int                     attrs_len;
    struct attr_format*     attrs;
};

struct node_format
{
    const char*          name;
    const char*          desc;
    int                  size;
    int                  is_union;
    int                  attrs_len;
    struct attr_format*  attrs;
    int                  fields_len;
    struct field_format* fields;
};

struct node_db
{
    int                 nodes_len;
    struct node_format* nodes;
};

/************************************/
/************************************/
/************************************/
void print_raw(FILE* file, void* buff, int buff_len);
u_int64_t pop_from_buf(const u_int8_t *buff, u_int32_t bit_offset, u_int32_t field_size);
void push_to_buf(u_int8_t *buff, u_int32_t bit_offset, u_int32_t field_size, u_int64_t field_value);
const char* db_get_field_enum_name(struct field_format* field, int val);
int db_get_field_enum_val(struct field_format* field, const char* name);
const char* db_get_field_attr(struct field_format* field, const char* attr_name);
const char* db_get_node_attr(struct node_format* node, const char* attr_name);
struct node_format* db_find_node(struct node_db* db, const char* node_name);
struct field_format* db_find_field(struct node_format*, const char* field_name);
#endif // ADABE2C_UTILS
/* Description -   */
/* Size in bytes - 16 */
struct PSID_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - PS - ID */
	/* 0.24 - 16.23 */
	 u_int8_t PSID[16];
};

/* Description -   */
/* Size in bytes - 16 */
struct GID_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 16.31 */
	 u_int32_t DWord[4];
};

/* Description -   */
/* Size in bytes - 8 */
struct uint64bit {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 4.31 */
	 u_int32_t High;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 8.31 */
	 u_int32_t Low;
};

/* Description -   */
/* Size in bytes - 2 */
struct CCTI_Entry_ListElement {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - This is the multiplier used when calculating the injection rate delay */
	/* 0.0 - 0.13 */
	 u_int16_t CCT_Multiplier;
	/* Description - This is the shift value used when calculating the injection rate delay */
	/* 0.14 - 0.15 */
	 u_int8_t CCT_Shift;
};

/* Description -   */
/* Size in bytes - 8 */
struct CACongestionEntryListElement {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - When the CCTI is equal to this value, an event is logged in the CAs cyclic event log. */
	/* 0.0 - 0.7 */
	 u_int8_t Trigger_Threshold;
	/* Description - The number to be added to the table Index (CCTI) on the receipt of a BECN. */
	/* 0.8 - 0.15 */
	 u_int8_t CCTI_Increase;
	/* Description - When the timer expires it will be reset to its specified value, and 1 will be decremented from the CCTI. */
	/* 0.16 - 4.31 */
	 u_int16_t CCTI_Timer;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - The minimum value permitted for the CCTI. */
	/* 4.24 - 8.31 */
	 u_int8_t CCTI_Min;
};

/* Description -   */
/* Size in bytes - 16 */
struct CongestionLogEventListCAElement {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Local QP that reached CN Threshold. Set to zero if port threshold reached. */
	/* 0.8 - 4.31 */
	 u_int32_t Local_QP_CN_Entry;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - Service Type of local QP: 0 - RC, 1 - UC, 2 - RD, 3 - UD */
	/* 4.0 - 4.3 */
	 u_int8_t Service_Type_CN_Entry;
	/* Description - Service Level associated with local QP */
	/* 4.4 - 4.7 */
	 u_int8_t SL_CN_Entry;
	/* Description - Remote QP that is connected to local QP. Set to zero for datagram QPs. */
	/* 4.8 - 8.31 */
	 u_int32_t Remote_QP_Number_CN_Entry;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description - LID of remote port that is connected to local QP. Set to zero for datagram service. */
	/* 8.16 - 12.31 */
	 u_int16_t Remote_LID_CN_Entry;
/*---------------- DWORD[3] (Offset 0xc) ----------------*/
	/* Description - Time stamp of congestion event */
	/* 12.0 - 16.31 */
	 u_int32_t Timestamp;
};

/* Description -   */
/* Size in bytes - 12 */
struct CongestionEntryListSwitchElement {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Destination lid of congestion event */
	/* 0.0 - 0.15 */
	 u_int16_t DLID;
	/* Description - Source lid of congestion event */
	/* 0.16 - 4.31 */
	 u_int16_t SLID;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - Service level of congestion event */
	/* 4.28 - 8.31 */
	 u_int8_t SL;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description - Time stamp of congestion event */
	/* 8.0 - 12.31 */
	 u_int32_t Timestamp;
};

/* Description -  SW Info */
/* Size in bytes - 32 */
struct SWInfo_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.7 */
	 u_int8_t SubMinor;
	/* Description -  */
	/* 0.8 - 0.15 */
	 u_int8_t Minor;
	/* Description -  */
	/* 0.16 - 0.23 */
	 u_int8_t Major;
	/* Description -  */
	/* 0.24 - 4.31 */
	 u_int8_t Reserved8;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 32.31 */
	 u_int32_t Reserved_Dword[7];
};

/* Description -  FW Info */
/* Size in bytes - 64 */
struct FWInfo_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.7 */
	 u_int8_t SubMinor;
	/* Description -  */
	/* 0.8 - 0.15 */
	 u_int8_t Minor;
	/* Description -  */
	/* 0.16 - 0.23 */
	 u_int8_t Major;
	/* Description -  */
	/* 0.24 - 4.31 */
	 u_int8_t Reserved8;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 8.31 */
	 u_int32_t BuildID;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description -  */
	/* 8.0 - 8.15 */
	 u_int16_t Year;
	/* Description -  */
	/* 8.16 - 8.23 */
	 u_int8_t Day;
	/* Description -  */
	/* 8.24 - 12.31 */
	 u_int8_t Month;
/*---------------- DWORD[3] (Offset 0xc) ----------------*/
	/* Description -  */
	/* 12.0 - 12.15 */
	 u_int16_t Hour;
	/* Description -  */
	/* 12.16 - 16.31 */
	 u_int16_t Reserved16;
/*---------------- DWORD[4] (Offset 0x10) ----------------*/
	/* Description -  */
	/* 16.0 - 32.31 */
	 struct PSID_Block_Element PSID;
/*---------------- DWORD[8] (Offset 0x20) ----------------*/
	/* Description -  */
	/* 32.0 - 36.31 */
	 u_int32_t INI_File_Version;
/*---------------- DWORD[9] (Offset 0x24) ----------------*/
	/* Description -  */
	/* 36.0 - 40.31 */
	 u_int32_t Extended_Major;
/*---------------- DWORD[10] (Offset 0x28) ----------------*/
	/* Description -  */
	/* 40.0 - 44.31 */
	 u_int32_t Extended_Minor;
/*---------------- DWORD[11] (Offset 0x2c) ----------------*/
	/* Description -  */
	/* 44.0 - 48.31 */
	 u_int32_t Extended_SubMinor;
/*---------------- DWORD[12] (Offset 0x30) ----------------*/
	/* Description -  */
	/* 48.0 - 64.31 */
	 u_int32_t Reserved[4];
};

/* Description -  HW Info */
/* Size in bytes - 32 */
struct HWInfo_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.15 */
	 u_int16_t DeviceID;
	/* Description -  */
	/* 0.16 - 4.31 */
	 u_int16_t DeviceHWRevision;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - Reserved */
	/* 4.0 - 28.31 */
	 u_int32_t Reserved_Dword[6];
/*---------------- DWORD[7] (Offset 0x1c) ----------------*/
	/* Description - Time (in sec) since last reset */
	/* 28.0 - 32.31 */
	 u_int32_t UpTime;
};

/* Description -   */
/* Size in bytes - 56 */
struct CC_KeyViolation {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.8 - 0.15 */
	 u_int8_t Method;
	/* Description -  */
	/* 0.16 - 4.31 */
	 u_int16_t SourceLID;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.16 - 8.31 */
	 u_int16_t ArrtibuteID;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description -  */
	/* 8.0 - 12.31 */
	 u_int32_t AttributeModifier;
/*---------------- DWORD[3] (Offset 0xc) ----------------*/
	/* Description -  */
	/* 12.8 - 16.31 */
	 u_int32_t QP;
/*---------------- DWORD[4] (Offset 0x10) ----------------*/
	/* Description - Congestion Control key, is used to validate the source of Congestion Control Mads. */
	/* 16.0 - 24.31 */
	 u_int64_t CC_Key;
/*---------------- DWORD[6] (Offset 0x18) ----------------*/
	/* Description -  */
	/* 24.0 - 40.31 */
	 struct GID_Block_Element SourceGID;
/*---------------- DWORD[10] (Offset 0x28) ----------------*/
	/* Description -  */
	/* 40.24 - 56.23 */
	 u_int8_t Padding[16];
};

/* Description -   */
/* Size in bytes - 128 */
struct CCTI_Entry_List {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - List of up to 64 CongestionControlTableEntries. see table 521 p1686 for data format */
	/* 0.0 - 128.31 */
	 struct CCTI_Entry_ListElement CCTI_Entry_ListElement[64];
};

/* Description -   */
/* Size in bytes - 128 */
struct CACongestionEntryList {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - List of sixteen CACongestionEntries, one per service level. (See Table 519 on Page 1684.) */
	/* 0.0 - 128.31 */
	 struct CACongestionEntryListElement CACongestionEntryListElement[16];
};

/* Description -   */
/* Size in bytes - 4 */
struct SwitchPortCongestionSettingElement {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - When control_type=0 this field contains the port marking_rate. When control_type=1 this field is reserved */
	/* 0.0 - 0.15 */
	 u_int16_t Cong_Parm;
	/* Description - When control_type=0 this field contains the minimum size of packets that may be marked with a FECN. When control_type=1 this field is reserved. */
	/* 0.16 - 0.23 */
	 u_int8_t Packet_Size;
	/* Description - When control_type=0 this field contains the congestion threshold value (Threshold) for this port. When Control Type is 1, contains the credit starvation threshold (CS_Threshold) value for this port. */
	/* 0.24 - 0.27 */
	 u_int8_t Threshold;
	/* Description - Indicates which type of attribute is being set: 0b = Congestion Control parameters are being set. 1b = Credit Starvation parameters are being set. */
	/* 0.30 - 0.30 */
	 u_int8_t Control_Type;
	/* Description - When set to 1, indicates this switch port congestion setting element is valid. */
	/* 0.31 - 4.31 */
	 u_int8_t Valid;
};

/* Description -   */
/* Size in bytes - 32 */
struct UINT256 {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 4.31 */
	 u_int32_t Mask_255_224;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 8.31 */
	 u_int32_t Mask_223_192;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description -  */
	/* 8.0 - 12.31 */
	 u_int32_t Mask_191_160;
/*---------------- DWORD[3] (Offset 0xc) ----------------*/
	/* Description -  */
	/* 12.0 - 16.31 */
	 u_int32_t Mask_159_128;
/*---------------- DWORD[4] (Offset 0x10) ----------------*/
	/* Description -  */
	/* 16.0 - 20.31 */
	 u_int32_t Mask_127_96;
/*---------------- DWORD[5] (Offset 0x14) ----------------*/
	/* Description -  */
	/* 20.0 - 24.31 */
	 u_int32_t Mask_95_64;
/*---------------- DWORD[6] (Offset 0x18) ----------------*/
	/* Description -  */
	/* 24.0 - 28.31 */
	 u_int32_t Mask_63_32;
/*---------------- DWORD[7] (Offset 0x1c) ----------------*/
	/* Description - bit0: port 0, bit1: port1... bit254: port 254, bit255: reserved */
	/* 28.0 - 32.31 */
	 u_int32_t Mask_31_0;
};

/* Description -   */
/* Size in bytes - 4 */
struct CC_SwitchCongestionSetting_Control_Map {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.0 */
	 u_int8_t Marking_RateIsValid;
	/* Description -  */
	/* 0.1 - 0.1 */
	 u_int8_t CS_ThresholdAndCS_ReturnDelayIsValid;
	/* Description -  */
	/* 0.2 - 0.2 */
	 u_int8_t ThresholdAndPacket_SizeIsValid;
	/* Description -  */
	/* 0.3 - 0.3 */
	 u_int8_t Credit_MaskIsValid;
	/* Description -  */
	/* 0.4 - 0.4 */
	 u_int8_t Victim_MaskIsValid;
};

/* Description -  array of at most 13 recent events */
/* Size in bytes - 208 */
struct CongestionLogEventListCA {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 208.31 */
	 struct CongestionLogEventListCAElement CongestionLogEventListCAElement[13];
};

/* Description -  array of at most 15 recent events */
/* Size in bytes - 180 */
struct CongestionEntryListSwitch {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 180.31 */
	 struct CongestionEntryListSwitchElement CongestionEntryListSwitchElement[15];
};

/* Description -   */
/* Size in bytes - 8 */
struct PortSampleControlOptionMask {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.0 */
	 u_int8_t PlaceHolder;
	/* Description -  */
	/* 0.16 - 0.16 */
	 u_int8_t SwPortVLCongestion_SWPortVLCongestion_n;
	/* Description -  */
	/* 0.17 - 0.17 */
	 u_int8_t PortRcvConCtrl_PortPktRcvFECN;
	/* Description -  */
	/* 0.18 - 0.18 */
	 u_int8_t PortRcvConCtrl_PortPktRcvBECN;
	/* Description -  */
	/* 0.19 - 0.19 */
	 u_int8_t PortSLRcvFECN_PortSLRcvFECN_n;
	/* Description -  */
	/* 0.20 - 0.20 */
	 u_int8_t PortSLRcvBECN_PortSLRcvBECN_n;
	/* Description -  */
	/* 0.21 - 0.21 */
	 u_int8_t PortXmitConCtrl_PortXmitTimeCong;
	/* Description -  */
	/* 0.22 - 0.22 */
	 u_int8_t PortVLXmitTimeCong_PortVLXmitTimeCong_n;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.1 - 4.1 */
	 u_int8_t PortXmitQueue_n;
	/* Description -  */
	/* 4.2 - 4.2 */
	 u_int8_t PortXmitDataVl_n;
	/* Description -  */
	/* 4.3 - 4.3 */
	 u_int8_t PortRcvDataVl_n;
	/* Description -  */
	/* 4.4 - 4.4 */
	 u_int8_t PortXmitPktVl_n;
	/* Description -  */
	/* 4.5 - 4.5 */
	 u_int8_t PortRcvPktVl_n;
	/* Description -  */
	/* 4.6 - 4.6 */
	 u_int8_t PortRcvErrorDetails_PortLocalPhysicalErrors;
	/* Description -  */
	/* 4.7 - 4.7 */
	 u_int8_t PortRcvErrorDetails_PortMalformedPacketErrors;
	/* Description -  */
	/* 4.8 - 4.8 */
	 u_int8_t PortRcvErrorDetails_PortBufferOverrunErrors;
	/* Description -  */
	/* 4.9 - 4.9 */
	 u_int8_t PortRcvErrorsDetails_PortDLIDMappingErrors;
	/* Description -  */
	/* 4.10 - 4.10 */
	 u_int8_t PortRcvErrorsDetails_PortVlMappingErrors;
	/* Description -  */
	/* 4.11 - 4.11 */
	 u_int8_t PortRcvErrorDetails_PortLoopingErrors;
	/* Description -  */
	/* 4.12 - 4.12 */
	 u_int8_t PortXmitDiscardDetails_PortInactiveDiscards;
	/* Description -  */
	/* 4.13 - 4.13 */
	 u_int8_t PortXmitDiscardsDetails_PortNeighborMTUDiscards;
	/* Description -  */
	/* 4.14 - 4.14 */
	 u_int8_t PortXmitDiscardDetails_PortSwLifetimeLimitDiscards;
	/* Description -  */
	/* 4.15 - 4.15 */
	 u_int8_t PortXmitDiscardDetails_PortSwHOQLifeLimitDiscards;
	/* Description -  */
	/* 4.16 - 4.16 */
	 u_int8_t PortOpRcvCounters_PortOpRcvPkts;
	/* Description -  */
	/* 4.17 - 4.17 */
	 u_int8_t PortOpRcvCounters_PortOpRcvData;
	/* Description -  */
	/* 4.18 - 4.18 */
	 u_int8_t PortFlowCtlCounters_PortXmitFlowPkts;
	/* Description -  */
	/* 4.19 - 4.19 */
	 u_int8_t PortFlowCtlCounters_PortRcvFlowPkts;
	/* Description -  */
	/* 4.20 - 4.20 */
	 u_int8_t PortVLOpPackets_PortVLOpPackets_n;
	/* Description -  */
	/* 4.21 - 4.21 */
	 u_int8_t PortVLOpData_PortVLOpData_n;
	/* Description -  */
	/* 4.22 - 4.22 */
	 u_int8_t PortVlXmitFlowCtlUpdateErrors_PortVLXmitCtlUpdateErrors_n;
	/* Description -  */
	/* 4.23 - 4.23 */
	 u_int8_t PortVLXmitWaitCounters_PortVLXmitWait_n;
	/* Description -  */
	/* 4.28 - 4.28 */
	 u_int8_t PortExtendedSpeedsCounter;
};

/* Description -   */
/* Size in bytes - 2 */
struct PortCountersExtended_Mask_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.0 */
	 u_int8_t SetPortXmitData;
	/* Description -  */
	/* 0.1 - 0.1 */
	 u_int8_t SetPortRcvData;
	/* Description -  */
	/* 0.2 - 0.2 */
	 u_int8_t SetPortXmitPkts;
	/* Description -  */
	/* 0.3 - 0.3 */
	 u_int8_t SetPortRcvPkts;
	/* Description -  */
	/* 0.4 - 0.4 */
	 u_int8_t SetPortUnicastXmitPkts;
	/* Description -  */
	/* 0.5 - 0.5 */
	 u_int8_t SetPortUnicastRcvPkts;
	/* Description -  */
	/* 0.6 - 0.6 */
	 u_int8_t SetPortMulticastXmitPkts;
	/* Description -  */
	/* 0.7 - 0.7 */
	 u_int8_t SetPortMulticastRcvPkts;
	/* Description -  */
	/* 0.8 - 0.15 */
	 u_int8_t Reserved;
};

/* Description -   */
/* Size in bytes - 1 */
struct PortCounters_Mask2_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.0 */
	 u_int8_t SetPortXmitWait;
};

/* Description -   */
/* Size in bytes - 2 */
struct PortCounters_Mask_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.0 */
	 u_int8_t SetSymbolErrorCounter;
	/* Description -  */
	/* 0.1 - 0.1 */
	 u_int8_t SetLinkErrorRecoveryCounter;
	/* Description -  */
	/* 0.2 - 0.2 */
	 u_int8_t SetLinkDownedCounter;
	/* Description -  */
	/* 0.3 - 0.3 */
	 u_int8_t SetPortRcvErrors;
	/* Description -  */
	/* 0.4 - 0.4 */
	 u_int8_t SetPortRcvRemotePhysicalErrors;
	/* Description -  */
	/* 0.5 - 0.5 */
	 u_int8_t SetPortRcvSwitchRelayErrors;
	/* Description -  */
	/* 0.6 - 0.6 */
	 u_int8_t SetPortXmitDiscards;
	/* Description -  */
	/* 0.7 - 0.7 */
	 u_int8_t SetPortXmitConstraintErrors;
	/* Description -  */
	/* 0.8 - 0.8 */
	 u_int8_t SetPortRcvConstraintErrors;
	/* Description -  */
	/* 0.9 - 0.9 */
	 u_int8_t SetLocalLinkIntegrityErrors;
	/* Description -  */
	/* 0.10 - 0.10 */
	 u_int8_t SetExcessiveBufferOverrunErrors;
	/* Description -  */
	/* 0.11 - 0.11 */
	 u_int8_t SetVL15Dropped;
	/* Description -  */
	/* 0.12 - 0.12 */
	 u_int8_t SetPortXmitData;
	/* Description -  */
	/* 0.13 - 0.13 */
	 u_int8_t SetPortRcvData;
	/* Description -  */
	/* 0.14 - 0.14 */
	 u_int8_t SetPortXmitPkts;
	/* Description -  */
	/* 0.15 - 0.15 */
	 u_int8_t SetPortRcvPkts;
};

/* Description -   */
/* Size in bytes - 4 */
struct LID_Port_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.15 */
	 u_int16_t LID;
	/* Description -  */
	/* 0.16 - 0.16 */
	 u_int8_t Valid;
	/* Description -  */
	/* 0.17 - 0.19 */
	 u_int8_t LMC;
	/* Description -  */
	/* 0.20 - 0.23 */
	 u_int8_t Reserved;
	/* Description -  */
	/* 0.24 - 4.31 */
	 u_int8_t Port;
};

/* Description -  VL Weight */
/* Size in bytes - 2 */
struct VL_Weight_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.3 */
	 u_int8_t Reserved;
	/* Description -  */
	/* 0.4 - 0.7 */
	 u_int8_t VL;
	/* Description -  */
	/* 0.8 - 0.15 */
	 u_int8_t Weight;
};

/* Description -  Partition Key */
/* Size in bytes - 2 */
struct P_Key_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.14 */
	 u_int16_t P_KeyBase;
	/* Description -  */
	/* 0.15 - 0.15 */
	 u_int8_t Membership_Type;
};

/* Description -   */
/* Size in bytes - 64 */
struct GUID_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 64.31 */
	 struct uint64bit GUID[8];
};

/* Description -   */
/* Size in bytes - 8 */
struct TID_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 4.31 */
	 u_int32_t High;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 8.31 */
	 u_int32_t Low;
};

/* Description -   */
/* Size in bytes - 128 */
struct VendorSpec_GeneralInfo {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 32.31 */
	 struct HWInfo_Block_Element HWInfo;
/*---------------- DWORD[8] (Offset 0x20) ----------------*/
	/* Description -  */
	/* 32.0 - 96.31 */
	 struct FWInfo_Block_Element FWInfo;
/*---------------- DWORD[24] (Offset 0x60) ----------------*/
	/* Description -  */
	/* 96.0 - 128.31 */
	 struct SWInfo_Block_Element SWInfo;
};

/* Description -   */
/* Size in bytes - 80 */
struct CC_Notice {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.23 */
	 u_int32_t ProducerType_VendorID;
	/* Description -  */
	/* 0.24 - 0.30 */
	 u_int8_t Type;
	/* Description -  */
	/* 0.31 - 4.31 */
	 u_int8_t IsGeneric;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 4.15 */
	 u_int16_t IssuerLID;
	/* Description -  */
	/* 4.16 - 8.31 */
	 u_int16_t TrapNumber;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description -  */
	/* 8.0 - 64.31 */
	 struct CC_KeyViolation CC_KeyViolation;
/*---------------- DWORD[16] (Offset 0x40) ----------------*/
	/* Description -  */
	/* 64.0 - 80.31 */
	 struct GID_Block_Element IssuerGID;
};

/* Description -   */
/* Size in bytes - 4 */
struct CC_TimeStamp {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Free running clock that provieds relative time infomation for a device. Time is kept in 1.024 usec units. */
	/* 0.0 - 4.31 */
	 u_int32_t TimeStamp;
};

/* Description -   */
/* Size in bytes - 132 */
struct CC_CongestionControlTable {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Maximum valid CCTI for this table. */
	/* 0.16 - 4.31 */
	 u_int16_t CCTI_Limit;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - List of up to 64 CongestionControlTableEntries. */
	/* 4.0 - 132.31 */
	 struct CCTI_Entry_List CCTI_Entry_List;
};

/* Description -   */
/* Size in bytes - 132 */
struct CC_CACongestionSetting {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - List of sixteen CACongestionEntries, one per service level. */
	/* 0.0 - 0.15 */
	 u_int16_t Control_Map;
	/* Description - Congestion attributes for this port: bit0 = 0: QP based CC; bit0 = 1: SL/Port based CC; All other bits are reserved. */
	/* 0.16 - 4.31 */
	 u_int16_t Port_Control;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 132.31 */
	 struct CACongestionEntryList CACongestionEntryList;
};

/* Description -   */
/* Size in bytes - 128 */
struct CC_SwitchPortCongestionSetting {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - see table 516 and table 517 p1681 for data format */
	/* 0.0 - 128.31 */
	 struct SwitchPortCongestionSettingElement SwitchPortCongestionSettingElement[32];
};

/* Description -   */
/* Size in bytes - 76 */
struct CC_SwitchCongestionSetting {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 4.31 */
	 struct CC_SwitchCongestionSetting_Control_Map Control_Map;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - If the bit is set to 1, then the port corresponding to that bit shall mark packets that enconter congestion with a FECN, wheter they are the source or victim of congestion. */
	/* 4.0 - 36.31 */
	 struct UINT256 Victim_Mask;
/*---------------- DWORD[9] (Offset 0x24) ----------------*/
	/* Description - If the bit is set to 1, then the port corresponding to that bit shall apply Credit Starvation. */
	/* 36.0 - 68.31 */
	 struct UINT256 Credit_Mask;
/*---------------- DWORD[17] (Offset 0x44) ----------------*/
	/* Description -  */
	/* 68.12 - 68.15 */
	 u_int8_t CS_Threshold;
	/* Description -  */
	/* 68.16 - 68.23 */
	 u_int8_t Packet_Size;
	/* Description -  */
	/* 68.28 - 72.31 */
	 u_int8_t Threshold;
/*---------------- DWORD[18] (Offset 0x48) ----------------*/
	/* Description -  */
	/* 72.0 - 72.15 */
	 u_int16_t Marking_Rate;
	/* Description -  */
	/* 72.16 - 76.31 */
	 u_int16_t CS_ReturnDelay;
};

/* Description -   */
/* Size in bytes - 220 */
struct CC_CongestionLogCA {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Number of CongestionLogEvents since log last sent */
	/* 0.0 - 0.15 */
	 u_int16_t ThresholdEventCounter;
	/* Description - Bit0 - 1: - CC_Key lease period timer active, Bit0 - 0: lease timer incative */
	/* 0.16 - 0.23 */
	 u_int8_t CongestionFlags;
	/* Description - 0x1 - switch , 0x2 - CA */
	/* 0.24 - 4.31 */
	 u_int8_t LogType;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - List of sixteen entries, one per service level. */
	/* 4.16 - 8.31 */
	 u_int16_t ThresholdCongestionEventMap;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description - 2^CurrentTimeStamp*1.24uSec = time stamp, wrap around ~1.22hr */
	/* 8.0 - 12.31 */
	 u_int32_t CurrentTimeStamp;
/*---------------- DWORD[3] (Offset 0xc) ----------------*/
	/* Description - for data format see spec 1.2.1 vol 1 p.1678 */
	/* 12.0 - 220.31 */
	 struct CongestionLogEventListCA CongestionLogEvent;
};

/* Description -   */
/* Size in bytes - 220 */
struct CC_CongestionLogSwitch {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Number of CongestionLogEvents since log last sent */
	/* 0.0 - 0.15 */
	 u_int16_t LogEventsCounter;
	/* Description - Bit0 - 1: CC_Key lease period timer active, Bit0 - 0: lease timer incative */
	/* 0.16 - 0.23 */
	 u_int8_t CongestionFlags;
	/* Description - 0x1 - switch , 0x2 - CA */
	/* 0.24 - 4.31 */
	 u_int8_t LogType;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - 2^CurrentTimeStamp*1.24uSec = time stamp, wrap around ~1.22hr */
	/* 4.0 - 8.31 */
	 u_int32_t CurrentTimeStamp;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description - If a bit is set then the corresponding port has marked packets with FECN, bit 0 - reserve, bit 1 - port 1 */
	/* 8.0 - 40.31 */
	 struct UINT256 PortMap;
/*---------------- DWORD[10] (Offset 0x28) ----------------*/
	/* Description - for data format see spec 1.2.1 vol 1 p.1678 */
	/* 40.0 - 220.31 */
	 struct CongestionEntryListSwitch CongestionEntryList;
};

/* Description -   */
/* Size in bytes - 16 */
struct CC_CongestionKeyInfo {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Congestion Control key, is used to validate the source of Congestion Control Mads. A value of 0 means that no CC_Key checks is ever performed by the CCMgrA. */
	/* 0.0 - 8.31 */
	 u_int64_t CC_Key;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description - How long the CC Key protect bit is to remain non-zero. */
	/* 8.0 - 8.15 */
	 u_int16_t CC_KeyLeasePeriod;
	/* Description -  */
	/* 8.31 - 12.31 */
	 u_int8_t CC_KeyProtectBit;
/*---------------- DWORD[3] (Offset 0xc) ----------------*/
	/* Description - Number of received MADs that violated CC Key. */
	/* 12.16 - 16.31 */
	 u_int16_t CC_KeyViolations;
};

/* Description -   */
/* Size in bytes - 4 */
struct CC_CongestionInfo {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - CA only - Number of 64 entry block in CCT  */
	/* 0.8 - 0.15 */
	 u_int8_t ControlTableCap;
	/* Description - Bit0 - 1: Switch support Creadit Starvation, Bit0 - 0: Switch doesn't support CS */
	/* 0.16 - 4.31 */
	 u_int16_t CongestionInfo;
};

/* Description -   */
/* Size in bytes - 128 */
struct PM_PortExtendedSpeedsCounters {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.16 - 0.23 */
	 u_int8_t PortSelect;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description - Bit 0 - SyncHeaderErrorCounter\;Bit 1 - UnknownBlockCounter\;Bit 2 - ErrorDetectionCounterLane0\;Bit 3 ErrorDetectionCounterLane1\;Bit 4 ErrorDetectionCounterLane2\;Bit 5 ErrorDetectionCounterLane3\;Bit 6 ErrorDetectionCounterLane4\;Bit 7 ErrorDetectionCounterLane5\;Bit 8 ErrorDetectionCounterLane6\;Bit 9 ErrorDetectionCounterLane7\;Bit 10 ErrorDetectionCounterLane8\;Bit 11 ErrorDetectionCounterLane9\;Bit 12 ErrorDetectionCounterLane10\;Bit 13 ErrorDetectionCounterLane11\;Bit 14 FECCorrectableBlockCounterLane0\;Bit 15 FECCorrectableBlockCounterLane1\;Bit 16 FECCorrectableBlockCounterLane2\;Bit 17 FECCorrectableBlockCounterLane3\;Bit 18 FECCorrectableBlockCounterLane4\;Bit 19 FECCorrectableBlockCounterLane5\;Bit 20 FECCorrectableBlockCounterLane6\;Bit 21 FECCorrectableBlockCounterLane7\;Bit 22 FECCorrectableBlockCounterLane8\;Bit 23 FECCorrectableBlockCounterLane9\;Bit 24 FECCorrectableBlockCounterLane10\;Bit 25 FECCorrectableBlockCounterLane11\;Bit 26 FECUncorrectableBlockCounterLane0\;Bit 27 FECUncorrectableBlockCounterLane1\;Bit 28 FECUncorrectableBlockCounterLane2\;Bit 29 FECUncorrectableBlockCounterLane3\;Bit 30 FECUncorrectableBlockCounterLane4\;Bit 31 FECUncorrectableBlockCounterLane5\;Bit 32 FECUncorrectableBlockCounterLane6\;Bit 33 FECUncorrectableBlockCounterLane7\;Bit 34 FECUncorrectableBlockCounterLane8\;Bit 35 FECUncorrectableBlockCounterLane9\;Bit 36 FECUncorrectableBlockCounterLane10\;Bit 37 FECUncorrectableBlockCounterLane11\;Bits 38-63 - Reserved\;\; */
	/* 8.0 - 16.31 */
	 u_int64_t CounterSelect;
/*---------------- DWORD[4] (Offset 0x10) ----------------*/
	/* Description -  */
	/* 16.0 - 16.15 */
	 u_int16_t UnknownBlockCounter;
	/* Description -  */
	/* 16.16 - 20.31 */
	 u_int16_t SyncHeaderErrorCounter;
/*---------------- DWORD[5] (Offset 0x14) ----------------*/
	/* Description -  */
	/* 20.16 - 44.15 */
	 u_int16_t ErrorDetectionCounterLane[12];
/*---------------- DWORD[11] (Offset 0x2c) ----------------*/
	/* Description -  */
	/* 44.0 - 92.31 */
	 u_int32_t FECCorrectableBlockCountrLane[12];
/*---------------- DWORD[23] (Offset 0x5c) ----------------*/
	/* Description -  */
	/* 92.0 - 140.31 */
	 u_int32_t FECUncorrectableBlockCounterLane[12];
};

/* Description -   */
/* Size in bytes - 104 */
struct PM_PortSamplesControl {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.2 */
	 u_int8_t CounterWidth;
	/* Description -  */
	/* 0.8 - 0.15 */
	 u_int8_t Tick;
	/* Description -  */
	/* 0.16 - 0.23 */
	 u_int8_t PortSelect;
	/* Description -  */
	/* 0.24 - 4.31 */
	 u_int8_t OpCode;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 4.26 */
	 u_int32_t CounterMasks1to9;
	/* Description -  */
	/* 4.27 - 4.29 */
	 u_int8_t CounterMask0;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description -  */
	/* 8.0 - 8.1 */
	 u_int8_t SampleStatus;
	/* Description -  */
	/* 8.8 - 8.15 */
	 u_int8_t SampleMechanisms;
	/* Description -  */
	/* 8.16 - 8.30 */
	 u_int16_t CounterMasks10to14;
/*---------------- DWORD[3] (Offset 0xc) ----------------*/
	/* Description -  */
	/* 12.0 - 20.31 */
	 struct PortSampleControlOptionMask PortSampleControlOptionMask;
/*---------------- DWORD[5] (Offset 0x14) ----------------*/
	/* Description -  */
	/* 20.0 - 28.31 */
	 u_int64_t VendorMask;
/*---------------- DWORD[7] (Offset 0x1c) ----------------*/
	/* Description -  */
	/* 28.0 - 32.31 */
	 u_int32_t SampleStart;
/*---------------- DWORD[8] (Offset 0x20) ----------------*/
	/* Description -  */
	/* 32.0 - 36.31 */
	 u_int32_t SampleInterval;
/*---------------- DWORD[9] (Offset 0x24) ----------------*/
	/* Description -  */
	/* 36.0 - 64.15 */
	 u_int16_t CounterSelect[15];
	/* Description -  */
	/* 36.16 - 40.31 */
	 u_int16_t Tag;
};

/* Description -  0x40 */
/* Size in bytes - 192 */
struct PM_PortSamplesResult {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Read-only copy of PortSamplesControl:SampleStatus. */
	/* 0.0 - 0.1 */
	 u_int8_t SampleStatus;
	/* Description - Read-only copy of PortSamplesControl:Tag. */
	/* 0.16 - 4.31 */
	 u_int16_t Tag;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 8.31 */
	 u_int32_t Counter0;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description -  */
	/* 8.0 - 12.31 */
	 u_int32_t Counter1;
/*---------------- DWORD[3] (Offset 0xc) ----------------*/
	/* Description -  */
	/* 12.0 - 16.31 */
	 u_int32_t Counter2;
/*---------------- DWORD[4] (Offset 0x10) ----------------*/
	/* Description -  */
	/* 16.0 - 20.31 */
	 u_int32_t Counter3;
/*---------------- DWORD[5] (Offset 0x14) ----------------*/
	/* Description -  */
	/* 20.0 - 24.31 */
	 u_int32_t Counter4;
/*---------------- DWORD[6] (Offset 0x18) ----------------*/
	/* Description -  */
	/* 24.0 - 28.31 */
	 u_int32_t Counter5;
/*---------------- DWORD[7] (Offset 0x1c) ----------------*/
	/* Description -  */
	/* 28.0 - 32.31 */
	 u_int32_t Counter6;
/*---------------- DWORD[8] (Offset 0x20) ----------------*/
	/* Description -  */
	/* 32.0 - 36.31 */
	 u_int32_t Counter7;
/*---------------- DWORD[9] (Offset 0x24) ----------------*/
	/* Description -  */
	/* 36.0 - 40.31 */
	 u_int32_t Counter8;
/*---------------- DWORD[10] (Offset 0x28) ----------------*/
	/* Description -  */
	/* 40.0 - 44.31 */
	 u_int32_t Counter9;
/*---------------- DWORD[11] (Offset 0x2c) ----------------*/
	/* Description -  */
	/* 44.0 - 48.31 */
	 u_int32_t Counter10;
/*---------------- DWORD[12] (Offset 0x30) ----------------*/
	/* Description -  */
	/* 48.0 - 52.31 */
	 u_int32_t Counter11;
/*---------------- DWORD[13] (Offset 0x34) ----------------*/
	/* Description -  */
	/* 52.0 - 56.31 */
	 u_int32_t Counter12;
/*---------------- DWORD[14] (Offset 0x38) ----------------*/
	/* Description -  */
	/* 56.0 - 60.31 */
	 u_int32_t Counter13;
/*---------------- DWORD[15] (Offset 0x3c) ----------------*/
	/* Description -  */
	/* 60.0 - 64.31 */
	 u_int32_t Counter14;
};

/* Description -   */
/* Size in bytes - 64 */
struct PM_PortRcvErrorDetails {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.0 */
	 u_int8_t PortLocalPhysicalErrors_Sel;
	/* Description -  */
	/* 0.1 - 0.1 */
	 u_int8_t PortMalformedPacketErrors_Sel;
	/* Description -  */
	/* 0.2 - 0.2 */
	 u_int8_t PortBufferOverrunErrors_Sel;
	/* Description -  */
	/* 0.3 - 0.3 */
	 u_int8_t PortDLIDMappingErrors_Sel;
	/* Description -  */
	/* 0.4 - 0.4 */
	 u_int8_t PortVLMappingErrors_Sel;
	/* Description -  */
	/* 0.5 - 0.5 */
	 u_int8_t PortLoopingErrors_Sel;
	/* Description -  */
	/* 0.6 - 0.15 */
	 u_int16_t Reserved2;
	/* Description -  */
	/* 0.16 - 0.23 */
	 u_int8_t PortSelect;
	/* Description - Reserved */
	/* 0.24 - 4.31 */
	 u_int8_t Reserved;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 4.15 */
	 u_int16_t PortMalformedPacketErrors;
	/* Description -  */
	/* 4.16 - 8.31 */
	 u_int16_t PortLocalPhysicalErrors;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description -  */
	/* 8.0 - 8.15 */
	 u_int16_t PortDLIDMappingErrors;
	/* Description -  */
	/* 8.16 - 12.31 */
	 u_int16_t PortBufferOverrunErrors;
/*---------------- DWORD[3] (Offset 0xc) ----------------*/
	/* Description -  */
	/* 12.0 - 12.15 */
	 u_int16_t PortLoopingErrors;
	/* Description -  */
	/* 12.16 - 16.31 */
	 u_int16_t PortVLMappingErrors;
};

/* Description -   */
/* Size in bytes - 64 */
struct PM_PortXmitDiscardDetails {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.0 */
	 u_int8_t PortInactiveDiscards_Sel;
	/* Description -  */
	/* 0.1 - 0.1 */
	 u_int8_t PortNeighborMTUDiscards_Sel;
	/* Description -  */
	/* 0.2 - 0.2 */
	 u_int8_t PortSwLifetimeLimitDiscards_Sel;
	/* Description -  */
	/* 0.3 - 0.3 */
	 u_int8_t PortSwHOQLifetimeLimitDiscards_Sel;
	/* Description -  */
	/* 0.4 - 0.15 */
	 u_int16_t Reserved2;
	/* Description -  */
	/* 0.16 - 0.23 */
	 u_int8_t PortSelect;
	/* Description - Reserved */
	/* 0.24 - 4.31 */
	 u_int8_t Reserved;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 4.15 */
	 u_int16_t PortNeighborMTUDiscards;
	/* Description -  */
	/* 4.16 - 8.31 */
	 u_int16_t PortInactiveDiscards;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description -  */
	/* 8.0 - 8.15 */
	 u_int16_t PortSwHOQLifetimeLimitDiscards;
	/* Description -  */
	/* 8.16 - 12.31 */
	 u_int16_t PortSwLifetimeLimitDiscards;
};

/* Description -   */
/* Size in bytes - 72 */
struct PM_PortCountersExtended {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.15 */
	 struct PortCountersExtended_Mask_Block_Element CounterSelect;
	/* Description -  */
	/* 0.16 - 0.23 */
	 u_int8_t PortSelect;
	/* Description -  */
	/* 0.24 - 4.31 */
	 u_int8_t Reserved0;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 8.31 */
	 u_int32_t Reserved1;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description - Optional; shall be zero if not implemented. Total number of data octets, divided by 4, transmitted on all VLs from the port. This includes all octets between (and not including) the start of packet delimiter and the VCRC, and may include packets containing errors. */
	/* 8.0 - 16.31 */
	 u_int64_t PortXmitData;
/*---------------- DWORD[4] (Offset 0x10) ----------------*/
	/* Description - Optional; shall be zero if not implemented. Total number of data octets, divided by 4, received on all VLs at the port. This includes all octets between (and not including) the start of packet delimiter and the VCRC, and may include packets containing errors. */
	/* 16.0 - 24.31 */
	 u_int64_t PortRcvData;
/*---------------- DWORD[6] (Offset 0x18) ----------------*/
	/* Description - Optional; shall be zero if not implemented. Total number of packets transmitted on all VLs from the port. */
	/* 24.0 - 32.31 */
	 u_int64_t PortXmitPkts;
/*---------------- DWORD[8] (Offset 0x20) ----------------*/
	/* Description - Optional; shall be zero if not implemented. Total number of packets, including packets containing errors */
	/* 32.0 - 40.31 */
	 u_int64_t PortRcvPkts;
/*---------------- DWORD[10] (Offset 0x28) ----------------*/
	/* Description - Optional; shall be zero if not implemented. Total number of packets, including packets containing errors */
	/* 40.0 - 48.31 */
	 u_int64_t PortUniCastXmitPkts;
/*---------------- DWORD[12] (Offset 0x30) ----------------*/
	/* Description - Optional; shall be zero if not implemented. Total number of packets, including packets containing errors */
	/* 48.0 - 56.31 */
	 u_int64_t PortUniCastRcvPkts;
/*---------------- DWORD[14] (Offset 0x38) ----------------*/
	/* Description - Optional; shall be zero if not implemented. Total number of packets, including packets containing errors */
	/* 56.0 - 64.31 */
	 u_int64_t PortMultiCastXmitPkts;
/*---------------- DWORD[16] (Offset 0x40) ----------------*/
	/* Description - Optional; shall be zero if not implemented. Total number of packets, including packets containing errors */
	/* 64.0 - 72.31 */
	 u_int64_t PortMultiCastRcvPkts;
};

/* Description -   */
/* Size in bytes - 44 */
struct PM_PortCounters {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.15 */
	 struct PortCounters_Mask_Block_Element CounterSelect;
	/* Description -  */
	/* 0.16 - 0.23 */
	 u_int8_t PortSelect;
	/* Description -  */
	/* 0.24 - 4.31 */
	 u_int8_t Reserved0;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 4.7 */
	 u_int8_t LinkDownedCounter;
	/* Description -  */
	/* 4.8 - 4.15 */
	 u_int8_t LinkErrorRecoveryCounter;
	/* Description -  */
	/* 4.16 - 8.31 */
	 u_int16_t SymbolErrorCounter;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description -  */
	/* 8.0 - 8.15 */
	 u_int16_t PortRcvRemotePhysicalErrors;
	/* Description -  */
	/* 8.16 - 12.31 */
	 u_int16_t PortRcvErrors;
/*---------------- DWORD[3] (Offset 0xc) ----------------*/
	/* Description -  */
	/* 12.0 - 12.15 */
	 u_int16_t PortXmitDiscards;
	/* Description -  */
	/* 12.16 - 16.31 */
	 u_int16_t PortRcvSwitchRelayErrors;
/*---------------- DWORD[4] (Offset 0x10) ----------------*/
	/* Description -  */
	/* 16.0 - 16.3 */
	 u_int8_t ExcessiveBufferOverrunErrors;
	/* Description -  */
	/* 16.4 - 16.7 */
	 u_int8_t LocalLinkIntegrityErrors;
	/* Description -  */
	/* 16.8 - 16.15 */
	 struct PortCounters_Mask2_Block_Element CounterSelect2;
	/* Description -  */
	/* 16.16 - 16.23 */
	 u_int8_t PortRcvConstraintErrors;
	/* Description -  */
	/* 16.24 - 20.31 */
	 u_int8_t PortXmitConstraintErrors;
/*---------------- DWORD[5] (Offset 0x14) ----------------*/
	/* Description -  */
	/* 20.0 - 20.15 */
	 u_int16_t VL15Dropped;
	/* Description -  */
	/* 20.16 - 24.31 */
	 u_int16_t Reserved2;
/*---------------- DWORD[6] (Offset 0x18) ----------------*/
	/* Description -  */
	/* 24.0 - 28.31 */
	 u_int32_t PortXmitData;
/*---------------- DWORD[7] (Offset 0x1c) ----------------*/
	/* Description -  */
	/* 28.0 - 32.31 */
	 u_int32_t PortRcvData;
/*---------------- DWORD[8] (Offset 0x20) ----------------*/
	/* Description -  */
	/* 32.0 - 36.31 */
	 u_int32_t PortXmitPkts;
/*---------------- DWORD[9] (Offset 0x24) ----------------*/
	/* Description -  */
	/* 36.0 - 40.31 */
	 u_int32_t PortRcvPkts;
/*---------------- DWORD[10] (Offset 0x28) ----------------*/
	/* Description -  */
	/* 40.0 - 44.31 */
	 u_int32_t PortXmitWait;
};

/* Description -   */
/* Size in bytes - 64 */
struct SMP_MlnxExtPortInfo {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.7 */
	 u_int8_t StateChangeEnable;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 4.7 */
	 u_int8_t LinkSpeedSupported;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description -  */
	/* 8.0 - 8.7 */
	 u_int8_t LinkSpeedEnabled;
/*---------------- DWORD[3] (Offset 0xc) ----------------*/
	/* Description -  */
	/* 12.0 - 12.7 */
	 u_int8_t LinkSpeedActive;
};

/* Description -   */
/* Size in bytes - 4 */
struct SMP_LedInfo {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.0 */
	 u_int8_t LedMask;
	/* Description -  */
	/* 0.1 - 4.31 */
	 u_int32_t Reserved;
};

/* Description -   */
/* Size in bytes - 24 */
struct SMP_SMInfo {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 8.31 */
	 u_int64_t GUID;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description -  */
	/* 8.0 - 16.31 */
	 u_int64_t Sm_Key;
/*---------------- DWORD[4] (Offset 0x10) ----------------*/
	/* Description -  */
	/* 16.0 - 20.31 */
	 u_int32_t ActCount;
/*---------------- DWORD[5] (Offset 0x14) ----------------*/
	/* Description -  */
	/* 20.24 - 20.27 */
	 u_int8_t SmState;
	/* Description -  */
	/* 20.28 - 24.31 */
	 u_int8_t Priority;
};

/* Description -   */
/* Size in bytes - 64 */
struct SMP_MulticastForwardingTable {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.16 - 64.15 */
	 u_int16_t PortMask[32];
};

/* Description -   */
/* Size in bytes - 64 */
struct SMP_RandomForwardingTable {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 64.31 */
	 struct LID_Port_Block_Element LID_Port_Block_Element[16];
};

/* Description -   */
/* Size in bytes - 64 */
struct SMP_LinearForwardingTable {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.24 - 64.23 */
	 u_int8_t Port[64];
};

/* Description -   */
/* Size in bytes - 64 */
struct SMP_VLArbitrationTable {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.16 - 64.15 */
	 struct VL_Weight_Block_Element VLArb[32];
};

/* Description -   */
/* Size in bytes - 8 */
struct SMP_SLToVLMappingTable {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.3 */
	 u_int8_t SL0ToVL;
	/* Description -  */
	/* 0.4 - 0.7 */
	 u_int8_t SL1ToVL;
	/* Description -  */
	/* 0.8 - 0.11 */
	 u_int8_t SL2ToVL;
	/* Description -  */
	/* 0.12 - 0.15 */
	 u_int8_t SL3ToVL;
	/* Description -  */
	/* 0.16 - 0.19 */
	 u_int8_t SL4ToVL;
	/* Description -  */
	/* 0.20 - 0.23 */
	 u_int8_t SL5ToVL;
	/* Description -  */
	/* 0.24 - 0.27 */
	 u_int8_t SL6ToVL;
	/* Description -  */
	/* 0.28 - 4.31 */
	 u_int8_t SL7ToVL;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 4.3 */
	 u_int8_t SL8ToVL;
	/* Description -  */
	/* 4.4 - 4.7 */
	 u_int8_t SL9ToVL;
	/* Description -  */
	/* 4.8 - 4.11 */
	 u_int8_t SL10ToVL;
	/* Description -  */
	/* 4.12 - 4.15 */
	 u_int8_t SL11ToVL;
	/* Description -  */
	/* 4.16 - 4.19 */
	 u_int8_t SL12ToVL;
	/* Description -  */
	/* 4.20 - 4.23 */
	 u_int8_t SL13ToVL;
	/* Description -  */
	/* 4.24 - 4.27 */
	 u_int8_t SL14ToVL;
	/* Description -  */
	/* 4.28 - 8.31 */
	 u_int8_t SL15ToVL;
};

/* Description -   */
/* Size in bytes - 64 */
struct SMP_PKeyTable {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.16 - 64.15 */
	 struct P_Key_Block_Element PKey_Entry[32];
};

/* Description -  0x38 */
/* Size in bytes - 64 */
struct SMP_PortInfo {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 8.31 */
	 u_int64_t MKey;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description -  */
	/* 8.0 - 16.31 */
	 u_int64_t GIDPrfx;
/*---------------- DWORD[4] (Offset 0x10) ----------------*/
	/* Description -  */
	/* 16.0 - 16.15 */
	 u_int16_t MSMLID;
	/* Description - offset  128 */
	/* 16.16 - 20.31 */
	 u_int16_t LID;
/*---------------- DWORD[5] (Offset 0x14) ----------------*/
	/* Description - offset  160 */
	/* 20.0 - 24.31 */
	 u_int32_t CapMsk;
/*---------------- DWORD[6] (Offset 0x18) ----------------*/
	/* Description -  */
	/* 24.0 - 24.15 */
	 u_int16_t M_KeyLeasePeriod;
	/* Description - offset  192 */
	/* 24.16 - 28.31 */
	 u_int16_t DiagCode;
/*---------------- DWORD[7] (Offset 0x1c) ----------------*/
	/* Description -  */
	/* 28.0 - 28.7 */
	 u_int8_t LinkWidthActv;
	/* Description -  */
	/* 28.8 - 28.15 */
	 u_int8_t LinkWidthSup;
	/* Description -  */
	/* 28.16 - 28.23 */
	 u_int8_t LinkWidthEn;
	/* Description - offset  224 */
	/* 28.24 - 32.31 */
	 u_int8_t LocalPortNum;
/*---------------- DWORD[8] (Offset 0x20) ----------------*/
	/* Description -  */
	/* 32.0 - 32.3 */
	 u_int8_t LinkSpeedEn;
	/* Description -  */
	/* 32.4 - 32.7 */
	 u_int8_t LinkSpeedActv;
	/* Description -  */
	/* 32.8 - 32.10 */
	 u_int8_t LMC;
	/* Description -  */
	/* 32.14 - 32.15 */
	 u_int8_t MKeyProtBits;
	/* Description -  */
	/* 32.16 - 32.19 */
	 u_int8_t LinkDownDefState;
	/* Description -  */
	/* 32.20 - 32.23 */
	 u_int8_t PortPhyState;
	/* Description -  */
	/* 32.24 - 32.27 */
	 u_int8_t PortState;
	/* Description - offset 256 */
	/* 32.28 - 36.31 */
	 u_int8_t LinkSpeedSup;
/*---------------- DWORD[9] (Offset 0x24) ----------------*/
	/* Description -  */
	/* 36.0 - 36.7 */
	 u_int8_t VLArbHighCap;
	/* Description -  */
	/* 36.8 - 36.15 */
	 u_int8_t VLHighLimit;
	/* Description -  */
	/* 36.16 - 36.19 */
	 u_int8_t InitType;
	/* Description -  */
	/* 36.20 - 36.23 */
	 u_int8_t VLCap;
	/* Description -  */
	/* 36.24 - 36.27 */
	 u_int8_t MSMSL;
	/* Description - offset 288 */
	/* 36.28 - 40.31 */
	 u_int8_t NMTU;
/*---------------- DWORD[10] (Offset 0x28) ----------------*/
	/* Description -  */
	/* 40.0 - 40.0 */
	 u_int8_t FilterRawOutb;
	/* Description -  */
	/* 40.1 - 40.1 */
	 u_int8_t FilterRawInb;
	/* Description -  */
	/* 40.2 - 40.2 */
	 u_int8_t PartEnfOutb;
	/* Description -  */
	/* 40.3 - 40.3 */
	 u_int8_t PartEnfInb;
	/* Description -  */
	/* 40.4 - 40.7 */
	 u_int8_t OpVLs;
	/* Description -  */
	/* 40.8 - 40.12 */
	 u_int8_t HoQLife;
	/* Description -  */
	/* 40.13 - 40.15 */
	 u_int8_t VLStallCnt;
	/* Description -  */
	/* 40.16 - 40.19 */
	 u_int8_t MTUCap;
	/* Description -  */
	/* 40.20 - 40.23 */
	 u_int8_t InitTypeReply;
	/* Description - offset 320 */
	/* 40.24 - 44.31 */
	 u_int8_t VLArbLowCap;
/*---------------- DWORD[11] (Offset 0x2c) ----------------*/
	/* Description -  */
	/* 44.0 - 44.15 */
	 u_int16_t PKeyViolations;
	/* Description -  */
	/* 44.16 - 48.31 */
	 u_int16_t MKeyViolations;
/*---------------- DWORD[12] (Offset 0x30) ----------------*/
	/* Description -  */
	/* 48.0 - 48.4 */
	 u_int8_t SubnTmo;
	/* Description -  */
	/* 48.7 - 48.7 */
	 u_int8_t ClientReregister;
	/* Description -  */
	/* 48.8 - 48.15 */
	 u_int8_t GUIDCap;
	/* Description -  */
	/* 48.16 - 52.31 */
	 u_int16_t QKeyViolations;
/*---------------- DWORD[13] (Offset 0x34) ----------------*/
	/* Description -  */
	/* 52.0 - 52.15 */
	 u_int16_t MaxCreditHint;
	/* Description -  */
	/* 52.16 - 52.19 */
	 u_int8_t OverrunErrs;
	/* Description -  */
	/* 52.20 - 52.23 */
	 u_int8_t LocalPhyError;
	/* Description -  */
	/* 52.24 - 52.28 */
	 u_int8_t RespTimeValue;
/*---------------- DWORD[14] (Offset 0x38) ----------------*/
	/* Description -  */
	/* 56.0 - 56.23 */
	 u_int32_t LinkRoundTripLatency;
/*---------------- DWORD[15] (Offset 0x3c) ----------------*/
	/* Description - offset 507 */
	/* 60.0 - 60.4 */
	 u_int8_t LinkSpeedExtEn;
	/* Description - offset 500 */
	/* 60.8 - 60.11 */
	 u_int8_t LinkSpeedExtSup;
	/* Description - offset 496 */
	/* 60.12 - 60.15 */
	 u_int8_t LinkSpeedExtActv;
};

/* Description -   */
/* Size in bytes - 64 */
struct SMP_GUIDInfo {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 64.31 */
	 struct GUID_Block_Element GUIDBlock;
};

/* Description -   */
/* Size in bytes - 64 */
struct SMP_SwitchInfo {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.15 */
	 u_int16_t RandomFDBCap;
	/* Description -  */
	/* 0.16 - 4.31 */
	 u_int16_t LinearFDBCap;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 4.15 */
	 u_int16_t LinearFDBTop;
	/* Description -  */
	/* 4.16 - 8.31 */
	 u_int16_t MCastFDBCap;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description - bit[0] when set indicate switch support optimized SLtoVL Mapping\;bit[1] - reserve */
	/* 8.0 - 8.1 */
	 u_int8_t OptimizedSLVLMapping;
	/* Description -  */
	/* 8.2 - 8.2 */
	 u_int8_t PortStateChange;
	/* Description -  */
	/* 8.3 - 8.7 */
	 u_int8_t LifeTimeValue;
	/* Description -  */
	/* 8.8 - 8.15 */
	 u_int8_t DefMCastNotPriPort;
	/* Description -  */
	/* 8.16 - 8.23 */
	 u_int8_t DefMCastPriPort;
	/* Description -  */
	/* 8.24 - 12.31 */
	 u_int8_t DefPort;
/*---------------- DWORD[3] (Offset 0xc) ----------------*/
	/* Description -  */
	/* 12.0 - 12.15 */
	 u_int16_t PartEnfCap;
	/* Description -  */
	/* 12.16 - 16.31 */
	 u_int16_t LidsPerPort;
/*---------------- DWORD[4] (Offset 0x10) ----------------*/
	/* Description -  */
	/* 16.0 - 16.15 */
	 u_int16_t MCastFDBTop;
	/* Description -  */
	/* 16.27 - 16.27 */
	 u_int8_t ENP0;
	/* Description -  */
	/* 16.28 - 16.28 */
	 u_int8_t FilterRawOutbCap;
	/* Description -  */
	/* 16.29 - 16.29 */
	 u_int8_t FilterRawInbCap;
	/* Description -  */
	/* 16.30 - 16.30 */
	 u_int8_t OutbEnfCap;
	/* Description -  */
	/* 16.31 - 20.31 */
	 u_int8_t InbEnfCap;
};

/* Description -  0x28 */
/* Size in bytes - 40 */
struct SMP_NodeInfo {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Number of physical ports on this node. */
	/* 0.0 - 0.7 */
	 u_int8_t NumPorts;
	/* Description - 1: Channel Adapter 2: Switch 3: Router 0, 4 - 255: Reserved\; */
	/* 0.8 - 0.15 */
	 u_int8_t NodeType;
	/* Description -  */
	/* 0.16 - 0.23 */
	 u_int8_t ClassVersion;
	/* Description -  */
	/* 0.24 - 4.31 */
	 u_int8_t BaseVersion;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description -  */
	/* 4.0 - 12.31 */
	 u_int64_t SystemImageGUID;
/*---------------- DWORD[3] (Offset 0xc) ----------------*/
	/* Description -  */
	/* 12.0 - 20.31 */
	 u_int64_t NodeGUID;
/*---------------- DWORD[5] (Offset 0x14) ----------------*/
	/* Description -  */
	/* 20.0 - 28.31 */
	 u_int64_t PortGUID;
/*---------------- DWORD[7] (Offset 0x1c) ----------------*/
	/* Description - Device ID information as assigned by device manufacturer. */
	/* 28.0 - 28.15 */
	 u_int16_t DeviceID;
	/* Description - Number of entries in the Partition Table for CA, router, and the switch management port. This is at a minimum set to 1 for all nodes including switches. */
	/* 28.16 - 32.31 */
	 u_int16_t PartitionCap;
/*---------------- DWORD[8] (Offset 0x20) ----------------*/
	/* Description - Device revision, assigned by manufacturer. */
	/* 32.0 - 36.31 */
	 u_int32_t revision;
/*---------------- DWORD[9] (Offset 0x24) ----------------*/
	/* Description -  */
	/* 36.0 - 36.23 */
	 u_int32_t VendorID;
	/* Description - Number of the link port which received this SMP. */
	/* 36.24 - 40.31 */
	 u_int8_t LocalPortNum;
};

/* Description -   */
/* Size in bytes - 64 */
struct SMP_NodeDesc {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - UTF-8 encoded string to describe node in text format */
	/* 0.24 - 64.23 */
	 char Byte[65];
};

/* Description -   */
/* Size in bytes - 192 */
struct CC_Mgt_Data_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 192.31 */
	 u_int32_t DWORD[48];
};

/* Description -   */
/* Size in bytes - 32 */
struct CC_Log_Data_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 32.31 */
	 u_int32_t DWORD[8];
};

/* Description -  MAD Header Common */
/* Size in bytes - 24 */
struct MAD_Header_Common {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Method to perform Based on Management Class */
	/* 0.0 - 0.7 */
	 u_int8_t Method;
	/* Description - Version of MAD class-specific format */
	/* 0.8 - 0.15 */
	 u_int8_t ClassVersion;
	/* Description - Class of operation */
	/* 0.16 - 0.23 */
	 u_int8_t MgmtClass;
	/* Description - Version of MAD base format */
	/* 0.24 - 4.31 */
	 u_int8_t BaseVersion;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - This field is reserved except for the Subnet Management Class */
	/* 4.0 - 4.15 */
	 u_int16_t ClassSpecific;
	/* Description - Code indicating status of operation */
	/* 4.16 - 8.31 */
	 u_int16_t Status;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description -  */
	/* 8.0 - 16.31 */
	 u_int64_t TID_Block_Element;
/*---------------- DWORD[4] (Offset 0x10) ----------------*/
	/* Description -  */
	/* 16.0 - 16.15 */
	 u_int16_t Rsv16;
	/* Description - Defines objects being operated by a management class (Page 658) */
	/* 16.16 - 20.31 */
	 u_int16_t AttributeID;
/*---------------- DWORD[5] (Offset 0x14) ----------------*/
	/* Description - Provides further scope to the attributes */
	/* 20.0 - 24.31 */
	 u_int32_t AttributeModifier;
};

/* Description -   */
/* Size in bytes - 224 */
struct VendorSpecific_MAD_Data_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 224.31 */
	 u_int32_t DWORD[56];
};

/* Description -   */
/* Size in bytes - 200 */
struct SubnetAdministartion_MAD_Data_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 200.31 */
	 u_int32_t DWORD[50];
};

/* Description -  MAD Header Common With RMPP */
/* Size in bytes - 36 */
struct MAD_Header_Common_With_RMPP {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Method to perform Based on Management Class */
	/* 0.0 - 0.7 */
	 u_int8_t Method;
	/* Description - Version of MAD class-specific format */
	/* 0.8 - 0.15 */
	 u_int8_t ClassVersion;
	/* Description - Class of operation */
	/* 0.16 - 0.23 */
	 u_int8_t MgmtClass;
	/* Description - Version of MAD base format */
	/* 0.24 - 4.31 */
	 u_int8_t BaseVersion;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - This field is reserved except for the Subnet Management Class */
	/* 4.0 - 4.15 */
	 u_int16_t ClassSpecific;
	/* Description - Code indicating status of operation */
	/* 4.16 - 8.31 */
	 u_int16_t Status;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description -  */
	/* 8.0 - 16.31 */
	 u_int64_t TID_Block_Element;
/*---------------- DWORD[4] (Offset 0x10) ----------------*/
	/* Description -  */
	/* 16.0 - 16.15 */
	 u_int16_t Rsv16;
	/* Description - Defines objects being operated by a management class (Page 658) */
	/* 16.16 - 20.31 */
	 u_int16_t AttributeID;
/*---------------- DWORD[5] (Offset 0x14) ----------------*/
	/* Description - Provides further scope to the attributes */
	/* 20.0 - 24.31 */
	 u_int32_t AttributeModifier;
/*---------------- DWORD[6] (Offset 0x18) ----------------*/
	/* Description -  */
	/* 24.0 - 24.7 */
	 u_int8_t RMPPStatus;
	/* Description -  */
	/* 24.8 - 24.11 */
	 u_int8_t RMPPFlags;
	/* Description -  */
	/* 24.12 - 24.15 */
	 u_int8_t RRespTime;
	/* Description - Indicates the type of RMPP packet being transferred. */
	/* 24.16 - 24.23 */
	 u_int8_t RMPPType;
	/* Description - Version of RMPP. Shall be set to the version of RMPP implemented. */
	/* 24.24 - 28.31 */
	 u_int8_t RMPPVersion;
/*---------------- DWORD[7] (Offset 0x1c) ----------------*/
	/* Description -  */
	/* 28.0 - 32.31 */
	 u_int32_t Data1;
/*---------------- DWORD[8] (Offset 0x20) ----------------*/
	/* Description -  */
	/* 32.0 - 36.31 */
	 u_int32_t Data2;
};

/* Description -   */
/* Size in bytes - 192 */
struct PerfManagement_MAD_Data_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 192.31 */
	 u_int32_t DWORD[48];
};

/* Description -   */
/* Size in bytes - 64 */
struct SMP_MAD_Data_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 64.31 */
	 u_int32_t DWORD[16];
};

/* Description -   */
/* Size in bytes - 64 */
struct DirRPath_Block_Element {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.24 - 64.23 */
	 u_int8_t BYTE[64];
};

/* Description -  MAD Header for SMP Direct Routed  */
/* Size in bytes - 24 */
struct MAD_Header_SMP_Direct_Routed {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Method to perform Based on Management Class */
	/* 0.0 - 0.7 */
	 u_int8_t Method;
	/* Description - Version of MAD class-specific format */
	/* 0.8 - 0.15 */
	 u_int8_t ClassVersion;
	/* Description - Class of operation */
	/* 0.16 - 0.23 */
	 u_int8_t MgmtClass;
	/* Description - Version of MAD base format */
	/* 0.24 - 4.31 */
	 u_int8_t BaseVersion;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - Used in The direct route */
	/* 4.0 - 4.7 */
	 u_int8_t HopCounter;
	/* Description - Used in The direct route */
	/* 4.8 - 4.15 */
	 u_int8_t HopPointer;
	/* Description - Code indicating status of operation */
	/* 4.16 - 4.30 */
	 u_int16_t Status;
	/* Description - direction of direct route packet */
	/* 4.31 - 8.31 */
	 u_int8_t D;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description -  */
	/* 8.0 - 16.31 */
	 u_int64_t TID_Block_Element;
/*---------------- DWORD[4] (Offset 0x10) ----------------*/
	/* Description -  */
	/* 16.0 - 16.15 */
	 u_int16_t Rsv16;
	/* Description - Defines objects being operated by a management class (Page 658) */
	/* 16.16 - 20.31 */
	 u_int16_t AttributeID;
/*---------------- DWORD[5] (Offset 0x14) ----------------*/
	/* Description - Provides further scope to the attributes */
	/* 20.0 - 24.31 */
	 u_int32_t AttributeModifier;
};

/* Description -   */
/* Size in bytes - 131072 */
union VENDOR_SPECS {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 128.31 */
	 struct VendorSpec_GeneralInfo VendorSpec_GeneralInfo;
};

/* Description -   */
/* Size in bytes - 131072 */
union CONGESTION_CONTOL {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 4.31 */
	 struct CC_CongestionInfo CC_CongestionInfo;
	/* Description -  */
	/* 0.0 - 16.31 */
	 struct CC_CongestionKeyInfo CC_CongestionKeyInfo;
	/* Description -  */
	/* 0.0 - 220.31 */
	 struct CC_CongestionLogSwitch CC_CongestionLogSwitch;
	/* Description -  */
	/* 0.0 - 220.31 */
	 struct CC_CongestionLogCA CC_CongestionLogCA;
	/* Description -  */
	/* 0.0 - 76.31 */
	 struct CC_SwitchCongestionSetting CC_SwitchCongestionSetting;
	/* Description -  */
	/* 0.0 - 128.31 */
	 struct CC_SwitchPortCongestionSetting CC_SwitchPortCongestionSetting;
	/* Description -  */
	/* 0.0 - 132.31 */
	 struct CC_CACongestionSetting CC_CACongestionSetting;
	/* Description -  */
	/* 0.0 - 132.31 */
	 struct CC_CongestionControlTable CC_CongestionControlTable;
	/* Description -  */
	/* 0.0 - 4.31 */
	 struct CC_TimeStamp CC_TimeStamp;
	/* Description -  */
	/* 0.0 - 80.31 */
	 struct CC_Notice CC_Notice;
};

/* Description -   */
/* Size in bytes - 131072 */
union PERFORMANCE_MADS {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 44.31 */
	 struct PM_PortCounters PM_PortCounters;
	/* Description -  */
	/* 0.0 - 72.31 */
	 struct PM_PortCountersExtended PM_PortCountersExtended;
	/* Description -  */
	/* 0.0 - 64.31 */
	 struct PM_PortXmitDiscardDetails PM_PortXmitDiscardDetails;
	/* Description -  */
	/* 0.0 - 64.31 */
	 struct PM_PortRcvErrorDetails PM_PortRcvErrorDetails;
	/* Description -  */
	/* 0.0 - 192.31 */
	 struct PM_PortSamplesResult PM_PortSamplesResult;
	/* Description -  */
	/* 0.0 - 104.31 */
	 struct PM_PortSamplesControl PM_PortSamplesControl;
	/* Description -  */
	/* 0.0 - 128.31 */
	 struct PM_PortExtendedSpeedsCounters PM_PortExtendedSpeedsCounters;
};

/* Description -   */
/* Size in bytes - 131072 */
union SMP_MADS {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 64.31 */
	 struct SMP_NodeDesc SMP_NodeDesc;
	/* Description -  */
	/* 0.0 - 40.31 */
	 struct SMP_NodeInfo SMP_NodeInfo;
	/* Description -  */
	/* 0.0 - 64.31 */
	 struct SMP_SwitchInfo SMP_SwitchInfo;
	/* Description -  */
	/* 0.0 - 64.31 */
	 struct SMP_GUIDInfo SMP_GUIDInfo;
	/* Description -  */
	/* 0.0 - 64.31 */
	 struct SMP_PortInfo SMP_PortInfo;
	/* Description -  */
	/* 0.0 - 64.31 */
	 struct SMP_PKeyTable SMP_PKeyTable;
	/* Description -  */
	/* 0.0 - 8.31 */
	 struct SMP_SLToVLMappingTable SMP_SLToVLMappingTable;
	/* Description -  */
	/* 0.0 - 64.31 */
	 struct SMP_VLArbitrationTable SMP_VLArbitrationTable;
	/* Description -  */
	/* 0.0 - 64.31 */
	 struct SMP_LinearForwardingTable SMP_LinearForwardingTable;
	/* Description -  */
	/* 0.0 - 64.31 */
	 struct SMP_RandomForwardingTable SMP_RandomForwardingTable;
	/* Description -  */
	/* 0.0 - 64.31 */
	 struct SMP_MulticastForwardingTable SMP_MulticastForwardingTable;
	/* Description -  */
	/* 0.0 - 24.31 */
	 struct SMP_SMInfo SMP_SMInfo;
	/* Description -  */
	/* 0.0 - 4.31 */
	 struct SMP_LedInfo SMP_LedInfo;
	/* Description -  */
	/* 0.0 - 64.31 */
	 struct SMP_MlnxExtPortInfo SMP_MlnxExtPortInfo;
};

/* Description -   */
/* Size in bytes - 72 */
struct IB_ClassPortInfo {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Supported capabilities of this management class, \;Bit set to 1 for affirmation of management support. \;Bit 0 - If 1, the management class generates Trap() MADs Bit 1 - If 1, the management class implements Get(Notice) and Set(Notice) \;Bit 2-7: reserved \;Bit 8: Switch only - set if the EnhancedPort0 supports CA Congestion Control. (Note a switch can support Congestion control on data ports without supporting it on EnhancedPort0) \;Bit 9-15: class-specific capabilities. */
	/* 0.0 - 0.15 */
	 u_int16_t CapMsk;
	/* Description - Current supported management class version. \;Indicates that this channel adapter, switch, or router supports up to and including this version. */
	/* 0.16 - 0.23 */
	 u_int8_t ClassVersion;
	/* Description - Current supported MAD Base Version. \;Indicates that this channel adapter, switch, or router supports up to and including this version. */
	/* 0.24 - 4.31 */
	 u_int8_t BaseVersion;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - See 13.4.6.2 Timers and Timeouts . */
	/* 4.0 - 4.4 */
	 u_int8_t RespTimeValue;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description - The GID a requester shall use as the destination GID in the GRH of messages used to access redirected class services. */
	/* 8.0 - 24.31 */
	 struct uint64bit RedirectGID[2];
/*---------------- DWORD[6] (Offset 0x18) ----------------*/
	/* Description - The Flow Label a requester shall use in the GRH of messages used to access redirected class services. */
	/* 24.0 - 24.19 */
	 u_int32_t RedirectFL;
	/* Description - The SL a requester shall use to access the class services. */
	/* 24.20 - 24.23 */
	 u_int8_t RedirectSL;
	/* Description - The Traffic Class a requester shall use in the GRH of messages used to access redirected class services. */
	/* 24.24 - 28.31 */
	 u_int8_t RedirectTC;
/*---------------- DWORD[7] (Offset 0x1c) ----------------*/
	/* Description - The P_Key a requester shall use to access the class services. */
	/* 28.0 - 28.15 */
	 u_int16_t RedirectPKey;
	/* Description - If this value is non-zero, it is the DLID a requester shall use to access the class services. */
	/* 28.16 - 32.31 */
	 u_int16_t RedirectLID;
/*---------------- DWORD[8] (Offset 0x20) ----------------*/
	/* Description - The QP a requester shall use to access the class services. Zero is illegal. */
	/* 32.0 - 32.23 */
	 u_int32_t RedirectQP;
/*---------------- DWORD[9] (Offset 0x24) ----------------*/
	/* Description - The Q_Key associated with the RedirectQP. This Q_Key shall be set to the well known Q_Key. */
	/* 36.0 - 40.31 */
	 u_int32_t RedirectQKey;
/*---------------- DWORD[10] (Offset 0x28) ----------------*/
	/* Description - The GID to be used as the destination GID in the GRH of Trap() messages originated by this service. */
	/* 40.0 - 56.31 */
	 u_int32_t TrapGID[4];
/*---------------- DWORD[14] (Offset 0x38) ----------------*/
	/* Description - The Flow Label to be placed in the GRH of Trap() messages originated by this service. */
	/* 56.0 - 56.19 */
	 u_int32_t TrapFL;
	/* Description - The SL that shall be used when sending Trap() messages originated by this service. */
	/* 56.20 - 56.23 */
	 u_int8_t TrapSL;
	/* Description - The Traffic Class to be placed in the GRH of Trap() messages originated by this service. */
	/* 56.24 - 60.31 */
	 u_int8_t TrapTC;
/*---------------- DWORD[15] (Offset 0x3c) ----------------*/
	/* Description - The P_Key to be placed in the header for traps originated by this service. */
	/* 60.0 - 60.15 */
	 u_int16_t TrapPKey;
	/* Description - The DLID to where Trap() messages shall be sent by this service. */
	/* 60.16 - 64.31 */
	 u_int16_t TrapLID;
/*---------------- DWORD[16] (Offset 0x40) ----------------*/
	/* Description - The QP to which Trap() messages originated by this service shall be sent. The value shall not be zero. */
	/* 64.0 - 64.23 */
	 u_int32_t TrapQP;
	/* Description - The Hop Limit to be placed in the GRH of Trap() messages originated by this service. The default value is 255. */
	/* 64.24 - 68.31 */
	 u_int8_t TrapHL;
/*---------------- DWORD[17] (Offset 0x44) ----------------*/
	/* Description - The Q_Key associated with the TrapQP. */
	/* 68.0 - 72.31 */
	 u_int32_t TrapQKey;
};

/* Description -  MAD Performance management Data Format */
/* Size in bytes - 256 */
struct MAD_CongestionControl {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 24.31 */
	 struct MAD_Header_Common MAD_Header_Common;
/*---------------- DWORD[6] (Offset 0x18) ----------------*/
	/* Description - Congestion Control key, is used to validate the source of Congestion Control Mads. */
	/* 24.0 - 32.31 */
	 u_int64_t CC_Key;
/*---------------- DWORD[8] (Offset 0x20) ----------------*/
	/* Description -  */
	/* 32.0 - 64.31 */
	 struct CC_Log_Data_Block_Element CC_LogData;
/*---------------- DWORD[16] (Offset 0x40) ----------------*/
	/* Description -  */
	/* 64.0 - 256.31 */
	 struct CC_Mgt_Data_Block_Element CC_MgtData;
};

/* Description -  MAD Vendor Specific Data Format */
/* Size in bytes - 256 */
struct MAD_VendorSpec {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 24.31 */
	 struct MAD_Header_Common MAD_Header_Common;
/*---------------- DWORD[6] (Offset 0x18) ----------------*/
	/* Description -  */
	/* 24.0 - 32.31 */
	 u_int64_t V_Key;
/*---------------- DWORD[8] (Offset 0x20) ----------------*/
	/* Description -  */
	/* 32.0 - 256.31 */
	 struct VendorSpecific_MAD_Data_Block_Element Data;
};

/* Description -   */
/* Size in bytes - 52 */
struct SA_MCMMemberRecord {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 16.31 */
	 struct GID_Block_Element MGID;
/*---------------- DWORD[4] (Offset 0x10) ----------------*/
	/* Description -  */
	/* 16.0 - 32.31 */
	 struct GID_Block_Element PortGID;
/*---------------- DWORD[8] (Offset 0x20) ----------------*/
	/* Description -  */
	/* 32.0 - 36.31 */
	 u_int32_t Q_key;
/*---------------- DWORD[9] (Offset 0x24) ----------------*/
	/* Description -  */
	/* 36.0 - 36.7 */
	 u_int8_t TClass;
	/* Description -  */
	/* 36.8 - 36.13 */
	 u_int8_t MTU;
	/* Description -  */
	/* 36.14 - 36.15 */
	 u_int8_t MTUSelector;
	/* Description -  */
	/* 36.16 - 40.31 */
	 u_int16_t MLID;
/*---------------- DWORD[10] (Offset 0x28) ----------------*/
	/* Description -  */
	/* 40.0 - 40.5 */
	 u_int8_t PacketLifeTime;
	/* Description -  */
	/* 40.6 - 40.7 */
	 u_int8_t PacketLifeTimeSelector;
	/* Description -  */
	/* 40.8 - 40.13 */
	 u_int8_t Rate;
	/* Description -  */
	/* 40.14 - 40.15 */
	 u_int8_t RateSelector;
	/* Description -  */
	/* 40.16 - 44.31 */
	 u_int16_t P_Key;
/*---------------- DWORD[11] (Offset 0x2c) ----------------*/
	/* Description -  */
	/* 44.0 - 44.7 */
	 u_int8_t HopLimit;
	/* Description -  */
	/* 44.8 - 44.27 */
	 u_int32_t FlowLabel;
	/* Description -  */
	/* 44.28 - 48.31 */
	 u_int8_t SL;
/*---------------- DWORD[12] (Offset 0x30) ----------------*/
	/* Description -  */
	/* 48.0 - 48.22 */
	 u_int32_t Reserved;
	/* Description -  */
	/* 48.23 - 48.23 */
	 u_int8_t ProxyJoin;
	/* Description -  */
	/* 48.24 - 48.27 */
	 u_int8_t JoinState;
	/* Description -  */
	/* 48.28 - 52.31 */
	 u_int8_t Scope;
};

/* Description -  MAD Subnet Administration Data Format */
/* Size in bytes - 256 */
struct MAD_SubnetAdministartion {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 36.31 */
	 struct MAD_Header_Common_With_RMPP MAD_Header_Common_With_RMPP;
/*---------------- DWORD[9] (Offset 0x24) ----------------*/
	/* Description -  */
	/* 36.0 - 44.31 */
	 u_int64_t Sm_Key;
/*---------------- DWORD[11] (Offset 0x2c) ----------------*/
	/* Description -  */
	/* 44.0 - 44.15 */
	 u_int16_t Reserved;
	/* Description -  */
	/* 44.16 - 48.31 */
	 u_int16_t AttributeOffset;
/*---------------- DWORD[12] (Offset 0x30) ----------------*/
	/* Description -  */
	/* 48.0 - 56.31 */
	 u_int64_t ComponentMask;
/*---------------- DWORD[14] (Offset 0x38) ----------------*/
	/* Description -  */
	/* 56.0 - 256.31 */
	 struct SubnetAdministartion_MAD_Data_Block_Element Data;
};

/* Description -  MAD Performance management Data Format */
/* Size in bytes - 256 */
struct MAD_PerformanceManagement {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 24.31 */
	 struct MAD_Header_Common MAD_Header_Common;
/*---------------- DWORD[6] (Offset 0x18) ----------------*/
	/* Description -  */
	/* 24.0 - 64.31 */
	 u_int32_t Reserved_Dword[10];
/*---------------- DWORD[16] (Offset 0x40) ----------------*/
	/* Description -  */
	/* 64.0 - 256.31 */
	 struct PerfManagement_MAD_Data_Block_Element Data;
};

/* Description -  MAD SMP Data Format - Lid Routed */
/* Size in bytes - 256 */
struct MAD_SMP_LID_Routed {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 24.31 */
	 struct MAD_Header_Common MAD_Header_Common;
/*---------------- DWORD[6] (Offset 0x18) ----------------*/
	/* Description -  */
	/* 24.0 - 32.31 */
	 u_int64_t M_Key;
/*---------------- DWORD[8] (Offset 0x20) ----------------*/
	/* Description - Reserved 32 bytes. */
	/* 32.0 - 64.31 */
	 u_int32_t Reserved1[8];
/*---------------- DWORD[16] (Offset 0x40) ----------------*/
	/* Description -  */
	/* 64.0 - 128.31 */
	 struct SMP_MAD_Data_Block_Element Data;
/*---------------- DWORD[32] (Offset 0x80) ----------------*/
	/* Description - Reserved 128 bytes. */
	/* 128.0 - 256.31 */
	 u_int32_t Reserved2[32];
};

/* Description -  MAD SMP Data Format - Direct Routed */
/* Size in bytes - 256 */
struct MAD_SMP_Direct_Routed {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 24.31 */
	 struct MAD_Header_SMP_Direct_Routed MAD_Header_SMP_Direct_Routed;
/*---------------- DWORD[6] (Offset 0x18) ----------------*/
	/* Description -  */
	/* 24.0 - 32.31 */
	 u_int64_t M_Key;
/*---------------- DWORD[8] (Offset 0x20) ----------------*/
	/* Description - Directed route destination LID. Used in directed routing. */
	/* 32.0 - 32.15 */
	 u_int16_t DrDLID;
	/* Description - Directed route source LID. Used in directed routing. */
	/* 32.16 - 36.31 */
	 u_int16_t DrSLID;
/*---------------- DWORD[9] (Offset 0x24) ----------------*/
	/* Description - Reserved 28 bytes */
	/* 36.0 - 64.31 */
	 u_int32_t Reserved[7];
/*---------------- DWORD[16] (Offset 0x40) ----------------*/
	/* Description -  */
	/* 64.0 - 128.31 */
	 struct SMP_MAD_Data_Block_Element Data;
/*---------------- DWORD[32] (Offset 0x80) ----------------*/
	/* Description -  */
	/* 128.0 - 192.31 */
	 struct DirRPath_Block_Element InitPath;
/*---------------- DWORD[48] (Offset 0xc0) ----------------*/
	/* Description -  */
	/* 192.0 - 256.31 */
	 struct DirRPath_Block_Element RetPath;
};

/* Description -  VCRC of IB Packet */
/* Size in bytes - 4 */
struct VCRC {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Variant CRC */
	/* 0.0 - 0.15 */
	 u_int16_t VCRC;
	/* Description - Reserved */
	/* 0.16 - 4.31 */
	 u_int16_t Rsrvd;
};

/* Description -  ICRC of IB Packet */
/* Size in bytes - 4 */
struct ICRC {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Invariant CRC */
	/* 0.0 - 4.31 */
	 u_int32_t ICRC;
};

/* Description -  Invalidate Extended Transport Header */
/* Size in bytes - 4 */
struct IB_IETH {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Remote Key */
	/* 0.0 - 4.31 */
	 u_int32_t R_Key;
};

/* Description -  Immediate Extended Transport Header */
/* Size in bytes - 4 */
struct IB_ImmDt {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Immediate Extended Transport Header */
	/* 0.0 - 4.31 */
	 u_int32_t Immediate_Data;
};

/* Description -  Atomic ACK Extended Transport Header */
/* Size in bytes - 8 */
struct IB_AtomicAckETH {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Return operand in atomic operations and contains the data in the remote memory location before the atomic operationIndicates if this is an ACK or NAK packet plus additional information about the ACK or NAKRemote virtual address */
	/* 0.0 - 8.31 */
	 u_int64_t OrigRemDt;
};

/* Description -  ACK Extended Transport Header */
/* Size in bytes - 4 */
struct IB_AETH {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Indicates the sequence number of the last message completed at the responder. Remote Key that authorizes access to the remote virtual address */
	/* 0.0 - 0.23 */
	 u_int32_t MSN;
	/* Description - Indicates if this is an ACK or NAK packet plus additional information about the ACK or NAKRemote virtual address */
	/* 0.24 - 4.31 */
	 u_int8_t Syndrome;
};

/* Description -  Atomic Extended Transport Header */
/* Size in bytes - 28 */
struct IB_AtomicETH {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Remote virtual address */
	/* 0.0 - 8.31 */
	 u_int64_t VA;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description - Remote Key that authorizes access to the remote virtual address */
	/* 8.0 - 12.31 */
	 u_int32_t R_Key;
/*---------------- DWORD[3] (Offset 0xc) ----------------*/
	/* Description - Operand in atomic operations */
	/* 12.0 - 20.31 */
	 u_int64_t SwapDt;
/*---------------- DWORD[5] (Offset 0x14) ----------------*/
	/* Description - Operand in CmpSwap atomic operation */
	/* 20.0 - 28.31 */
	 u_int64_t CmpDt;
};

/* Description -  RDMA Extended Transport Header Fields */
/* Size in bytes - 16 */
struct IB_RETH {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Virtual Address of the RDMA operation */
	/* 0.0 - 8.31 */
	 u_int64_t VA;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description - Remote Key that authorizes access for the RDMA operation */
	/* 8.0 - 12.31 */
	 u_int32_t R_Key;
/*---------------- DWORD[3] (Offset 0xc) ----------------*/
	/* Description - Indicates the length (in Bytes) of the DMA operation */
	/* 12.0 - 16.31 */
	 u_int32_t DMALen;
};

/* Description -  Datagram Extended Transport Header */
/* Size in bytes - 8 */
struct IB_DETH {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Queue Key */
	/* 0.0 - 4.31 */
	 u_int32_t Q_Key;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - Source QP */
	/* 4.0 - 4.23 */
	 u_int32_t SrcQP;
	/* Description - TX - 0, RX - ignore */
	/* 4.24 - 8.31 */
	 u_int8_t Rsv8;
};

/* Description -  Reliable Datagram Extended Transport Header */
/* Size in bytes - 4 */
struct IB_RDETH {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - This field indicates which End-to-End Context should be used for this Reliable Datagram packet */
	/* 0.0 - 0.23 */
	 u_int32_t EECnxt;
	/* Description - TX - 0, RX - ignore */
	/* 0.24 - 4.31 */
	 u_int8_t Rsv8;
};

/* Description -  Base Transport Header for CNP */
/* Size in bytes - 12 */
struct IB_BTH_CNP {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.15 */
	 struct P_Key_Block_Element P_Key;
	/* Description - Transport Header Version (is set to 0x0) */
	/* 0.16 - 0.19 */
	 u_int8_t TVer;
	/* Description - Pad Count */
	/* 0.20 - 0.21 */
	 u_int8_t PadCnt;
	/* Description - If zero, it means there is no change in current migration state */
	/* 0.22 - 0.22 */
	 u_int8_t MigReq;
	/* Description - Solicited Event */
	/* 0.23 - 0.23 */
	 u_int8_t SE;
	/* Description - operation code (011_00100 - UD_SEND_only) */
	/* 0.24 - 4.31 */
	 u_int8_t OpCode;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - Destination QP */
	/* 4.0 - 4.23 */
	 u_int32_t DestQP;
	/* Description - Reserved(variant) - 6 bits. TX - 0, RX - ignore. Not included in ICRC */
	/* 4.24 - 4.29 */
	 u_int8_t Rsv6;
	/* Description -  */
	/* 4.30 - 4.30 */
	 u_int8_t Becn;
	/* Description -  */
	/* 4.31 - 8.31 */
	 u_int8_t Fecn;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description - Packet Sequence Number */
	/* 8.0 - 8.23 */
	 u_int32_t PSN;
	/* Description - TX - 0, RX - ignore. This field is included in the ICRC */
	/* 8.24 - 8.30 */
	 u_int8_t Rsv7;
	/* Description - Requests responder to schedule an ACK on the associated QP */
	/* 8.31 - 12.31 */
	 u_int8_t AckReq;
};

/* Description -  Base Transport Header */
/* Size in bytes - 12 */
struct IB_BTH {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.15 */
	 struct P_Key_Block_Element P_Key;
	/* Description - Transport Header Version (is set to 0x0) */
	/* 0.16 - 0.19 */
	 u_int8_t TVer;
	/* Description - Pad Count */
	/* 0.20 - 0.21 */
	 u_int8_t PadCnt;
	/* Description - If zero, it means there is no change in current migration state */
	/* 0.22 - 0.22 */
	 u_int8_t MigReq;
	/* Description - Solicited Event */
	/* 0.23 - 0.23 */
	 u_int8_t SE;
	/* Description - operation code (011_00100 - UD_SEND_only) */
	/* 0.24 - 4.31 */
	 u_int8_t OpCode;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - Destination QP */
	/* 4.0 - 4.23 */
	 u_int32_t DestQP;
	/* Description - Reserved(variant) - 8 bits. TX - 0, RX - ignore. Not included in ICRC */
	/* 4.24 - 8.31 */
	 u_int8_t Rsv8;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description - Packet Sequence Number */
	/* 8.0 - 8.23 */
	 u_int32_t PSN;
	/* Description - TX - 0, RX - ignore. This field is included in the ICRC */
	/* 8.24 - 8.30 */
	 u_int8_t Rsv7;
	/* Description - Requests responder to schedule an ACK on the associated QP */
	/* 8.31 - 12.31 */
	 u_int8_t AckReq;
};

/* Description -  Global Routing Header */
/* Size in bytes - 40 */
struct IB_GRH {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  This field identifies sequences of packets requiring special handling. */
	/* 0.0 - 0.19 */
	 u_int32_t FlowLabel;
	/* Description - This field is used by IBA to communicate global service level. */
	/* 0.20 - 0.27 */
	 u_int8_t TClass;
	/* Description - This field indicates version of the GRH */
	/* 0.28 - 4.31 */
	 u_int8_t IPVer;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - This field sets a strict bound on the number of hops \;between subnets a packet can make before being discarded. \;This is enforced only by routers. */
	/* 4.0 - 4.7 */
	 u_int8_t HopLmt;
	/* Description - This field identifies the header following the GRH. \;This field is included for compatibility with IPV6 headers. \;It should indicate IBA transport. */
	/* 4.8 - 4.15 */
	 u_int8_t NxtHdr;
	/* Description - For an IBA packet this field specifies the number of bytes \;starting from the first byte after the GRH, \;up to and including the last byte of the ICRC. \;For a raw IPv6 datagram this field specifies the number \;of bytes starting from the first byte after the GRH, \;up to but not including either the VCRC or any padding, \;to achieve a multiple of 4 byte packet length. \;For raw IPv6 datagrams padding is determined \;from the lower 2 bits of this GRH:PayLen field. \;Note: GRH:PayLen is different from LRH:PkyLen. */
	/* 4.16 - 8.31 */
	 u_int16_t PayLen;
/*---------------- DWORD[2] (Offset 0x8) ----------------*/
	/* Description -  */
	/* 8.0 - 24.31 */
	 struct GID_Block_Element SGID;
/*---------------- DWORD[6] (Offset 0x18) ----------------*/
	/* Description -  */
	/* 24.0 - 40.31 */
	 struct GID_Block_Element DGID;
};

/* Description -  Raw Header */
/* Size in bytes - 4 */
struct IB_RWH {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 0.15 */
	 u_int16_t EtherType;
	/* Description -  */
	/* 0.16 - 4.31 */
	 u_int16_t HW_Reserved;
};

/* Description -  Local Routed Header */
/* Size in bytes - 8 */
struct IB_LRH {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description - Destination Local Identifier */
	/* 0.0 - 0.15 */
	 u_int16_t DLID;
	/* Description - Link Next Header (3-IBA/GRH, 2-IBA/BTH, 1-Raw/IPv6, 0-Raw/RWH) */
	/* 0.16 - 0.17 */
	 u_int8_t LNH;
	/* Description - The 2 -bit Reserve field, shall be transmited as 00 and ignored on receive */
	/* 0.18 - 0.19 */
	 u_int8_t Rsv2;
	/* Description - Service Level  */
	/* 0.20 - 0.23 */
	 u_int8_t Sl;
	/* Description - Link Version (shall be set to 0x0) */
	/* 0.24 - 0.27 */
	 u_int8_t LVer;
	/* Description - Virtual Lane */
	/* 0.28 - 4.31 */
	 u_int8_t Vl;
/*---------------- DWORD[1] (Offset 0x4) ----------------*/
	/* Description - Source Local Identifier */
	/* 4.0 - 4.15 */
	 u_int16_t SLID;
	/* Description - Packet Length (from LRH upto Variant CRC in 4 byte words) */
	/* 4.16 - 4.26 */
	 u_int16_t PktLen;
	/* Description - The 5 - bit Reserve shall be transmited as 0x0 and ignored on receive */
	/* 4.27 - 8.31 */
	 u_int8_t Rsv5;
};

/* Description -   */
/* Size in bytes - 16777216 */
union PACKETS_EXTERNAL {
/*---------------- DWORD[0] (Offset 0x0) ----------------*/
	/* Description -  */
	/* 0.0 - 8.31 */
	 struct IB_LRH IB_LRH;
	/* Description -  */
	/* 0.0 - 4.31 */
	 struct IB_RWH IB_RWH;
	/* Description -  */
	/* 0.0 - 40.31 */
	 struct IB_GRH IB_GRH;
	/* Description -  */
	/* 0.0 - 12.31 */
	 struct IB_BTH IB_BTH;
	/* Description -  */
	/* 0.0 - 12.31 */
	 struct IB_BTH_CNP IB_BTH_CNP;
	/* Description -  */
	/* 0.0 - 4.31 */
	 struct IB_RDETH IB_RDETH;
	/* Description -  */
	/* 0.0 - 8.31 */
	 struct IB_DETH IB_DETH;
	/* Description -  */
	/* 0.0 - 16.31 */
	 struct IB_RETH IB_RETH;
	/* Description -  */
	/* 0.0 - 28.31 */
	 struct IB_AtomicETH IB_AtomicETH;
	/* Description -  */
	/* 0.0 - 4.31 */
	 struct IB_AETH IB_AETH;
	/* Description -  */
	/* 0.0 - 8.31 */
	 struct IB_AtomicAckETH IB_AtomicAckETH;
	/* Description -  */
	/* 0.0 - 4.31 */
	 struct IB_ImmDt IB_ImmDt;
	/* Description -  */
	/* 0.0 - 4.31 */
	 struct IB_IETH IB_IETH;
	/* Description -  */
	/* 0.0 - 4.31 */
	 struct ICRC ICRC;
	/* Description -  */
	/* 0.0 - 4.31 */
	 struct VCRC VCRC;
	/* Description -  */
	/* 0.0 - 24.31 */
	 struct MAD_Header_Common MAD_Header_Common;
	/* Description -  */
	/* 0.0 - 36.31 */
	 struct MAD_Header_Common_With_RMPP MAD_Header_Common_With_RMPP;
	/* Description -  */
	/* 0.0 - 24.31 */
	 struct MAD_Header_SMP_Direct_Routed MAD_Header_SMP_Direct_Routed;
	/* Description -  */
	/* 0.0 - 256.31 */
	 struct MAD_SMP_Direct_Routed MAD_SMP_Direct_Routed;
	/* Description -  */
	/* 0.0 - 256.31 */
	 struct MAD_SMP_LID_Routed MAD_SMP_LID_Routed;
	/* Description -  */
	/* 0.0 - 256.31 */
	 struct MAD_PerformanceManagement MAD_PerformanceManagement;
	/* Description -  */
	/* 0.0 - 256.31 */
	 struct MAD_SubnetAdministartion MAD_SubnetAdministartion;
	/* Description -  */
	/* 0.0 - 52.31 */
	 struct SA_MCMMemberRecord SA_MCMMemberRecord;
	/* Description -  */
	/* 0.0 - 256.31 */
	 struct MAD_VendorSpec MAD_VendorSpec;
	/* Description -  */
	/* 0.0 - 256.31 */
	 struct MAD_CongestionControl MAD_CongestionControl;
	/* Description -  */
	/* 0.0 - 72.31 */
	 struct IB_ClassPortInfo IB_ClassPortInfo;
	/* Description -  */
	/* 0.0 - 131072.31 */
	 union SMP_MADS SMP_MADS;
	/* Description -  */
	/* 0.0 - 131072.31 */
	 union PERFORMANCE_MADS PERFORMANCE_MADS;
	/* Description -  */
	/* 0.0 - 131072.31 */
	 union CONGESTION_CONTOL CONGESTION_CONTOL;
	/* Description -  */
	/* 0.0 - 131072.31 */
	 union VENDOR_SPECS VENDOR_SPECS;
};


/*================= PACK/UNPACK/PRINT FUNCTIONS ======================*/
/* PSID_Block_Element */
void PSID_Block_Element_pack(const struct PSID_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void PSID_Block_Element_unpack(struct PSID_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void PSID_Block_Element_print(const struct PSID_Block_Element *ptr_struct, FILE* file, int indent_level);
int PSID_Block_Element_size();
void PSID_Block_Element_dump(const struct PSID_Block_Element *ptr_struct, FILE* file);
/* GID_Block_Element */
void GID_Block_Element_pack(const struct GID_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void GID_Block_Element_unpack(struct GID_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void GID_Block_Element_print(const struct GID_Block_Element *ptr_struct, FILE* file, int indent_level);
int GID_Block_Element_size();
void GID_Block_Element_dump(const struct GID_Block_Element *ptr_struct, FILE* file);
/* uint64bit */
void uint64bit_pack(const struct uint64bit *ptr_struct, u_int8_t* ptr_buff);
void uint64bit_unpack(struct uint64bit *ptr_struct, const u_int8_t* ptr_buff);
void uint64bit_print(const struct uint64bit *ptr_struct, FILE* file, int indent_level);
int uint64bit_size();
void uint64bit_dump(const struct uint64bit *ptr_struct, FILE* file);
/* CCTI_Entry_ListElement */
void CCTI_Entry_ListElement_pack(const struct CCTI_Entry_ListElement *ptr_struct, u_int8_t* ptr_buff);
void CCTI_Entry_ListElement_unpack(struct CCTI_Entry_ListElement *ptr_struct, const u_int8_t* ptr_buff);
void CCTI_Entry_ListElement_print(const struct CCTI_Entry_ListElement *ptr_struct, FILE* file, int indent_level);
int CCTI_Entry_ListElement_size();
void CCTI_Entry_ListElement_dump(const struct CCTI_Entry_ListElement *ptr_struct, FILE* file);
/* CACongestionEntryListElement */
void CACongestionEntryListElement_pack(const struct CACongestionEntryListElement *ptr_struct, u_int8_t* ptr_buff);
void CACongestionEntryListElement_unpack(struct CACongestionEntryListElement *ptr_struct, const u_int8_t* ptr_buff);
void CACongestionEntryListElement_print(const struct CACongestionEntryListElement *ptr_struct, FILE* file, int indent_level);
int CACongestionEntryListElement_size();
void CACongestionEntryListElement_dump(const struct CACongestionEntryListElement *ptr_struct, FILE* file);
/* CongestionLogEventListCAElement */
void CongestionLogEventListCAElement_pack(const struct CongestionLogEventListCAElement *ptr_struct, u_int8_t* ptr_buff);
void CongestionLogEventListCAElement_unpack(struct CongestionLogEventListCAElement *ptr_struct, const u_int8_t* ptr_buff);
void CongestionLogEventListCAElement_print(const struct CongestionLogEventListCAElement *ptr_struct, FILE* file, int indent_level);
int CongestionLogEventListCAElement_size();
void CongestionLogEventListCAElement_dump(const struct CongestionLogEventListCAElement *ptr_struct, FILE* file);
/* CongestionEntryListSwitchElement */
void CongestionEntryListSwitchElement_pack(const struct CongestionEntryListSwitchElement *ptr_struct, u_int8_t* ptr_buff);
void CongestionEntryListSwitchElement_unpack(struct CongestionEntryListSwitchElement *ptr_struct, const u_int8_t* ptr_buff);
void CongestionEntryListSwitchElement_print(const struct CongestionEntryListSwitchElement *ptr_struct, FILE* file, int indent_level);
int CongestionEntryListSwitchElement_size();
void CongestionEntryListSwitchElement_dump(const struct CongestionEntryListSwitchElement *ptr_struct, FILE* file);
/* SWInfo_Block_Element */
void SWInfo_Block_Element_pack(const struct SWInfo_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void SWInfo_Block_Element_unpack(struct SWInfo_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void SWInfo_Block_Element_print(const struct SWInfo_Block_Element *ptr_struct, FILE* file, int indent_level);
int SWInfo_Block_Element_size();
void SWInfo_Block_Element_dump(const struct SWInfo_Block_Element *ptr_struct, FILE* file);
/* FWInfo_Block_Element */
void FWInfo_Block_Element_pack(const struct FWInfo_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void FWInfo_Block_Element_unpack(struct FWInfo_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void FWInfo_Block_Element_print(const struct FWInfo_Block_Element *ptr_struct, FILE* file, int indent_level);
int FWInfo_Block_Element_size();
void FWInfo_Block_Element_dump(const struct FWInfo_Block_Element *ptr_struct, FILE* file);
/* HWInfo_Block_Element */
void HWInfo_Block_Element_pack(const struct HWInfo_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void HWInfo_Block_Element_unpack(struct HWInfo_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void HWInfo_Block_Element_print(const struct HWInfo_Block_Element *ptr_struct, FILE* file, int indent_level);
int HWInfo_Block_Element_size();
void HWInfo_Block_Element_dump(const struct HWInfo_Block_Element *ptr_struct, FILE* file);
/* CC_KeyViolation */
void CC_KeyViolation_pack(const struct CC_KeyViolation *ptr_struct, u_int8_t* ptr_buff);
void CC_KeyViolation_unpack(struct CC_KeyViolation *ptr_struct, const u_int8_t* ptr_buff);
void CC_KeyViolation_print(const struct CC_KeyViolation *ptr_struct, FILE* file, int indent_level);
int CC_KeyViolation_size();
void CC_KeyViolation_dump(const struct CC_KeyViolation *ptr_struct, FILE* file);
/* CCTI_Entry_List */
void CCTI_Entry_List_pack(const struct CCTI_Entry_List *ptr_struct, u_int8_t* ptr_buff);
void CCTI_Entry_List_unpack(struct CCTI_Entry_List *ptr_struct, const u_int8_t* ptr_buff);
void CCTI_Entry_List_print(const struct CCTI_Entry_List *ptr_struct, FILE* file, int indent_level);
int CCTI_Entry_List_size();
void CCTI_Entry_List_dump(const struct CCTI_Entry_List *ptr_struct, FILE* file);
/* CACongestionEntryList */
void CACongestionEntryList_pack(const struct CACongestionEntryList *ptr_struct, u_int8_t* ptr_buff);
void CACongestionEntryList_unpack(struct CACongestionEntryList *ptr_struct, const u_int8_t* ptr_buff);
void CACongestionEntryList_print(const struct CACongestionEntryList *ptr_struct, FILE* file, int indent_level);
int CACongestionEntryList_size();
void CACongestionEntryList_dump(const struct CACongestionEntryList *ptr_struct, FILE* file);
/* SwitchPortCongestionSettingElement */
void SwitchPortCongestionSettingElement_pack(const struct SwitchPortCongestionSettingElement *ptr_struct, u_int8_t* ptr_buff);
void SwitchPortCongestionSettingElement_unpack(struct SwitchPortCongestionSettingElement *ptr_struct, const u_int8_t* ptr_buff);
void SwitchPortCongestionSettingElement_print(const struct SwitchPortCongestionSettingElement *ptr_struct, FILE* file, int indent_level);
int SwitchPortCongestionSettingElement_size();
void SwitchPortCongestionSettingElement_dump(const struct SwitchPortCongestionSettingElement *ptr_struct, FILE* file);
/* UINT256 */
void UINT256_pack(const struct UINT256 *ptr_struct, u_int8_t* ptr_buff);
void UINT256_unpack(struct UINT256 *ptr_struct, const u_int8_t* ptr_buff);
void UINT256_print(const struct UINT256 *ptr_struct, FILE* file, int indent_level);
int UINT256_size();
void UINT256_dump(const struct UINT256 *ptr_struct, FILE* file);
/* CC_SwitchCongestionSetting_Control_Map */
void CC_SwitchCongestionSetting_Control_Map_pack(const struct CC_SwitchCongestionSetting_Control_Map *ptr_struct, u_int8_t* ptr_buff);
void CC_SwitchCongestionSetting_Control_Map_unpack(struct CC_SwitchCongestionSetting_Control_Map *ptr_struct, const u_int8_t* ptr_buff);
void CC_SwitchCongestionSetting_Control_Map_print(const struct CC_SwitchCongestionSetting_Control_Map *ptr_struct, FILE* file, int indent_level);
int CC_SwitchCongestionSetting_Control_Map_size();
void CC_SwitchCongestionSetting_Control_Map_dump(const struct CC_SwitchCongestionSetting_Control_Map *ptr_struct, FILE* file);
/* CongestionLogEventListCA */
void CongestionLogEventListCA_pack(const struct CongestionLogEventListCA *ptr_struct, u_int8_t* ptr_buff);
void CongestionLogEventListCA_unpack(struct CongestionLogEventListCA *ptr_struct, const u_int8_t* ptr_buff);
void CongestionLogEventListCA_print(const struct CongestionLogEventListCA *ptr_struct, FILE* file, int indent_level);
int CongestionLogEventListCA_size();
void CongestionLogEventListCA_dump(const struct CongestionLogEventListCA *ptr_struct, FILE* file);
/* CongestionEntryListSwitch */
void CongestionEntryListSwitch_pack(const struct CongestionEntryListSwitch *ptr_struct, u_int8_t* ptr_buff);
void CongestionEntryListSwitch_unpack(struct CongestionEntryListSwitch *ptr_struct, const u_int8_t* ptr_buff);
void CongestionEntryListSwitch_print(const struct CongestionEntryListSwitch *ptr_struct, FILE* file, int indent_level);
int CongestionEntryListSwitch_size();
void CongestionEntryListSwitch_dump(const struct CongestionEntryListSwitch *ptr_struct, FILE* file);
/* PortSampleControlOptionMask */
void PortSampleControlOptionMask_pack(const struct PortSampleControlOptionMask *ptr_struct, u_int8_t* ptr_buff);
void PortSampleControlOptionMask_unpack(struct PortSampleControlOptionMask *ptr_struct, const u_int8_t* ptr_buff);
void PortSampleControlOptionMask_print(const struct PortSampleControlOptionMask *ptr_struct, FILE* file, int indent_level);
int PortSampleControlOptionMask_size();
void PortSampleControlOptionMask_dump(const struct PortSampleControlOptionMask *ptr_struct, FILE* file);
/* PortCountersExtended_Mask_Block_Element */
void PortCountersExtended_Mask_Block_Element_pack(const struct PortCountersExtended_Mask_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void PortCountersExtended_Mask_Block_Element_unpack(struct PortCountersExtended_Mask_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void PortCountersExtended_Mask_Block_Element_print(const struct PortCountersExtended_Mask_Block_Element *ptr_struct, FILE* file, int indent_level);
int PortCountersExtended_Mask_Block_Element_size();
void PortCountersExtended_Mask_Block_Element_dump(const struct PortCountersExtended_Mask_Block_Element *ptr_struct, FILE* file);
/* PortCounters_Mask2_Block_Element */
void PortCounters_Mask2_Block_Element_pack(const struct PortCounters_Mask2_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void PortCounters_Mask2_Block_Element_unpack(struct PortCounters_Mask2_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void PortCounters_Mask2_Block_Element_print(const struct PortCounters_Mask2_Block_Element *ptr_struct, FILE* file, int indent_level);
int PortCounters_Mask2_Block_Element_size();
void PortCounters_Mask2_Block_Element_dump(const struct PortCounters_Mask2_Block_Element *ptr_struct, FILE* file);
/* PortCounters_Mask_Block_Element */
void PortCounters_Mask_Block_Element_pack(const struct PortCounters_Mask_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void PortCounters_Mask_Block_Element_unpack(struct PortCounters_Mask_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void PortCounters_Mask_Block_Element_print(const struct PortCounters_Mask_Block_Element *ptr_struct, FILE* file, int indent_level);
int PortCounters_Mask_Block_Element_size();
void PortCounters_Mask_Block_Element_dump(const struct PortCounters_Mask_Block_Element *ptr_struct, FILE* file);
/* LID_Port_Block_Element */
void LID_Port_Block_Element_pack(const struct LID_Port_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void LID_Port_Block_Element_unpack(struct LID_Port_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void LID_Port_Block_Element_print(const struct LID_Port_Block_Element *ptr_struct, FILE* file, int indent_level);
int LID_Port_Block_Element_size();
void LID_Port_Block_Element_dump(const struct LID_Port_Block_Element *ptr_struct, FILE* file);
/* VL_Weight_Block_Element */
void VL_Weight_Block_Element_pack(const struct VL_Weight_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void VL_Weight_Block_Element_unpack(struct VL_Weight_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void VL_Weight_Block_Element_print(const struct VL_Weight_Block_Element *ptr_struct, FILE* file, int indent_level);
int VL_Weight_Block_Element_size();
void VL_Weight_Block_Element_dump(const struct VL_Weight_Block_Element *ptr_struct, FILE* file);
/* P_Key_Block_Element */
void P_Key_Block_Element_pack(const struct P_Key_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void P_Key_Block_Element_unpack(struct P_Key_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void P_Key_Block_Element_print(const struct P_Key_Block_Element *ptr_struct, FILE* file, int indent_level);
int P_Key_Block_Element_size();
void P_Key_Block_Element_dump(const struct P_Key_Block_Element *ptr_struct, FILE* file);
/* GUID_Block_Element */
void GUID_Block_Element_pack(const struct GUID_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void GUID_Block_Element_unpack(struct GUID_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void GUID_Block_Element_print(const struct GUID_Block_Element *ptr_struct, FILE* file, int indent_level);
int GUID_Block_Element_size();
void GUID_Block_Element_dump(const struct GUID_Block_Element *ptr_struct, FILE* file);
/* TID_Block_Element */
void TID_Block_Element_pack(const struct TID_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void TID_Block_Element_unpack(struct TID_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void TID_Block_Element_print(const struct TID_Block_Element *ptr_struct, FILE* file, int indent_level);
int TID_Block_Element_size();
void TID_Block_Element_dump(const struct TID_Block_Element *ptr_struct, FILE* file);
/* VendorSpec_GeneralInfo */
void VendorSpec_GeneralInfo_pack(const struct VendorSpec_GeneralInfo *ptr_struct, u_int8_t* ptr_buff);
void VendorSpec_GeneralInfo_unpack(struct VendorSpec_GeneralInfo *ptr_struct, const u_int8_t* ptr_buff);
void VendorSpec_GeneralInfo_print(const struct VendorSpec_GeneralInfo *ptr_struct, FILE* file, int indent_level);
int VendorSpec_GeneralInfo_size();
void VendorSpec_GeneralInfo_dump(const struct VendorSpec_GeneralInfo *ptr_struct, FILE* file);
/* CC_Notice */
void CC_Notice_pack(const struct CC_Notice *ptr_struct, u_int8_t* ptr_buff);
void CC_Notice_unpack(struct CC_Notice *ptr_struct, const u_int8_t* ptr_buff);
void CC_Notice_print(const struct CC_Notice *ptr_struct, FILE* file, int indent_level);
int CC_Notice_size();
void CC_Notice_dump(const struct CC_Notice *ptr_struct, FILE* file);
/* CC_TimeStamp */
void CC_TimeStamp_pack(const struct CC_TimeStamp *ptr_struct, u_int8_t* ptr_buff);
void CC_TimeStamp_unpack(struct CC_TimeStamp *ptr_struct, const u_int8_t* ptr_buff);
void CC_TimeStamp_print(const struct CC_TimeStamp *ptr_struct, FILE* file, int indent_level);
int CC_TimeStamp_size();
void CC_TimeStamp_dump(const struct CC_TimeStamp *ptr_struct, FILE* file);
/* CC_CongestionControlTable */
void CC_CongestionControlTable_pack(const struct CC_CongestionControlTable *ptr_struct, u_int8_t* ptr_buff);
void CC_CongestionControlTable_unpack(struct CC_CongestionControlTable *ptr_struct, const u_int8_t* ptr_buff);
void CC_CongestionControlTable_print(const struct CC_CongestionControlTable *ptr_struct, FILE* file, int indent_level);
int CC_CongestionControlTable_size();
void CC_CongestionControlTable_dump(const struct CC_CongestionControlTable *ptr_struct, FILE* file);
/* CC_CACongestionSetting */
void CC_CACongestionSetting_pack(const struct CC_CACongestionSetting *ptr_struct, u_int8_t* ptr_buff);
void CC_CACongestionSetting_unpack(struct CC_CACongestionSetting *ptr_struct, const u_int8_t* ptr_buff);
void CC_CACongestionSetting_print(const struct CC_CACongestionSetting *ptr_struct, FILE* file, int indent_level);
int CC_CACongestionSetting_size();
void CC_CACongestionSetting_dump(const struct CC_CACongestionSetting *ptr_struct, FILE* file);
/* CC_SwitchPortCongestionSetting */
void CC_SwitchPortCongestionSetting_pack(const struct CC_SwitchPortCongestionSetting *ptr_struct, u_int8_t* ptr_buff);
void CC_SwitchPortCongestionSetting_unpack(struct CC_SwitchPortCongestionSetting *ptr_struct, const u_int8_t* ptr_buff);
void CC_SwitchPortCongestionSetting_print(const struct CC_SwitchPortCongestionSetting *ptr_struct, FILE* file, int indent_level);
int CC_SwitchPortCongestionSetting_size();
void CC_SwitchPortCongestionSetting_dump(const struct CC_SwitchPortCongestionSetting *ptr_struct, FILE* file);
/* CC_SwitchCongestionSetting */
void CC_SwitchCongestionSetting_pack(const struct CC_SwitchCongestionSetting *ptr_struct, u_int8_t* ptr_buff);
void CC_SwitchCongestionSetting_unpack(struct CC_SwitchCongestionSetting *ptr_struct, const u_int8_t* ptr_buff);
void CC_SwitchCongestionSetting_print(const struct CC_SwitchCongestionSetting *ptr_struct, FILE* file, int indent_level);
int CC_SwitchCongestionSetting_size();
void CC_SwitchCongestionSetting_dump(const struct CC_SwitchCongestionSetting *ptr_struct, FILE* file);
/* CC_CongestionLogCA */
void CC_CongestionLogCA_pack(const struct CC_CongestionLogCA *ptr_struct, u_int8_t* ptr_buff);
void CC_CongestionLogCA_unpack(struct CC_CongestionLogCA *ptr_struct, const u_int8_t* ptr_buff);
void CC_CongestionLogCA_print(const struct CC_CongestionLogCA *ptr_struct, FILE* file, int indent_level);
int CC_CongestionLogCA_size();
void CC_CongestionLogCA_dump(const struct CC_CongestionLogCA *ptr_struct, FILE* file);
/* CC_CongestionLogSwitch */
void CC_CongestionLogSwitch_pack(const struct CC_CongestionLogSwitch *ptr_struct, u_int8_t* ptr_buff);
void CC_CongestionLogSwitch_unpack(struct CC_CongestionLogSwitch *ptr_struct, const u_int8_t* ptr_buff);
void CC_CongestionLogSwitch_print(const struct CC_CongestionLogSwitch *ptr_struct, FILE* file, int indent_level);
int CC_CongestionLogSwitch_size();
void CC_CongestionLogSwitch_dump(const struct CC_CongestionLogSwitch *ptr_struct, FILE* file);
/* CC_CongestionKeyInfo */
void CC_CongestionKeyInfo_pack(const struct CC_CongestionKeyInfo *ptr_struct, u_int8_t* ptr_buff);
void CC_CongestionKeyInfo_unpack(struct CC_CongestionKeyInfo *ptr_struct, const u_int8_t* ptr_buff);
void CC_CongestionKeyInfo_print(const struct CC_CongestionKeyInfo *ptr_struct, FILE* file, int indent_level);
int CC_CongestionKeyInfo_size();
void CC_CongestionKeyInfo_dump(const struct CC_CongestionKeyInfo *ptr_struct, FILE* file);
/* CC_CongestionInfo */
void CC_CongestionInfo_pack(const struct CC_CongestionInfo *ptr_struct, u_int8_t* ptr_buff);
void CC_CongestionInfo_unpack(struct CC_CongestionInfo *ptr_struct, const u_int8_t* ptr_buff);
void CC_CongestionInfo_print(const struct CC_CongestionInfo *ptr_struct, FILE* file, int indent_level);
int CC_CongestionInfo_size();
void CC_CongestionInfo_dump(const struct CC_CongestionInfo *ptr_struct, FILE* file);
/* PM_PortExtendedSpeedsCounters */
void PM_PortExtendedSpeedsCounters_pack(const struct PM_PortExtendedSpeedsCounters *ptr_struct, u_int8_t* ptr_buff);
void PM_PortExtendedSpeedsCounters_unpack(struct PM_PortExtendedSpeedsCounters *ptr_struct, const u_int8_t* ptr_buff);
void PM_PortExtendedSpeedsCounters_print(const struct PM_PortExtendedSpeedsCounters *ptr_struct, FILE* file, int indent_level);
int PM_PortExtendedSpeedsCounters_size();
void PM_PortExtendedSpeedsCounters_dump(const struct PM_PortExtendedSpeedsCounters *ptr_struct, FILE* file);
/* PM_PortSamplesControl */
void PM_PortSamplesControl_pack(const struct PM_PortSamplesControl *ptr_struct, u_int8_t* ptr_buff);
void PM_PortSamplesControl_unpack(struct PM_PortSamplesControl *ptr_struct, const u_int8_t* ptr_buff);
void PM_PortSamplesControl_print(const struct PM_PortSamplesControl *ptr_struct, FILE* file, int indent_level);
int PM_PortSamplesControl_size();
void PM_PortSamplesControl_dump(const struct PM_PortSamplesControl *ptr_struct, FILE* file);
/* PM_PortSamplesResult */
void PM_PortSamplesResult_pack(const struct PM_PortSamplesResult *ptr_struct, u_int8_t* ptr_buff);
void PM_PortSamplesResult_unpack(struct PM_PortSamplesResult *ptr_struct, const u_int8_t* ptr_buff);
void PM_PortSamplesResult_print(const struct PM_PortSamplesResult *ptr_struct, FILE* file, int indent_level);
int PM_PortSamplesResult_size();
void PM_PortSamplesResult_dump(const struct PM_PortSamplesResult *ptr_struct, FILE* file);
/* PM_PortRcvErrorDetails */
void PM_PortRcvErrorDetails_pack(const struct PM_PortRcvErrorDetails *ptr_struct, u_int8_t* ptr_buff);
void PM_PortRcvErrorDetails_unpack(struct PM_PortRcvErrorDetails *ptr_struct, const u_int8_t* ptr_buff);
void PM_PortRcvErrorDetails_print(const struct PM_PortRcvErrorDetails *ptr_struct, FILE* file, int indent_level);
int PM_PortRcvErrorDetails_size();
void PM_PortRcvErrorDetails_dump(const struct PM_PortRcvErrorDetails *ptr_struct, FILE* file);
/* PM_PortXmitDiscardDetails */
void PM_PortXmitDiscardDetails_pack(const struct PM_PortXmitDiscardDetails *ptr_struct, u_int8_t* ptr_buff);
void PM_PortXmitDiscardDetails_unpack(struct PM_PortXmitDiscardDetails *ptr_struct, const u_int8_t* ptr_buff);
void PM_PortXmitDiscardDetails_print(const struct PM_PortXmitDiscardDetails *ptr_struct, FILE* file, int indent_level);
int PM_PortXmitDiscardDetails_size();
void PM_PortXmitDiscardDetails_dump(const struct PM_PortXmitDiscardDetails *ptr_struct, FILE* file);
/* PM_PortCountersExtended */
void PM_PortCountersExtended_pack(const struct PM_PortCountersExtended *ptr_struct, u_int8_t* ptr_buff);
void PM_PortCountersExtended_unpack(struct PM_PortCountersExtended *ptr_struct, const u_int8_t* ptr_buff);
void PM_PortCountersExtended_print(const struct PM_PortCountersExtended *ptr_struct, FILE* file, int indent_level);
int PM_PortCountersExtended_size();
void PM_PortCountersExtended_dump(const struct PM_PortCountersExtended *ptr_struct, FILE* file);
/* PM_PortCounters */
void PM_PortCounters_pack(const struct PM_PortCounters *ptr_struct, u_int8_t* ptr_buff);
void PM_PortCounters_unpack(struct PM_PortCounters *ptr_struct, const u_int8_t* ptr_buff);
void PM_PortCounters_print(const struct PM_PortCounters *ptr_struct, FILE* file, int indent_level);
int PM_PortCounters_size();
void PM_PortCounters_dump(const struct PM_PortCounters *ptr_struct, FILE* file);
/* SMP_MlnxExtPortInfo */
void SMP_MlnxExtPortInfo_pack(const struct SMP_MlnxExtPortInfo *ptr_struct, u_int8_t* ptr_buff);
void SMP_MlnxExtPortInfo_unpack(struct SMP_MlnxExtPortInfo *ptr_struct, const u_int8_t* ptr_buff);
void SMP_MlnxExtPortInfo_print(const struct SMP_MlnxExtPortInfo *ptr_struct, FILE* file, int indent_level);
int SMP_MlnxExtPortInfo_size();
void SMP_MlnxExtPortInfo_dump(const struct SMP_MlnxExtPortInfo *ptr_struct, FILE* file);
/* SMP_LedInfo */
void SMP_LedInfo_pack(const struct SMP_LedInfo *ptr_struct, u_int8_t* ptr_buff);
void SMP_LedInfo_unpack(struct SMP_LedInfo *ptr_struct, const u_int8_t* ptr_buff);
void SMP_LedInfo_print(const struct SMP_LedInfo *ptr_struct, FILE* file, int indent_level);
int SMP_LedInfo_size();
void SMP_LedInfo_dump(const struct SMP_LedInfo *ptr_struct, FILE* file);
/* SMP_SMInfo */
void SMP_SMInfo_pack(const struct SMP_SMInfo *ptr_struct, u_int8_t* ptr_buff);
void SMP_SMInfo_unpack(struct SMP_SMInfo *ptr_struct, const u_int8_t* ptr_buff);
void SMP_SMInfo_print(const struct SMP_SMInfo *ptr_struct, FILE* file, int indent_level);
int SMP_SMInfo_size();
void SMP_SMInfo_dump(const struct SMP_SMInfo *ptr_struct, FILE* file);
/* SMP_MulticastForwardingTable */
void SMP_MulticastForwardingTable_pack(const struct SMP_MulticastForwardingTable *ptr_struct, u_int8_t* ptr_buff);
void SMP_MulticastForwardingTable_unpack(struct SMP_MulticastForwardingTable *ptr_struct, const u_int8_t* ptr_buff);
void SMP_MulticastForwardingTable_print(const struct SMP_MulticastForwardingTable *ptr_struct, FILE* file, int indent_level);
int SMP_MulticastForwardingTable_size();
void SMP_MulticastForwardingTable_dump(const struct SMP_MulticastForwardingTable *ptr_struct, FILE* file);
/* SMP_RandomForwardingTable */
void SMP_RandomForwardingTable_pack(const struct SMP_RandomForwardingTable *ptr_struct, u_int8_t* ptr_buff);
void SMP_RandomForwardingTable_unpack(struct SMP_RandomForwardingTable *ptr_struct, const u_int8_t* ptr_buff);
void SMP_RandomForwardingTable_print(const struct SMP_RandomForwardingTable *ptr_struct, FILE* file, int indent_level);
int SMP_RandomForwardingTable_size();
void SMP_RandomForwardingTable_dump(const struct SMP_RandomForwardingTable *ptr_struct, FILE* file);
/* SMP_LinearForwardingTable */
void SMP_LinearForwardingTable_pack(const struct SMP_LinearForwardingTable *ptr_struct, u_int8_t* ptr_buff);
void SMP_LinearForwardingTable_unpack(struct SMP_LinearForwardingTable *ptr_struct, const u_int8_t* ptr_buff);
void SMP_LinearForwardingTable_print(const struct SMP_LinearForwardingTable *ptr_struct, FILE* file, int indent_level);
int SMP_LinearForwardingTable_size();
void SMP_LinearForwardingTable_dump(const struct SMP_LinearForwardingTable *ptr_struct, FILE* file);
/* SMP_VLArbitrationTable */
void SMP_VLArbitrationTable_pack(const struct SMP_VLArbitrationTable *ptr_struct, u_int8_t* ptr_buff);
void SMP_VLArbitrationTable_unpack(struct SMP_VLArbitrationTable *ptr_struct, const u_int8_t* ptr_buff);
void SMP_VLArbitrationTable_print(const struct SMP_VLArbitrationTable *ptr_struct, FILE* file, int indent_level);
int SMP_VLArbitrationTable_size();
void SMP_VLArbitrationTable_dump(const struct SMP_VLArbitrationTable *ptr_struct, FILE* file);
/* SMP_SLToVLMappingTable */
void SMP_SLToVLMappingTable_pack(const struct SMP_SLToVLMappingTable *ptr_struct, u_int8_t* ptr_buff);
void SMP_SLToVLMappingTable_unpack(struct SMP_SLToVLMappingTable *ptr_struct, const u_int8_t* ptr_buff);
void SMP_SLToVLMappingTable_print(const struct SMP_SLToVLMappingTable *ptr_struct, FILE* file, int indent_level);
int SMP_SLToVLMappingTable_size();
void SMP_SLToVLMappingTable_dump(const struct SMP_SLToVLMappingTable *ptr_struct, FILE* file);
/* SMP_PKeyTable */
void SMP_PKeyTable_pack(const struct SMP_PKeyTable *ptr_struct, u_int8_t* ptr_buff);
void SMP_PKeyTable_unpack(struct SMP_PKeyTable *ptr_struct, const u_int8_t* ptr_buff);
void SMP_PKeyTable_print(const struct SMP_PKeyTable *ptr_struct, FILE* file, int indent_level);
int SMP_PKeyTable_size();
void SMP_PKeyTable_dump(const struct SMP_PKeyTable *ptr_struct, FILE* file);
/* SMP_PortInfo */
void SMP_PortInfo_pack(const struct SMP_PortInfo *ptr_struct, u_int8_t* ptr_buff);
void SMP_PortInfo_unpack(struct SMP_PortInfo *ptr_struct, const u_int8_t* ptr_buff);
void SMP_PortInfo_print(const struct SMP_PortInfo *ptr_struct, FILE* file, int indent_level);
int SMP_PortInfo_size();
void SMP_PortInfo_dump(const struct SMP_PortInfo *ptr_struct, FILE* file);
/* SMP_GUIDInfo */
void SMP_GUIDInfo_pack(const struct SMP_GUIDInfo *ptr_struct, u_int8_t* ptr_buff);
void SMP_GUIDInfo_unpack(struct SMP_GUIDInfo *ptr_struct, const u_int8_t* ptr_buff);
void SMP_GUIDInfo_print(const struct SMP_GUIDInfo *ptr_struct, FILE* file, int indent_level);
int SMP_GUIDInfo_size();
void SMP_GUIDInfo_dump(const struct SMP_GUIDInfo *ptr_struct, FILE* file);
/* SMP_SwitchInfo */
void SMP_SwitchInfo_pack(const struct SMP_SwitchInfo *ptr_struct, u_int8_t* ptr_buff);
void SMP_SwitchInfo_unpack(struct SMP_SwitchInfo *ptr_struct, const u_int8_t* ptr_buff);
void SMP_SwitchInfo_print(const struct SMP_SwitchInfo *ptr_struct, FILE* file, int indent_level);
int SMP_SwitchInfo_size();
void SMP_SwitchInfo_dump(const struct SMP_SwitchInfo *ptr_struct, FILE* file);
/* SMP_NodeInfo */
void SMP_NodeInfo_pack(const struct SMP_NodeInfo *ptr_struct, u_int8_t* ptr_buff);
void SMP_NodeInfo_unpack(struct SMP_NodeInfo *ptr_struct, const u_int8_t* ptr_buff);
void SMP_NodeInfo_print(const struct SMP_NodeInfo *ptr_struct, FILE* file, int indent_level);
int SMP_NodeInfo_size();
void SMP_NodeInfo_dump(const struct SMP_NodeInfo *ptr_struct, FILE* file);
/* SMP_NodeDesc */
void SMP_NodeDesc_pack(const struct SMP_NodeDesc *ptr_struct, u_int8_t* ptr_buff);
void SMP_NodeDesc_unpack(struct SMP_NodeDesc *ptr_struct, const u_int8_t* ptr_buff);
void SMP_NodeDesc_print(const struct SMP_NodeDesc *ptr_struct, FILE* file, int indent_level);
int SMP_NodeDesc_size();
void SMP_NodeDesc_dump(const struct SMP_NodeDesc *ptr_struct, FILE* file);
/* CC_Mgt_Data_Block_Element */
void CC_Mgt_Data_Block_Element_pack(const struct CC_Mgt_Data_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void CC_Mgt_Data_Block_Element_unpack(struct CC_Mgt_Data_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void CC_Mgt_Data_Block_Element_print(const struct CC_Mgt_Data_Block_Element *ptr_struct, FILE* file, int indent_level);
int CC_Mgt_Data_Block_Element_size();
void CC_Mgt_Data_Block_Element_dump(const struct CC_Mgt_Data_Block_Element *ptr_struct, FILE* file);
/* CC_Log_Data_Block_Element */
void CC_Log_Data_Block_Element_pack(const struct CC_Log_Data_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void CC_Log_Data_Block_Element_unpack(struct CC_Log_Data_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void CC_Log_Data_Block_Element_print(const struct CC_Log_Data_Block_Element *ptr_struct, FILE* file, int indent_level);
int CC_Log_Data_Block_Element_size();
void CC_Log_Data_Block_Element_dump(const struct CC_Log_Data_Block_Element *ptr_struct, FILE* file);
/* MAD_Header_Common */
void MAD_Header_Common_pack(const struct MAD_Header_Common *ptr_struct, u_int8_t* ptr_buff);
void MAD_Header_Common_unpack(struct MAD_Header_Common *ptr_struct, const u_int8_t* ptr_buff);
void MAD_Header_Common_print(const struct MAD_Header_Common *ptr_struct, FILE* file, int indent_level);
int MAD_Header_Common_size();
void MAD_Header_Common_dump(const struct MAD_Header_Common *ptr_struct, FILE* file);
/* VendorSpecific_MAD_Data_Block_Element */
void VendorSpecific_MAD_Data_Block_Element_pack(const struct VendorSpecific_MAD_Data_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void VendorSpecific_MAD_Data_Block_Element_unpack(struct VendorSpecific_MAD_Data_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void VendorSpecific_MAD_Data_Block_Element_print(const struct VendorSpecific_MAD_Data_Block_Element *ptr_struct, FILE* file, int indent_level);
int VendorSpecific_MAD_Data_Block_Element_size();
void VendorSpecific_MAD_Data_Block_Element_dump(const struct VendorSpecific_MAD_Data_Block_Element *ptr_struct, FILE* file);
/* SubnetAdministartion_MAD_Data_Block_Element */
void SubnetAdministartion_MAD_Data_Block_Element_pack(const struct SubnetAdministartion_MAD_Data_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void SubnetAdministartion_MAD_Data_Block_Element_unpack(struct SubnetAdministartion_MAD_Data_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void SubnetAdministartion_MAD_Data_Block_Element_print(const struct SubnetAdministartion_MAD_Data_Block_Element *ptr_struct, FILE* file, int indent_level);
int SubnetAdministartion_MAD_Data_Block_Element_size();
void SubnetAdministartion_MAD_Data_Block_Element_dump(const struct SubnetAdministartion_MAD_Data_Block_Element *ptr_struct, FILE* file);
/* MAD_Header_Common_With_RMPP */
void MAD_Header_Common_With_RMPP_pack(const struct MAD_Header_Common_With_RMPP *ptr_struct, u_int8_t* ptr_buff);
void MAD_Header_Common_With_RMPP_unpack(struct MAD_Header_Common_With_RMPP *ptr_struct, const u_int8_t* ptr_buff);
void MAD_Header_Common_With_RMPP_print(const struct MAD_Header_Common_With_RMPP *ptr_struct, FILE* file, int indent_level);
int MAD_Header_Common_With_RMPP_size();
void MAD_Header_Common_With_RMPP_dump(const struct MAD_Header_Common_With_RMPP *ptr_struct, FILE* file);
/* PerfManagement_MAD_Data_Block_Element */
void PerfManagement_MAD_Data_Block_Element_pack(const struct PerfManagement_MAD_Data_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void PerfManagement_MAD_Data_Block_Element_unpack(struct PerfManagement_MAD_Data_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void PerfManagement_MAD_Data_Block_Element_print(const struct PerfManagement_MAD_Data_Block_Element *ptr_struct, FILE* file, int indent_level);
int PerfManagement_MAD_Data_Block_Element_size();
void PerfManagement_MAD_Data_Block_Element_dump(const struct PerfManagement_MAD_Data_Block_Element *ptr_struct, FILE* file);
/* SMP_MAD_Data_Block_Element */
void SMP_MAD_Data_Block_Element_pack(const struct SMP_MAD_Data_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void SMP_MAD_Data_Block_Element_unpack(struct SMP_MAD_Data_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void SMP_MAD_Data_Block_Element_print(const struct SMP_MAD_Data_Block_Element *ptr_struct, FILE* file, int indent_level);
int SMP_MAD_Data_Block_Element_size();
void SMP_MAD_Data_Block_Element_dump(const struct SMP_MAD_Data_Block_Element *ptr_struct, FILE* file);
/* DirRPath_Block_Element */
void DirRPath_Block_Element_pack(const struct DirRPath_Block_Element *ptr_struct, u_int8_t* ptr_buff);
void DirRPath_Block_Element_unpack(struct DirRPath_Block_Element *ptr_struct, const u_int8_t* ptr_buff);
void DirRPath_Block_Element_print(const struct DirRPath_Block_Element *ptr_struct, FILE* file, int indent_level);
int DirRPath_Block_Element_size();
void DirRPath_Block_Element_dump(const struct DirRPath_Block_Element *ptr_struct, FILE* file);
/* MAD_Header_SMP_Direct_Routed */
void MAD_Header_SMP_Direct_Routed_pack(const struct MAD_Header_SMP_Direct_Routed *ptr_struct, u_int8_t* ptr_buff);
void MAD_Header_SMP_Direct_Routed_unpack(struct MAD_Header_SMP_Direct_Routed *ptr_struct, const u_int8_t* ptr_buff);
void MAD_Header_SMP_Direct_Routed_print(const struct MAD_Header_SMP_Direct_Routed *ptr_struct, FILE* file, int indent_level);
int MAD_Header_SMP_Direct_Routed_size();
void MAD_Header_SMP_Direct_Routed_dump(const struct MAD_Header_SMP_Direct_Routed *ptr_struct, FILE* file);
/* VENDOR_SPECS */
void VENDOR_SPECS_pack(const union VENDOR_SPECS *ptr_struct, u_int8_t* ptr_buff);
void VENDOR_SPECS_unpack(union VENDOR_SPECS *ptr_struct, const u_int8_t* ptr_buff);
void VENDOR_SPECS_print(const union VENDOR_SPECS *ptr_struct, FILE* file, int indent_level);
int VENDOR_SPECS_size();
void VENDOR_SPECS_dump(const union VENDOR_SPECS *ptr_struct, FILE* file);
/* CONGESTION_CONTOL */
void CONGESTION_CONTOL_pack(const union CONGESTION_CONTOL *ptr_struct, u_int8_t* ptr_buff);
void CONGESTION_CONTOL_unpack(union CONGESTION_CONTOL *ptr_struct, const u_int8_t* ptr_buff);
void CONGESTION_CONTOL_print(const union CONGESTION_CONTOL *ptr_struct, FILE* file, int indent_level);
int CONGESTION_CONTOL_size();
void CONGESTION_CONTOL_dump(const union CONGESTION_CONTOL *ptr_struct, FILE* file);
/* PERFORMANCE_MADS */
void PERFORMANCE_MADS_pack(const union PERFORMANCE_MADS *ptr_struct, u_int8_t* ptr_buff);
void PERFORMANCE_MADS_unpack(union PERFORMANCE_MADS *ptr_struct, const u_int8_t* ptr_buff);
void PERFORMANCE_MADS_print(const union PERFORMANCE_MADS *ptr_struct, FILE* file, int indent_level);
int PERFORMANCE_MADS_size();
void PERFORMANCE_MADS_dump(const union PERFORMANCE_MADS *ptr_struct, FILE* file);
/* SMP_MADS */
void SMP_MADS_pack(const union SMP_MADS *ptr_struct, u_int8_t* ptr_buff);
void SMP_MADS_unpack(union SMP_MADS *ptr_struct, const u_int8_t* ptr_buff);
void SMP_MADS_print(const union SMP_MADS *ptr_struct, FILE* file, int indent_level);
int SMP_MADS_size();
void SMP_MADS_dump(const union SMP_MADS *ptr_struct, FILE* file);
/* IB_ClassPortInfo */
void IB_ClassPortInfo_pack(const struct IB_ClassPortInfo *ptr_struct, u_int8_t* ptr_buff);
void IB_ClassPortInfo_unpack(struct IB_ClassPortInfo *ptr_struct, const u_int8_t* ptr_buff);
void IB_ClassPortInfo_print(const struct IB_ClassPortInfo *ptr_struct, FILE* file, int indent_level);
int IB_ClassPortInfo_size();
void IB_ClassPortInfo_dump(const struct IB_ClassPortInfo *ptr_struct, FILE* file);
/* MAD_CongestionControl */
void MAD_CongestionControl_pack(const struct MAD_CongestionControl *ptr_struct, u_int8_t* ptr_buff);
void MAD_CongestionControl_unpack(struct MAD_CongestionControl *ptr_struct, const u_int8_t* ptr_buff);
void MAD_CongestionControl_print(const struct MAD_CongestionControl *ptr_struct, FILE* file, int indent_level);
int MAD_CongestionControl_size();
void MAD_CongestionControl_dump(const struct MAD_CongestionControl *ptr_struct, FILE* file);
/* MAD_VendorSpec */
void MAD_VendorSpec_pack(const struct MAD_VendorSpec *ptr_struct, u_int8_t* ptr_buff);
void MAD_VendorSpec_unpack(struct MAD_VendorSpec *ptr_struct, const u_int8_t* ptr_buff);
void MAD_VendorSpec_print(const struct MAD_VendorSpec *ptr_struct, FILE* file, int indent_level);
int MAD_VendorSpec_size();
void MAD_VendorSpec_dump(const struct MAD_VendorSpec *ptr_struct, FILE* file);
/* SA_MCMMemberRecord */
void SA_MCMMemberRecord_pack(const struct SA_MCMMemberRecord *ptr_struct, u_int8_t* ptr_buff);
void SA_MCMMemberRecord_unpack(struct SA_MCMMemberRecord *ptr_struct, const u_int8_t* ptr_buff);
void SA_MCMMemberRecord_print(const struct SA_MCMMemberRecord *ptr_struct, FILE* file, int indent_level);
int SA_MCMMemberRecord_size();
void SA_MCMMemberRecord_dump(const struct SA_MCMMemberRecord *ptr_struct, FILE* file);
/* MAD_SubnetAdministartion */
void MAD_SubnetAdministartion_pack(const struct MAD_SubnetAdministartion *ptr_struct, u_int8_t* ptr_buff);
void MAD_SubnetAdministartion_unpack(struct MAD_SubnetAdministartion *ptr_struct, const u_int8_t* ptr_buff);
void MAD_SubnetAdministartion_print(const struct MAD_SubnetAdministartion *ptr_struct, FILE* file, int indent_level);
int MAD_SubnetAdministartion_size();
void MAD_SubnetAdministartion_dump(const struct MAD_SubnetAdministartion *ptr_struct, FILE* file);
/* MAD_PerformanceManagement */
void MAD_PerformanceManagement_pack(const struct MAD_PerformanceManagement *ptr_struct, u_int8_t* ptr_buff);
void MAD_PerformanceManagement_unpack(struct MAD_PerformanceManagement *ptr_struct, const u_int8_t* ptr_buff);
void MAD_PerformanceManagement_print(const struct MAD_PerformanceManagement *ptr_struct, FILE* file, int indent_level);
int MAD_PerformanceManagement_size();
void MAD_PerformanceManagement_dump(const struct MAD_PerformanceManagement *ptr_struct, FILE* file);
/* MAD_SMP_LID_Routed */
void MAD_SMP_LID_Routed_pack(const struct MAD_SMP_LID_Routed *ptr_struct, u_int8_t* ptr_buff);
void MAD_SMP_LID_Routed_unpack(struct MAD_SMP_LID_Routed *ptr_struct, const u_int8_t* ptr_buff);
void MAD_SMP_LID_Routed_print(const struct MAD_SMP_LID_Routed *ptr_struct, FILE* file, int indent_level);
int MAD_SMP_LID_Routed_size();
void MAD_SMP_LID_Routed_dump(const struct MAD_SMP_LID_Routed *ptr_struct, FILE* file);
/* MAD_SMP_Direct_Routed */
void MAD_SMP_Direct_Routed_pack(const struct MAD_SMP_Direct_Routed *ptr_struct, u_int8_t* ptr_buff);
void MAD_SMP_Direct_Routed_unpack(struct MAD_SMP_Direct_Routed *ptr_struct, const u_int8_t* ptr_buff);
void MAD_SMP_Direct_Routed_print(const struct MAD_SMP_Direct_Routed *ptr_struct, FILE* file, int indent_level);
int MAD_SMP_Direct_Routed_size();
void MAD_SMP_Direct_Routed_dump(const struct MAD_SMP_Direct_Routed *ptr_struct, FILE* file);
/* VCRC */
void VCRC_pack(const struct VCRC *ptr_struct, u_int8_t* ptr_buff);
void VCRC_unpack(struct VCRC *ptr_struct, const u_int8_t* ptr_buff);
void VCRC_print(const struct VCRC *ptr_struct, FILE* file, int indent_level);
int VCRC_size();
void VCRC_dump(const struct VCRC *ptr_struct, FILE* file);
/* ICRC */
void ICRC_pack(const struct ICRC *ptr_struct, u_int8_t* ptr_buff);
void ICRC_unpack(struct ICRC *ptr_struct, const u_int8_t* ptr_buff);
void ICRC_print(const struct ICRC *ptr_struct, FILE* file, int indent_level);
int ICRC_size();
void ICRC_dump(const struct ICRC *ptr_struct, FILE* file);
/* IB_IETH */
void IB_IETH_pack(const struct IB_IETH *ptr_struct, u_int8_t* ptr_buff);
void IB_IETH_unpack(struct IB_IETH *ptr_struct, const u_int8_t* ptr_buff);
void IB_IETH_print(const struct IB_IETH *ptr_struct, FILE* file, int indent_level);
int IB_IETH_size();
void IB_IETH_dump(const struct IB_IETH *ptr_struct, FILE* file);
/* IB_ImmDt */
void IB_ImmDt_pack(const struct IB_ImmDt *ptr_struct, u_int8_t* ptr_buff);
void IB_ImmDt_unpack(struct IB_ImmDt *ptr_struct, const u_int8_t* ptr_buff);
void IB_ImmDt_print(const struct IB_ImmDt *ptr_struct, FILE* file, int indent_level);
int IB_ImmDt_size();
void IB_ImmDt_dump(const struct IB_ImmDt *ptr_struct, FILE* file);
/* IB_AtomicAckETH */
void IB_AtomicAckETH_pack(const struct IB_AtomicAckETH *ptr_struct, u_int8_t* ptr_buff);
void IB_AtomicAckETH_unpack(struct IB_AtomicAckETH *ptr_struct, const u_int8_t* ptr_buff);
void IB_AtomicAckETH_print(const struct IB_AtomicAckETH *ptr_struct, FILE* file, int indent_level);
int IB_AtomicAckETH_size();
void IB_AtomicAckETH_dump(const struct IB_AtomicAckETH *ptr_struct, FILE* file);
/* IB_AETH */
void IB_AETH_pack(const struct IB_AETH *ptr_struct, u_int8_t* ptr_buff);
void IB_AETH_unpack(struct IB_AETH *ptr_struct, const u_int8_t* ptr_buff);
void IB_AETH_print(const struct IB_AETH *ptr_struct, FILE* file, int indent_level);
int IB_AETH_size();
void IB_AETH_dump(const struct IB_AETH *ptr_struct, FILE* file);
/* IB_AtomicETH */
void IB_AtomicETH_pack(const struct IB_AtomicETH *ptr_struct, u_int8_t* ptr_buff);
void IB_AtomicETH_unpack(struct IB_AtomicETH *ptr_struct, const u_int8_t* ptr_buff);
void IB_AtomicETH_print(const struct IB_AtomicETH *ptr_struct, FILE* file, int indent_level);
int IB_AtomicETH_size();
void IB_AtomicETH_dump(const struct IB_AtomicETH *ptr_struct, FILE* file);
/* IB_RETH */
void IB_RETH_pack(const struct IB_RETH *ptr_struct, u_int8_t* ptr_buff);
void IB_RETH_unpack(struct IB_RETH *ptr_struct, const u_int8_t* ptr_buff);
void IB_RETH_print(const struct IB_RETH *ptr_struct, FILE* file, int indent_level);
int IB_RETH_size();
void IB_RETH_dump(const struct IB_RETH *ptr_struct, FILE* file);
/* IB_DETH */
void IB_DETH_pack(const struct IB_DETH *ptr_struct, u_int8_t* ptr_buff);
void IB_DETH_unpack(struct IB_DETH *ptr_struct, const u_int8_t* ptr_buff);
void IB_DETH_print(const struct IB_DETH *ptr_struct, FILE* file, int indent_level);
int IB_DETH_size();
void IB_DETH_dump(const struct IB_DETH *ptr_struct, FILE* file);
/* IB_RDETH */
void IB_RDETH_pack(const struct IB_RDETH *ptr_struct, u_int8_t* ptr_buff);
void IB_RDETH_unpack(struct IB_RDETH *ptr_struct, const u_int8_t* ptr_buff);
void IB_RDETH_print(const struct IB_RDETH *ptr_struct, FILE* file, int indent_level);
int IB_RDETH_size();
void IB_RDETH_dump(const struct IB_RDETH *ptr_struct, FILE* file);
/* IB_BTH_CNP */
void IB_BTH_CNP_pack(const struct IB_BTH_CNP *ptr_struct, u_int8_t* ptr_buff);
void IB_BTH_CNP_unpack(struct IB_BTH_CNP *ptr_struct, const u_int8_t* ptr_buff);
void IB_BTH_CNP_print(const struct IB_BTH_CNP *ptr_struct, FILE* file, int indent_level);
int IB_BTH_CNP_size();
void IB_BTH_CNP_dump(const struct IB_BTH_CNP *ptr_struct, FILE* file);
/* IB_BTH */
void IB_BTH_pack(const struct IB_BTH *ptr_struct, u_int8_t* ptr_buff);
void IB_BTH_unpack(struct IB_BTH *ptr_struct, const u_int8_t* ptr_buff);
void IB_BTH_print(const struct IB_BTH *ptr_struct, FILE* file, int indent_level);
int IB_BTH_size();
void IB_BTH_dump(const struct IB_BTH *ptr_struct, FILE* file);
/* IB_GRH */
void IB_GRH_pack(const struct IB_GRH *ptr_struct, u_int8_t* ptr_buff);
void IB_GRH_unpack(struct IB_GRH *ptr_struct, const u_int8_t* ptr_buff);
void IB_GRH_print(const struct IB_GRH *ptr_struct, FILE* file, int indent_level);
int IB_GRH_size();
void IB_GRH_dump(const struct IB_GRH *ptr_struct, FILE* file);
/* IB_RWH */
void IB_RWH_pack(const struct IB_RWH *ptr_struct, u_int8_t* ptr_buff);
void IB_RWH_unpack(struct IB_RWH *ptr_struct, const u_int8_t* ptr_buff);
void IB_RWH_print(const struct IB_RWH *ptr_struct, FILE* file, int indent_level);
int IB_RWH_size();
void IB_RWH_dump(const struct IB_RWH *ptr_struct, FILE* file);
/* IB_LRH */
void IB_LRH_pack(const struct IB_LRH *ptr_struct, u_int8_t* ptr_buff);
void IB_LRH_unpack(struct IB_LRH *ptr_struct, const u_int8_t* ptr_buff);
void IB_LRH_print(const struct IB_LRH *ptr_struct, FILE* file, int indent_level);
int IB_LRH_size();
void IB_LRH_dump(const struct IB_LRH *ptr_struct, FILE* file);
/* PACKETS_EXTERNAL */
void PACKETS_EXTERNAL_pack(const union PACKETS_EXTERNAL *ptr_struct, u_int8_t* ptr_buff);
void PACKETS_EXTERNAL_unpack(union PACKETS_EXTERNAL *ptr_struct, const u_int8_t* ptr_buff);
void PACKETS_EXTERNAL_print(const union PACKETS_EXTERNAL *ptr_struct, FILE* file, int indent_level);
int PACKETS_EXTERNAL_size();
void PACKETS_EXTERNAL_dump(const union PACKETS_EXTERNAL *ptr_struct, FILE* file);
#endif // PACKETS_LAYOUTS_H
