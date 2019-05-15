/*
 * Copyright (c) 2007-2011 Intel Corporation.  All rights reserved.
 * 
 * This Software is licensed under one of the following licenses:
 * 
 * 1) under the terms of the "Common Public License 1.0" a copy of which is
 *    in the file LICENSE.txt in the root directory. The license is also
 *    available from the Open Source Initiative, see 
 *    http://www.opensource.org/licenses/cpl.php.
 * 
 * 2) under the terms of the "The BSD License" a copy of which is in the file
 *    LICENSE2.txt in the root directory. The license is also available from
 *    the Open Source Initiative, see
 *    http://www.opensource.org/licenses/bsd-license.php.
 * 
 * 3) under the terms of the "GNU General Public License (GPL) Version 2" a
 *    copy of which is in the file LICENSE3.txt in the root directory. The
 *    license is also available from the Open Source Initiative, see
 *    http://www.opensource.org/licenses/gpl-license.php.
 * 
 * Licensee has the right to choose one of the above licenses.
 * 
 * Redistributions of source code must retain the above copyright
 * notice and one of the license notices.
 * 
 * Redistributions in binary form must reproduce both the above copyright
 * notice, one of the license notices in the documentation
 * and/or other materials provided with the distribution.
 */

/**********************************************************************
 *
 * HEADER: dat_ib_extensions.h
 *
 * PURPOSE: extensions to the DAT API for IB transport specific services 
 *	    NOTE: Prototyped IB extension support in openib-cma 1.2 provider.
 *	          Applications MUST recompile with new dat.h definitions
 *		  and include this file.
 *
 * Description: Header file for "uDAPL: User Direct Access Programming
 *		Library, Version: 2.0"
 *
 * Mapping rules:
 *      All global symbols are prepended with "DAT_" or "dat_"
 *      All DAT objects have an 'api' tag which, such as 'ep' or 'lmr'
 *      The method table is in the provider definition structure.
 *
 *
 **********************************************************************/
#ifndef _DAT_IB_EXTENSIONS_H_
#define _DAT_IB_EXTENSIONS_H_

/* 
 * Provider specific attribute strings for extension support 
 *	returned with dat_ia_query() and 
 *	DAT_PROVIDER_ATTR_MASK == DAT_PROVIDER_FIELD_PROVIDER_SPECIFIC_ATTR
 *
 *	DAT_NAMED_ATTR	name == extended operations and version, 
 *			version_value = version number of extension API
 */

/* 2.0.1 - Initial IB extension support, atomic and immed data
 *         dat_ib_post_fetch_and_add()
 *         dat_ib_post_cmp_and_swap()
 *         dat_ib_post_rdma_write_immed()
 *		
 * 2.0.2 - Add UD support, post send and remote_ah via connect events 
 *         dat_ib_post_send_ud()
 *
 * 2.0.3 - Add query/print counter support for IA, EP, and EVD's 
 *         dat_query_counters(), dat_print_counters()
 *
 * 2.0.4 - Add DAT_IB_UD_CONNECTION_REJECT_EVENT extended UD event
 * 2.0.5 - Add DAT_IB_UD extended UD connection error events
 * 2.0.6 - Add MPI over IB collective extensions
 *
 */
#define DAT_IB_EXTENSION_VERSION	206	/* 2.0.6 */
#define DAT_IB_ATTR_COUNTERS		"DAT_COUNTERS"
#define DAT_IB_ATTR_FETCH_AND_ADD	"DAT_IB_FETCH_AND_ADD"
#define DAT_IB_ATTR_CMP_AND_SWAP	"DAT_IB_CMP_AND_SWAP"
#define DAT_IB_ATTR_IMMED_DATA		"DAT_IB_IMMED_DATA"
#define DAT_IB_ATTR_UD			"DAT_IB_UD"

#define DAT_IB_COLL_SET_CLOCK		"DAT_COLL_SET_CLOCK"
#define DAT_IB_COLL_READ_CLOCK		"DAT_COLL_READ_CLOCK"
#define DAT_IB_COLL_BROADCAST		"DAT_COLL_BROADCAST"
#define DAT_IB_COLL_BARRIER		"DAT_COLL_BARRIER"
#define DAT_IB_COLL_SCATTER		"DAT_COLL_SCATTER"
#define DAT_IB_COLL_SCATTERV		"DAT_COLL_SCATTERV"
#define DAT_IB_COLL_GATHER		"DAT_COLL_GATHER"
#define DAT_IB_COLL_GATHERV		"DAT_COLL_GATHERV"
#define DAT_IB_COLL_ALLGATHER		"DAT_COLL_ALLGATHER"
#define DAT_IB_COLL_ALLGATHERV		"DAT_COLL_ALLGATHERV"
#define DAT_IB_COLL_ALLTOALL		"DAT_COLL_ALLTOALL"
#define DAT_IB_COLL_ALLTOALLV		"DAT_COLL_ALLTOALLV"
#define DAT_IB_COLL_REDUCE		"DAT_COLL_REDUCE"
#define DAT_IB_COLL_ALLREDUCE		"DAT_COLL_ALLREDUCE"
#define DAT_IB_COLL_REDUCE_SCATTER	"DAT_COLL_REDUCE_SCATTER"
#define DAT_IB_COLL_SCAN		"DAT_COLL_SCAN"

/* Collective handle */
typedef	DAT_HANDLE	DAT_IB_COLLECTIVE_HANDLE;

/* 
 * Definition for extended EVENT numbers, DAT_IB_EXTENSION_BASE_RANGE
 * is used by these extensions as a starting point for extended event numbers 
 *
 * DAT_IB_DTO_EVENT - All extended data transfers - req/recv evd
 * DAT_IB_AH_EVENT - address handle resolution - connect evd
 */
typedef enum dat_ib_event_number
{
	DAT_IB_DTO_EVENT = DAT_IB_EXTENSION_RANGE_BASE,
	DAT_IB_UD_CONNECTION_REQUEST_EVENT,
	DAT_IB_UD_CONNECTION_EVENT_ESTABLISHED,
	DAT_IB_UD_CONNECTION_REJECT_EVENT,
	DAT_IB_UD_CONNECTION_ERROR_EVENT,
	DAT_IB_COLLECTIVE_EVENT,

} DAT_IB_EVENT_NUMBER;

/* 
 * Extension operations 
 */
typedef enum dat_ib_op
{
	DAT_IB_FETCH_AND_ADD_OP,
	DAT_IB_CMP_AND_SWAP_OP,
	DAT_IB_RDMA_WRITE_IMMED_OP,
	DAT_IB_UD_SEND_OP,
	DAT_IB_QUERY_COUNTERS_OP,
	DAT_IB_PRINT_COUNTERS_OP,
	DAT_IB_COLLECTIVE_CREATE_MEMBER_OP,
	DAT_IB_COLLECTIVE_FREE_MEMBER_OP,
	DAT_IB_COLLECTIVE_CREATE_GROUP_OP,
	DAT_IB_COLLECTIVE_FREE_GROUP_OP,
	DAT_IB_COLLECTIVE_SET_CLOCK_OP,
	DAT_IB_COLLECTIVE_READ_CLOCK_OP,
	DAT_IB_COLLECTIVE_SCATTER_OP,
	DAT_IB_COLLECTIVE_SCATTERV_OP,
	DAT_IB_COLLECTIVE_GATHER_OP,
	DAT_IB_COLLECTIVE_GATHERV_OP,
	DAT_IB_COLLECTIVE_ALLGATHER_OP,
	DAT_IB_COLLECTIVE_ALLGATHERV_OP,
	DAT_IB_COLLECTIVE_ALLTOALL_OP,
	DAT_IB_COLLECTIVE_ALLTOALLV_OP,
	DAT_IB_COLLECTIVE_REDUCE_OP,
	DAT_IB_COLLECTIVE_ALLREDUCE_OP,
	DAT_IB_COLLECTIVE_REDUCE_SCATTER_OP,
	DAT_IB_COLLECTIVE_SCAN_OP,
	DAT_IB_COLLECTIVE_BROADCAST_OP,
	DAT_IB_COLLECTIVE_BARRIER_OP,
	
} DAT_IB_OP;

/*
 * The DAT_IB_EXT_TYPE enum specifies the type of extension operation that just
 * completed. All IB extended completion types both, DTO and NON-DTO, are 
 * reported in the extended operation type with the single DAT_IB_DTO_EVENT type. 
 * The specific extended DTO operation is reported with a DAT_IB_DTOS type in the 
 * operation field of the base DAT_EVENT structure. All other extended events are 
 * identified by unique DAT_IB_EVENT_NUMBER types.
 */
typedef enum dat_ib_ext_type
{
	DAT_IB_FETCH_AND_ADD,		// 0
	DAT_IB_CMP_AND_SWAP,		// 1
	DAT_IB_RDMA_WRITE_IMMED,	// 2
	DAT_IB_RDMA_WRITE_IMMED_DATA,	// 3
	DAT_IB_RECV_IMMED_DATA,		// 4
	DAT_IB_UD_CONNECT_REQUEST,	// 5
	DAT_IB_UD_REMOTE_AH,		// 6
	DAT_IB_UD_PASSIVE_REMOTE_AH,	// 7
	DAT_IB_UD_SEND,			// 8
	DAT_IB_UD_RECV,			// 9
	DAT_IB_UD_CONNECT_REJECT,	// 10
	DAT_IB_UD_CONNECT_ERROR,	// 11

	DAT_IB_COLLECTIVE_CREATE_STATUS,	// 12
	DAT_IB_COLLECTIVE_CREATE_DATA,		// 13
	DAT_IB_COLLECTIVE_CLOCK_SET_STATUS,	// 14
	DAT_IB_COLLECTIVE_SCATTER_STATUS,	// 15
	DAT_IB_COLLECTIVE_SCATTERV_STATUS,	// 16
	DAT_IB_COLLECTIVE_GATHER_STATUS,	// 17
	DAT_IB_COLLECTIVE_GATHERV_STATUS,	// 18
	DAT_IB_COLLECTIVE_ALLGATHER_STATUS,	// 19
	DAT_IB_COLLECTIVE_ALLGATHERV_STATUS,	// 20
	DAT_IB_COLLECTIVE_ALLTOALL_STATUS,	// 21
	DAT_IB_COLLECTIVE_ALLTOALLV_STATUS,	// 22
	DAT_IB_COLLECTIVE_REDUCE_STATUS,	// 23
	DAT_IB_COLLECTIVE_ALLREDUCE_STATUS,	// 24
	DAT_IB_COLLECTIVE_REDUCE_SCATTER_STATUS,// 25
	DAT_IB_COLLECTIVE_SCAN_STATUS,		// 26
	DAT_IB_COLLECTIVE_BROADCAST_STATUS,	// 27
	DAT_IB_COLLECTIVE_BARRIER_STATUS,	// 28

} DAT_IB_EXT_TYPE;

/* 
 * Extension event status
 */
typedef enum dat_ib_status
{
	DAT_OP_SUCCESS = DAT_SUCCESS,
	DAT_IB_OP_ERR,
	DAT_IB_COLL_COMP_ERR,

} DAT_IB_STATUS;

/* 
 * Definitions for additional extension type RETURN codes above
 * standard DAT types. Included with standard DAT_TYPE_STATUS 
 * bits using a DAT_EXTENSION BASE as a starting point.
 */
typedef enum dat_ib_return
{
	DAT_IB_ERR = DAT_EXTENSION_BASE,
	DAT_IB_COLLECTIVE_ERR

} DAT_IB_RETURN;

/* 
 * Definition for extended IB DTO operations, DAT_DTO_EXTENSION_BASE
 * is used by DAT extensions as a starting point of extension DTOs 
 */
typedef enum dat_ib_dtos
{
	DAT_IB_DTO_RDMA_WRITE_IMMED = DAT_DTO_EXTENSION_BASE,
	DAT_IB_DTO_RECV_IMMED,
	DAT_IB_DTO_FETCH_ADD,
	DAT_IB_DTO_CMP_SWAP,
	DAT_IB_DTO_RECV_MSG_IMMED,
	DAT_IB_DTO_SEND_UD,
	DAT_IB_DTO_RECV_UD,
	DAT_IB_DTO_RECV_UD_IMMED,	
	DAT_IB_DTO_COLLECTIVES,
	
} DAT_IB_DTOS;

/* 
 * Definitions for additional extension handle types beyond 
 * standard DAT handle. New Bit definitions MUST start at 
 * DAT_HANDLE_TYPE_EXTENSION_BASE
 */
typedef enum dat_ib_handle_type
{
    DAT_IB_HANDLE_TYPE_EXT = DAT_HANDLE_TYPE_EXTENSION_BASE,
    DAT_IB_HANDLE_TYPE_COLLECTIVE

} DAT_IB_HANDLE_TYPE;

/*
 * The DAT_IB_EVD_EXTENSION_FLAGS enum specifies the EVD extension flags that
 * do not map directly to existing DAT_EVD_FLAGS. This new EVD flag has been 
 * added to identify an extended EVD that does not fit the existing stream 
 * types.
 */
typedef enum dat_ib_evd_extension_flags
{
	DAT_IB_EVD_EXTENSION_FLAG = DAT_EVD_EXTENSION_BASE,

} DAT_IB_EVD_EXTENSION_FLAGS;

/* 
 * Definition for memory privilege extension flags.
 * New privileges required for new atomic DTO type extensions.
 * New Bit definitions MUST start at DAT_MEM_PRIV_EXTENSION
 */
typedef enum dat_ib_mem_priv_flags
{
	DAT_IB_MEM_PRIV_REMOTE_ATOMIC = DAT_MEM_PRIV_EXTENSION_BASE,
	
} DAT_IB_MEM_PRIV_FLAGS;

/* 
 * Definition for IB address handle, unreliable datagram.
 */
typedef struct dat_ib_addr_handle
{
    struct ibv_ah	*ah;
    DAT_UINT32	        qpn;
    DAT_SOCK_ADDR6	ia_addr;
   
} DAT_IB_ADDR_HANDLE;

/*
 * Definition for the value filed of extended event that contains immediate data
 */
typedef struct dat_ib_immed_data 
{
    DAT_UINT32			data;

} DAT_IB_IMMED_DATA;

/* definition for IB collective event data */
typedef struct dat_ib_collective_event_data
{
    DAT_HANDLE  handle;
    DAT_CONTEXT context;

} DAT_IB_COLLECTIVE_EVENT_DATA;

/* 
 * Definitions for extended event data:
 *	When dat_event->event_number >= DAT_IB_EXTENSION_BASE_RANGE
 *	then dat_event->extension_data == DAT_IB_EXTENSION_EVENT_DATA type
 *	and ((DAT_IB_EXTENSION_EVENT_DATA*)dat_event->extension_data)->type
 *	specifies extension data values. 
 * NOTE: DAT_IB_EXTENSION_EVENT_DATA cannot exceed 64 bytes as defined by 
 *	 "DAT_UINT64 extension_data[8]" in DAT_EVENT (dat.h)
 *
 *  Provide UD address handles via extended connect establishment. 
 *  ia_addr provided with extended conn events for reference.
 */
typedef struct dat_ib_extension_event_data
{
    DAT_IB_EXT_TYPE	type;
    DAT_IB_STATUS	status;
    union {
		DAT_IB_IMMED_DATA	immed;
    } val;
    union {
	    DAT_IB_ADDR_HANDLE			remote_ah;
	    DAT_IB_COLLECTIVE_EVENT_DATA	coll;
    };

} DAT_IB_EXTENSION_EVENT_DATA;

/* 
 * Definitions for additional extension handle types beyond 
 * standard DAT handle. New Bit definitions MUST start at 
 * DAT_HANDLE_TYPE_EXTENSION_BASE
 */
typedef enum dat_ib_service_type
{
    DAT_IB_SERVICE_TYPE_UD   = DAT_SERVICE_TYPE_EXTENSION_BASE,

} DAT_IB_SERVICE_TYPE;

/*
 * Definitions for 64-bit IA Counters
 */
typedef enum dat_ia_counters
{
	DCNT_IA_PZ_CREATE,
	DCNT_IA_PZ_FREE,
	DCNT_IA_LMR_CREATE,
	DCNT_IA_LMR_FREE,
	DCNT_IA_RMR_CREATE,
	DCNT_IA_RMR_FREE,
	DCNT_IA_PSP_CREATE,
	DCNT_IA_PSP_CREATE_ANY,
	DCNT_IA_PSP_FREE,
	DCNT_IA_RSP_CREATE,
	DCNT_IA_RSP_FREE,
	DCNT_IA_EVD_CREATE,
	DCNT_IA_EVD_FREE,
	DCNT_IA_EP_CREATE,
	DCNT_IA_EP_FREE,
	DCNT_IA_SRQ_CREATE,
	DCNT_IA_SRQ_FREE,
	DCNT_IA_SP_CR,
	DCNT_IA_SP_CR_ACCEPTED,
	DCNT_IA_SP_CR_REJECTED,
	DCNT_IA_MEM_ALLOC,
	DCNT_IA_MEM_ALLOC_DATA,
	DCNT_IA_MEM_FREE,
	DCNT_IA_ASYNC_ERROR,
	DCNT_IA_ASYNC_QP_ERROR,
	DCNT_IA_ASYNC_CQ_ERROR,
	DCNT_IA_ALL_COUNTERS,  /* MUST be last */

} DAT_IA_COUNTERS;

/*
 * Definitions for 64-bit EP Counters
 */
typedef enum dat_ep_counters
{
	DCNT_EP_CONNECT,
	DCNT_EP_DISCONNECT,
	DCNT_EP_POST_SEND,
	DCNT_EP_POST_SEND_DATA,
	DCNT_EP_POST_SEND_UD,
	DCNT_EP_POST_SEND_UD_DATA,
	DCNT_EP_POST_RECV,
	DCNT_EP_POST_RECV_DATA,
	DCNT_EP_POST_WRITE,
	DCNT_EP_POST_WRITE_DATA,
	DCNT_EP_POST_WRITE_IMM,
	DCNT_EP_POST_WRITE_IMM_DATA,
	DCNT_EP_POST_READ,
	DCNT_EP_POST_READ_DATA,
	DCNT_EP_POST_CMP_SWAP,
	DCNT_EP_POST_FETCH_ADD,
	DCNT_EP_RECV,
	DCNT_EP_RECV_DATA,
	DCNT_EP_RECV_UD,
	DCNT_EP_RECV_UD_DATA,
	DCNT_EP_RECV_IMM,
	DCNT_EP_RECV_IMM_DATA,
	DCNT_EP_RECV_RDMA_IMM,
	DCNT_EP_RECV_RDMA_IMM_DATA,
	DCNT_EP_ALL_COUNTERS,  /* MUST be last */

} DAT_EP_COUNTERS;

/*
 * Definitions for 64-bit EVD Counters
 */
typedef enum dat_evd_counters
{
	DCNT_EVD_WAIT,
	DCNT_EVD_WAIT_BLOCKED,
	DCNT_EVD_WAIT_NOTIFY,
	DCNT_EVD_DEQUEUE,
	DCNT_EVD_DEQUEUE_FOUND,
	DCNT_EVD_DEQUEUE_NOT_FOUND,
	DCNT_EVD_DEQUEUE_POLL,
	DCNT_EVD_DEQUEUE_POLL_FOUND,
	DCNT_EVD_CONN_CALLBACK,
	DCNT_EVD_DTO_CALLBACK,
	DCNT_EVD_ALL_COUNTERS,  /* MUST be last */

} DAT_EVD_COUNTERS;

/*
 * Data type for reduce operations
 */
typedef enum dat_ib_collective_data_type
{
        DAT_IB_COLLECTIVE_TYPE_INT8,
        DAT_IB_COLLECTIVE_TYPE_UINT8,
        DAT_IB_COLLECTIVE_TYPE_INT16,
        DAT_IB_COLLECTIVE_TYPE_UINT16,
        DAT_IB_COLLECTIVE_TYPE_INT32,
        DAT_IB_COLLECTIVE_TYPE_UINT32,
        DAT_IB_COLLECTIVE_TYPE_INT64,
        DAT_IB_COLLECTIVE_TYPE_UINT64,
        DAT_IB_COLLECTIVE_TYPE_FLOAT,
        DAT_IB_COLLECTIVE_TYPE_DOUBLE,
        DAT_IB_COLLECTIVE_TYPE_LONG_DOUBLE,
        DAT_IB_COLLECTIVE_TYPE_SHORT_INT,
        DAT_IB_COLLECTIVE_TYPE_2INT,
        DAT_IB_COLLECTIVE_TYPE_FLOAT_INT,
        DAT_IB_COLLECTIVE_TYPE_LONG_INT,
        DAT_IB_COLLECTIVE_TYPE_DOUBLE_INT,
        
} DAT_IB_COLLECTIVE_DATA_TYPE;

/*
 * Opcode for reduce operations
 */
typedef enum dat_ib_collective_reduce_data_op
{
        DAT_IB_COLLECTIVE_REDUCE_OP_MAX, 
        DAT_IB_COLLECTIVE_REDUCE_OP_MIN,
        DAT_IB_COLLECTIVE_REDUCE_OP_SUM,
        DAT_IB_COLLECTIVE_REDUCE_OP_PROD,
        DAT_IB_COLLECTIVE_REDUCE_OP_LAND,
        DAT_IB_COLLECTIVE_REDUCE_OP_BAND,
        DAT_IB_COLLECTIVE_REDUCE_OP_LOR,
        DAT_IB_COLLECTIVE_REDUCE_OP_BOR,
        DAT_IB_COLLECTIVE_REDUCE_OP_LXOR,
        DAT_IB_COLLECTIVE_REDUCE_OP_BXOR,
        DAT_IB_COLLECTIVE_REDUCE_OP_MAXLOC,
        DAT_IB_COLLECTIVE_REDUCE_OP_MINLOC

} DAT_IB_COLLECTIVE_REDUCE_DATA_OP;

/*
 * For group creation
 */
typedef unsigned int DAT_IB_COLLECTIVE_RANK;
typedef unsigned int DAT_IB_COLLECTIVE_ID;
typedef	void * DAT_IB_COLLECTIVE_MEMBER;

typedef struct dat_ib_collective_group
{
	int	local_size;       /* # of processes on this node */
	int	local_rank;       /* my rank within the node */
	int 	*local_ranks;     /* global rank for each local process */
	int	external_size;    /* # of nodes, each node has exactly one external process (local root) */
	int	external_rank;    /* my rank among all external processes if one of them, otherwise -1 */
	int	*external_ranks;  /* global rank for each external process */
	int	*intranode_table; /* mapping from global rank to local rank. -1 if the process is on a different node */
	int	*internode_table; /* mapping from global rank to external rank. -1 if the process is >not external */
	int	is_comm_world;

} DAT_IB_COLLECTIVE_GROUP;

/* Extended RETURN and EVENT STATUS string helper functions */

/* DAT_EXT_RETURN error to string */
static __inline__ DAT_RETURN DAT_API
dat_strerror_extension (
    IN  DAT_IB_RETURN 		value,
    OUT const char 		**message )
{
	switch( DAT_GET_TYPE(value) ) {
	case DAT_IB_ERR:
		*message = "DAT_IB_ERR";
		return DAT_SUCCESS;
	default:
		/* standard DAT return type */
		return(dat_strerror(value, message, NULL));
	}
}

/* DAT_EXT_STATUS error to string */
static __inline__ DAT_RETURN DAT_API
dat_strerror_ext_status (
    IN  DAT_IB_STATUS 	value,
    OUT const char 	**message )
{
	switch(value) {
	case 0:
		*message = " ";
		return DAT_SUCCESS;
	case DAT_IB_OP_ERR:
		*message = "DAT_IB_OP_ERR";
		return DAT_SUCCESS;
	default:
		*message = "unknown extension status";
		return DAT_INVALID_PARAMETER;
	}
}

/* 
 * Extended IB transport specific APIs
 *  redirection via DAT extension function
 *  va_arg function: DAT_HANDLE and OP type MUST be first 2 parameters
 *
 *  RETURN VALUE: DAT_RETURN
 */

/*
 * This asynchronous call is modeled after the InfiniBand atomic 
 * Fetch and Add operation. The add_value is added to the 64 bit 
 * value stored at the remote memory location specified in remote_iov
 * and the result is stored in the local_iov.  
 */
#define dat_ib_post_fetch_and_add(ep, add_val, lbuf, cookie, rbuf, flgs) \
	dat_extension_op(\
		IN (DAT_EP_HANDLE) (ep), \
		IN (DAT_IB_OP) DAT_IB_FETCH_AND_ADD_OP, \
		IN (DAT_UINT64) (add_val), \
		IN (DAT_LMR_TRIPLET *) (lbuf), \
		IN (cookie), \
		IN (DAT_RMR_TRIPLET *) (rbuf), \
		IN (DAT_COMPLETION_FLAGS) (flgs))
				
/*
 * This asynchronous call is modeled after the InfiniBand atomic 
 * Compare and Swap operation. The cmp_value is compared to the 64 bit 
 * value stored at the remote memory location specified in remote_iov.  
 * If the two values are equal, the 64 bit swap_value is stored in 
 * the remote memory location.  In all cases, the original 64 bit 
 * value stored in the remote memory location is copied to the local_iov.
 */
#define dat_ib_post_cmp_and_swap(ep, cmp_val, swap_val, lbuf, cookie, rbuf, flgs) \
	dat_extension_op(\
		IN (DAT_EP_HANDLE) (ep), \
		IN (DAT_IB_OP) DAT_IB_CMP_AND_SWAP_OP, \
		IN (DAT_UINT64) (cmp_val), \
		IN (DAT_UINT64) (swap_val), \
		IN (DAT_LMR_TRIPLET *) (lbuf), \
		IN (cookie), \
		IN (DAT_RMR_TRIPLET *) (rbuf), \
		IN (DAT_COMPLETION_FLAGS) (flgs))

/* 
 * RDMA Write with IMMEDIATE:
 *
 * This asynchronous call is modeled after the InfiniBand rdma write with  
 * immediate data operation. Event completion for the request completes as an 
 * DAT_EXTENSION with extension type set to DAT_DTO_EXTENSION_IMMED_DATA.
 * Event completion on the remote endpoint completes as receive DTO operation
 * type of DAT_EXTENSION with operation set to DAT_DTO_EXTENSION_IMMED_DATA.
 * The immediate data will be provided in the extented DTO event data structure.
 *
 * Note to Consumers: the immediate data will consume a receive
 * buffer at the Data Sink. 
 *
 * Other extension flags:
 *	n/a
 */
#define dat_ib_post_rdma_write_immed(ep, size, lbuf, cookie, rbuf, idata, flgs) \
	dat_extension_op(\
		IN (DAT_EP_HANDLE) (ep), \
		IN (DAT_IB_OP) DAT_IB_RDMA_WRITE_IMMED_OP, \
		IN (DAT_COUNT) (size), \
		IN (DAT_LMR_TRIPLET *) (lbuf), \
		IN (cookie), \
		IN (DAT_RMR_TRIPLET *) (rbuf), \
		IN (DAT_UINT32) (idata), \
		IN (DAT_COMPLETION_FLAGS) (flgs))

/* 
 * Unreliable datagram: msg send 
 *
 * This asynchronous call is modeled after the InfiniBand UD message send
 * Event completion for the request completes as an 
 * DAT_EXTENSION with extension type set to DAT_DTO_EXTENSION_UD_SEND.
 * Event completion on the remote endpoint completes as receive DTO operation
 * type of DAT_EXTENSION with operation set to DAT_DTO_EXTENSION_UD_RECV.
 *
 * Other extension flags:
 *	n/a
 */
#define dat_ib_post_send_ud(ep, segments, lbuf, ah_ptr, cookie, flgs) \
	dat_extension_op(\
		IN (DAT_EP_HANDLE) (ep), \
		IN (DAT_IB_OP) DAT_IB_UD_SEND_OP, \
		IN (DAT_COUNT) (segments), \
		IN (DAT_LMR_TRIPLET *) (lbuf), \
		IN (DAT_IB_ADDR_HANDLE *) (ah_ptr), \
		IN (cookie), \
		IN (DAT_COMPLETION_FLAGS) (flgs))

/* 
 * Unreliable datagram: msg recv 
 *
 * Mapping to standard EP post call.
 */
#define dat_ib_post_recv_ud	dat_ep_post_recv

/* 
 * Query counter(s):  
 * Provide IA, EP, or EVD and call will return appropriate counters
 * 	DAT_HANDLE dat_handle, enum cntr, *DAT_UINT64 p_cntrs_out, int reset
 *
 * use _ALL_COUNTERS to query all
 */
#define dat_ib_query_counters(dat_handle, cntr, p_cntrs_out, reset) \
	dat_extension_op(\
		IN (DAT_HANDLE) dat_handle, \
		IN (DAT_IB_OP) DAT_QUERY_COUNTERS_OP, \
		IN (int) (cntr), \
		IN (DAT_UINT64 *) (p_cntrs_out), \
		IN (int) (reset))
/* 
 * Print counter(s):  
 * Provide IA, EP, or EVD and call will print appropriate counters
 * 	DAT_HANDLE dat_handle, int cntr, int reset
 * 
 * use _ALL_COUNTERS to print all
 */
#define dat_ib_print_counters(dat_handle, cntr, reset) \
	dat_extension_op(\
		IN (DAT_HANDLE) dat_handle, \
		IN (DAT_IB_OP) DAT_PRINT_COUNTERS_OP, \
		IN (int) (cntr), \
		IN (int) (reset))

/*
 ************************ MPI IB Collective Functions ***********************
 */

/* MPI collective member and group setup functions */

/*
 * This synchronous call creates and returns local member
 * address information for a collective device or provider
 * for each rank. The size of the member address information
 * is dependent on the collective device or provider.
 * This address information, for each rank, must be exchanged
 * and used for group creation on all ranks.
 */
#define dat_ib_collective_create_member(ia_handle, progress_func, member, member_size) \
	dat_extension_op(\
		IN  (DAT_IA_HANDLE) (ia_handle), \
		IN  (DAT_IB_OP) DAT_IB_COLLECTIVE_CREATE_MEMBER_OP, \
		IN  (void *) (progress_func), \
		OUT (DAT_IB_COLLECTIVE_MEMBER *) (member), \
		OUT (DAT_UINT32 *) (member_size))

/*
 * This synchronous call destroys a previously created member
 * information associated with the this device ia_handle argument.
 */
#define dat_ib_collective_free_member(ia_handle, member) \
	dat_extension_op(\
		IN (DAT_IA_HANDLE) (ia_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_FREE_MEMBER_OP, \
		IN (DAT_IB_COLLECTIVE_MEMBER) (member))

/*
 * This asynchronous call initiates the process of creating a collective
 * group and must be called by all group members. The collective_group
 * argument points to an array of address/connection qualifier pairs that
 * identify the members of the group in rank order. The group_size argument
 * specifies the size of the group and therefore the size of the coll_group
 * array. The self argument identifies the rank of the caller.
 * The group_id argument specifies a network-unique identifier for this
 * instance of the collective group. The group_info provides global and local
 * rank and process information. All members of the group must specify
 * the same group_id value for the same collective instance. The evd_handle
 * argument specifies the EVD used for all asynchronous collective completions
 * including this call. The user_context argument will be returned in the
 * DAT_EXT_COLLECTIVE_CREATE_DATA event.
 *
 * On a successful completion, each group member will receive a
 * DAT_EXT_COLLECTIVE_CREATE_DATA event on the EVD specified by evd_handle.
 * The event contains the collective handle, the rank of the receiving
 * Endpoint within the collective group, the size of the group, and the
 * caller specified user_context. The returned collective handle can be used
 * in network clock, Multicast, and other collective operations.
 *
 * RETURN VALUE: DAT_RETURN
 */
#define dat_ib_collective_create_group(members, group_size, self, group_id, group_info, evd, pd, user_context) \
	dat_extension_op(\
		IN (DAT_EVD_HANDLE) (evd), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_CREATE_GROUP_OP, \
		IN (DAT_IB_COLLECTIVE_MEMBER *) (members), \
		IN (DAT_COUNT) (group_size), \
		IN (DAT_IB_COLLECTIVE_RANK) (self), \
		IN (DAT_IB_COLLECTIVE_ID) (group_id), \
		IN (DAT_IB_COLLECTIVE_GROUP *) (group_info), \
		IN (DAT_PZ_HANDLE) (pd), \
		IN (DAT_CONTEXT) (user_context))

/*
 * This synchronous call destroys a previously created collective group
 * associated with the collective_handle argument. Any pending or
 * in-process requests associated with the collective group will be
 * terminated and be posted to the appropriate EVD.
 *
 * RETURN VALUE: DAT_RETURN
 */
#define dat_ib_collective_free_group(coll_handle) \
	dat_extension_op(\
		IN (DAT_IB_COLLECTIVE_HANDLE) (coll_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_FREE_GROUP_OP)


/* MPI collective data operations */

/*
 * This call sets the network clock associated with
 * collective_handle. A provider implementation may keep a single
 * global clock for all collective handles. When this is the case,
 * this call sets an adjustment for the given handle so that
 * subsequent calls to read the clock will be relative to the value
 * specified by clock_value. This is an asynchronous call that
 * completes on the collective EVD. The network clock will not be
 * synchronized until the request is completed. Any member of the
 * collective can set the clock and only one member should make
 * this call on behave of the entire collective.
 */
#define dat_ib_collective_set_clock(coll_handle, clock_value, user_context ) \
	dat_extension_op( \
		IN (DAT_IB_COLLECTIVE_HANDLE) (coll_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_READ_CLOCK_OP, \
		IN (DAT_UINT64) (clock_value), \
		IN (DAT_CONTEXT) (user_contex))

/*
 * This synchronous call returns the current value of the network clock
 * associated with the given collective handle. This is a light weight
 * call to minimize skew
 */
#define dat_ib_collective_read_clock(coll_handle, clock_value ) \
	dat_extension_op( \
		IN  (DAT_IB_COLLECTIVE_HANDLE) (coll_handle), \
		IN  (DAT_IB_OP) DAT_IB_COLLECTIVE_READ_CLOCK_OP, \
		OUT (DAT_UINT64 *) clock_value))

/*
 * This call performs a scatter of the data specified by the
 * send_buffer argument to the collective group specified by coll_handle.
 * Data is received in the buffer specified by the recv_buffer argument.
 * The recv_byte_count argument specifies the size of the receive buffer.
 * Data from the root send_buffer will be divided by the number of members
 * in the collective group to form equal and contiguous memory partitions.
 * Each member of the collective group will receive its rank relative
 * partition. An error is returned if the send_byte_count does not describe
 * memory that can be evenly divided by the size of the collective group.
 * An "in place" transfer for the root rank can be indicated by passing NULL
 * as the recv_buffer argument. The send_buffer and send_byte_count
 * arguments are ignored on non-root members. The operation is completed on
 * the collective EVD unless completions are suppressed through the
 * completion flags.
 */
#define dat_ib_collective_scatter(coll_handle, sendbuf, sendsize, recvbuf, recvsize, root, user_context, flags) \
	dat_extension_op( \
		IN (DAT_IB_COLLECTIVE_HANDLE) (coll_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_SCATTER_OP, \
		IN (DAT_PVOID) (sendbuf), \
		IN (DAT_COUNT) (sendsize), \
		IN (DAT_PVOID) (recvbuf), \
		IN (DAT_COUNT) (recvsize), \
		IN (DAT_IB_COLLECTIVE_RANK) (root), \
		IN (DAT_CONTEXT) (user_context), \
		IN (DAT_COMPLETION_FLAGS) (flags))

/*
 * This call performs a non-uniform scatter of the data
 * specified by the send_buffers array argument to the collective group
 * specified by coll_handle. The send_buffers array contains one buffer
 * pointer for each member of the collective group, in rank order.
 * The send_byte_counts array contains a byte count for each corresponding
 * send buffer pointer. The recv_buffer and recev_byte_count arguments
 * specify where received portions of the scatter are to be received.
 * An "in place" transfer for the root rank can be indicated by passing
 * NULL as the recv_buffer argument. The send_buffers and send_byte_counts
 * arguments are ignored on non-root members. The operation is completed
 * on the collective EVD unless completions are suppressed through the
 * completion flags.
 *
 */
#define dat_ib_collective_scatterv(coll_handle, sendbuf, sendsizes, displs, recvbuf, recvsize, root, user_context, flags) \
	dat_extension_op( \
		IN (DAT_IB_COLLECTIVE_HANDLE) (coll_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_SCATTERV_OP, \
		IN (DAT_PVOID) (sendbuf), \
		IN (DAT_COUNT *) (sendsizes), \
		IN (DAT_COUNT *) (displs), \
		IN (DAT_PVOID) (recvbuf), \
		IN (DAT_COUNT) (recvsize), \
		IN (DAT_IB_COLLECTIVE_RANK) (root), \
		IN (DAT_CONTEXT) (user_context), \
		IN (DAT_COMPLETION_FLAGS) (flags))

/*
 * This call performs a gather of the data sent by all
 * members of the collective specified by the collective_handle argument.
 * The data to be sent is specified by the send_buffer and send_byte_count
 * arguments. Data is received by the collective member specified by the
 * root argument in the buffer specified by the recv_buffer and
 * recv_byte_count arguments.  Data is placed into the receive buffer in
 * collective rank order.  An "in place" transfer for the root rank can
 * be indicated by passing NULL as the send_buffer argument.
 * The recv_buffer and recv_byte_count arguments are ignored on non-root
 * members.  The operation is completed on the collective EVD unless
 * completions are suppressed through the completion flags.
 */
#define dat_ib_collective_gather(coll_handle, sendbuf, sendsize, recvbuf, recvsize, root, user_context, flags) \
	dat_extension_op( \
		IN (DAT_IB_COLLECTIVE_HANDLE) (coll_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_GATHER_OP, \
		IN (DAT_PVOID) (sendbuf), \
		IN (DAT_COUNT) (sendsize), \
		IN (DAT_PVOID) (recvbuf), \
		IN (DAT_COUNT) (recvsize), \
		IN (DAT_IB_COLLECTIVE_RANK) (root), \
		IN (DAT_CONTEXT) (user_context), \
		IN (DAT_COMPLETION_FLAGS)(flags))

/*
 * This call performs a non-uniform gather of the data sent by
 * all members of the collective specified by the collective_handle argument.
 * The data to be sent is specified by the send_buffer and send_byte_count
 * arguments.  Data is received by the collective member specified by the
 * root argument into the buffers specified by the recv_buffers and
 * recv_byte_counts array arguments.  Data is placed into the receive buffer
 * associated with the rank that sent it. An "in place" transfer for the root
 * rank can be indicated by passing NULL as the send_buffer argument.
 * The recv_buffers and recv_byte_counts arguments are ignored on non-root
 * members.  The operation is completed on the collective EVD unless
 * completions are suppressed through the completion flags.
 */
#define dat_ib_collective_gatherv(coll_handle, sendbuf, sendsize, recvbufs, recvsizes, displs, root, user_context, flags) \
	dat_extension_op( \
		(DAT_IB_COLLECTIVE_HANDLE)(coll_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_GATHERV_OP, \
		IN (DAT_PVOID) (sendbuf), \
		IN (DAT_COUNT) (sendsize), \
		IN (DAT_PVOID) (recvbuf), \
		IN (DAT_COUNT *) (recvsizes), \
		IN (DAT_COUNT *) (displs), \
		IN (DAT_IB_COLLECTIVE_RANK) (root), \
		IN (DAT_CONTEXT) (user_context), \
		IN (DAT_COMPLETION_FLAGS) (flags))

/*
 * This call is equivalent to having all members of a collective
 * group perform a dat_collective_gather() as the root.  This results in all
 * members of the collective having identical contents in their receive buffer
 */
#define dat_ib_collective_allgather(coll_handle, sendbuf, sendsize, recvbuf, recvsize, user_context, flags) \
	dat_extension_op( \
		(DAT_IB_COLLECTIVE_HANDLE)(coll_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_ALLGATHER_OP, \
		IN (DAT_PVOID) (sendbuf), \
		IN (DAT_COUNT) (sendsize), \
		IN (DAT_PVOID) (recvbuf), \
		IN (DAT_COUNT) (recvsize), \
		IN (DAT_CONTEXT) (user_context), \
		IN (DAT_COMPLETION_FLAGS) (flags))

/*
 * This call performs a non-uniform dat_collective_allgather()
 * operation.  It is equivalent to having all members of a collective group
 * perform a dat_collective_gatherv() as the root.  This results in all
 * members of the collective having identical contents in their receive
 * buffer.
 */
#define dat_ib_collective_allgatherv(coll_handle, sendbuf, sendsize, recvbuf, recvsizes, displs, user_context, flags) \
	dat_extension_op( \
		(DAT_IB_COLLECTIVE_HANDLE)(coll_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_ALLGATHERV_OP, \
		IN (DAT_PVOID) (sendbuf), \
		IN (DAT_COUNT) (sendsize), \
		IN (DAT_PVOID) (recvbuf), \
		IN (DAT_COUNT *) (recvsizes), \
		IN (DAT_COUNT *) (displs), \
		IN (DAT_CONTEXT) (user_context), \
		IN (DAT_COMPLETION_FLAGS) (flags))

/*
 * This call is an extension of dat_collective_allgather()
 * to the case where each member sends distinct data specified by send_buffer
 * to each of the other members. The jth block sent from rank i is received
 * by rank j and is placed in the ith block of recv_buffer.
 */
#define dat_ib_collective_alltoall(coll_handle, sendbuf, sendsize, recvbuf, recvsize, user_context, flags) \
	dat_extension_op( \
		(DAT_IB_COLLECTIVE_HANDLE)(coll_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_ALLTOALL_OP, \
		IN (DAT_PVOID) (sendbuf), \
		IN (DAT_COUNT) (sendsize), \
		IN (DAT_PVOID) (recvbuf), \
		IN (DAT_COUNT) (recvsize), \
		IN (DAT_CONTEXT) (user_context), \
		IN (DAT_COMPLETION_FLAGS) (flags))

/*
 * This call performs a non-uniform dat_collective_alltoall() operation
 */
#define dat_ib_collective_alltoallv(coll_handle, sendbuf, sendsizes, senddspls, recvbuf, recvsizes, recvdispls, user_context, flags) \
	dat_extension_op( \
		(DAT_IB_COLLECTIVE_HANDLE)(coll_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_ALLTOALLV_OP, \
		IN (DAT_PVOID) (sendbuf), \
		IN (DAT_COUNT *) (sendsizes), \
		IN (DAT_COUNT *) (senddispls), \
		IN (DAT_PVOID) (recvbuf), \
		IN (DAT_COUNT *) (recvsizes), \
		IN (DAT_COUNT *) (recvdispls), \
		IN (DAT_CONTEXT) (user_context), \
		IN (DAT_COMPLETION_FLAGS) (flags))

/*
 * This call combines the elements of the data type specified
 * by data_type from the buffer specified by send_buffer of all members of
 * the collective by performing the operation specified by reduce_operation
 * and placing the result into the buffer of the root member specified by
 * recv_buffer. It is an error to specify a floating point type with
 * any of the logical reduction operators.When using the REDUCE_OP_MINLOC
 * and REDUCE_OP _MAXLOC operations, it is assumed that the input and output
 * buffers contain pair values where the first member of the pair is of the
 * type specified by data_type followed by a COLLECTIVE_TYPE_UINT32 type.
 * When the reduction is complete, the receive buffer will contain the
 * MIN/MAX value in the first member of the pair with the first member rank
 * that contained it in the second member of the pair.  The tables below
 * show the result of a REDUCE_OP_SUM reduce operation.
 */
#define dat_ib_collective_reduce(coll_handle, sendbuf, sendsize, recvbuf, recvsize, op, type, root, user_context, flags) \
	dat_extension_op( \
		IN (DAT_IB_COLLECTIVE_HANDLE)(coll_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_REDUCE_OP, \
		IN (DAT_PVOID) (sendbuf), \
		IN (DAT_COUNT) (sendsize), \
		IN (DAT_PVOID) (recvbuf), \
		IN (DAT_COUNT) (recvsize), \
		IN (DAT_IB_COLLECTIVE_REDUCE_DATA_OP) (op), \
		IN (DAT_IB_COLLECTIVE_DATA_TYPE) (type), \
		IN (DAT_IB_COLLECTIVE_RANK) (root), \
		IN (DAT_CONTEXT) (user_context), \
		IN (DAT_COMPLETION_FLAGS) (flags))

/*
 * This call is identical to the dat_collective_reduce()
 * call with the exception that the recv_buffer and recv_byte_count arguments
 * are valid for all members of the collective and all members of will
 * receive the reduction results.
 */
#define dat_ib_collective_allreduce(coll_handle, sendbuf, sendsize, recvbuf, recvsize, op, type, user_context, flags) \
	dat_extension_op( \
		IN (DAT_IB_COLLECTIVE_HANDLE) (coll_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_ALLREDUCE_OP, \
		IN (DAT_PVOID) (sendbuf), \
		IN (DAT_COUNT) (sendsize), \
		IN (DAT_PVOID) (recvbuf), \
		IN (DAT_COUNT) (recvsize), \
		IN (DAT_IB_COLLECTIVE_REDUCE_DATA_OP) (op), \
		IN (DAT_IB_COLLECTIVE_DATA_TYPE) (type), \
		IN (DAT_CONTEXT) (user_context), \
		IN (DAT_COMPLETION_FLAGS) (flags))
/*
 * This call is identical to rank 0 of the collective calling
 * this dat_collective_reduce() followed by dat_collective_scatterv().
 * The number of bytes received in the scatter for each rank is determined
 * by rank offset into the recv_byte_counts array.
 */
#define dat_ib_collective_reduce_scatter(coll_handle, sendbuf, sendsize, recvbuf, recvsizes, op, type, user_context, flags) \
	dat_extension_op( \
		IN (DAT_IB_COLLECTIVE_HANDLE) (coll_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_REDUCE_SCATTER_OP, \
		IN (DAT_PVOID) (sendbuf), \
		IN (DAT_COUNT) (sendsize), \
		IN (DAT_PVOID) (recvbuf), \
		IN (DAT_COUNT *) (recvsizes), \
		IN (DAT_IB_COLLECTIVE_REDUCE_DATA_OP) (op), \
		IN (DAT_IB_COLLECTIVE_DATA_TYPE) (type), \
		IN (DAT_CONTEXT) (user_context), \
		IN (DAT_COMPLETION_FLAGS) (flags))

/*
 * This call is used to perform a prefix reduction on data
 * distributed across the group. The operation returns, in recv_buffer of
 * the member with rank i, the reduction of the values in send_buffer of
 * members with ranks 0,...,i (inclusive). The tables below show the
 * result of a REDUCE_OP_SUM scan operation.
 */
#define dat_ib_collective_scan(coll_handle, sendbuf, sendsize, recvbuf, recvsize, op, type, user_context, flags) \
	dat_extension_op( \
		IN (DAT_IB_COLLECTIVE_HANDLE) (coll_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_SCAN_OP, \
		IN (DAT_PVOID) (sendbuf), \
		IN (DAT_COUNT) (sendsize), \
		IN (DAT_PVOID) (recvbuf), \
		IN (DAT_COUNT) (recvsize), \
		IN (DAT_IB_COLLECTIVE_REDUCE_DATA_OP) (op), \
		IN (DAT_IB_COLLECTIVE_DATA_TYPE) (type), \
		IN (DAT_CONTEXT) (user_context), \
		IN (DAT_COMPLETION_FLAGS) (flags))

/*
 * This call performs a broadcast send operation that transfers
 * data specified by the buffer argument of the root into the buffer argument
 * of all other Endpoints in the collective group specified by coll_handle.
 * The operation is completed on the collective EVD unless completions are
 * suppressed through the completion flags.  All broadcasts are considered
 * �in place� transfers.  The tables below show the result of a broadcast
 * operation.
 */
#define dat_ib_collective_broadcast(coll_handle, buf, size, root, user_context, flags) \
	dat_extension_op(\
		IN (DAT_IB_COLLECTIVE_HANDLE) (coll_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_BROADCAST_OP, \
		IN (DAT_PVOID) (buf), \
		IN (DAT_COUNT) (size), \
		IN (DAT_IB_COLLECTIVE_RANK) (root), \
		IN (DAT_CONTEXT) (user_context), \
		IN (DAT_COMPLETION_FLAGS) (flags))

/*
 * This call will synchronize all endpoints of the collective
 * group specified by coll_handle. This is an asynchronous call that
 * will post a completion to the collective EVD when all endpoints
 * have synchronized.
 */
#define dat_ib_collective_barrier(coll_handle, user_context, flags) \
	dat_extension_op( \
		IN (DAT_IB_COLLECTIVE_HANDLE) (coll_handle), \
		IN (DAT_IB_OP) DAT_IB_COLLECTIVE_BARRIER_OP, \
		IN (DAT_CONTEXT) (user_context), \
		IN (DAT_COMPLETION_FLAGS) (flags))


/* Backward compatibility */
#define DAT_ATTR_COUNTERS DAT_IB_ATTR_COUNTERS
#define dat_query_counters dat_ib_query_counters	
#define dat_print_counters dat_ib_print_counters
#define DAT_QUERY_COUNTERS_OP DAT_IB_QUERY_COUNTERS_OP
#define DAT_PRINT_COUNTERS_OP DAT_IB_PRINT_COUNTERS_OP

#endif /* _DAT_IB_EXTENSIONS_H_ */

