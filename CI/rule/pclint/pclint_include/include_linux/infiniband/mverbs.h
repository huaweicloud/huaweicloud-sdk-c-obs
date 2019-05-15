/*
 * Copyright (c) 2009-2010 Mellanox Technologies.  All rights reserved.
 */

#ifndef INFINIBAND_MVERBS_H
#define INFINIBAND_MVERBS_H

#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h> /* for bool */

#include <infiniband/verbs.h>

#ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS   }
#else /* !__cplusplus */
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif /* __cplusplus */

#if __GNUC__ >= 3
#  define __attribute_const __attribute__((const))
#else
#  define __attribute_const
#endif


BEGIN_C_DECLS

#define IBV_MVERBS_EXT 1

#define IBV_M_MAGIC ((void *)0x032C6D7E)

#define FLOAT64 double

enum ibv_m_wr_data_type {
	IBV_M_DATA_TYPE_INT8 = 0,
	IBV_M_DATA_TYPE_INT16,
	IBV_M_DATA_TYPE_INT32,
	IBV_M_DATA_TYPE_INT64,
	IBV_M_DATA_TYPE_INT128,
	IBV_M_DATA_TYPE_FLOAT32,
	IBV_M_DATA_TYPE_FLOAT64,
	IBV_M_DATA_TYPE_FLOAT96,
	IBV_M_DATA_TYPE_FLOAT128,
	IBV_M_DATA_TYPE_COMPLEX,
	IBV_M_DATA_TYPE_INVALID		/* Keep Last */
};

enum ibv_m_wr_calc_op {
	IBV_M_CALC_OP_LXOR = 0,
	IBV_M_CALC_OP_BXOR,
	IBV_M_CALC_OP_LOR,
	IBV_M_CALC_OP_BOR,
	IBV_M_CALC_OP_LAND,
	IBV_M_CALC_OP_BAND,
	IBV_M_CALC_OP_ADD,
	IBV_M_CALC_OP_MAX,
	IBV_M_CALC_OP_MIN,		/* Keep after MAX */
	IBV_M_CALC_OP_MAXLOC,
	IBV_M_CALC_OP_MINLOC,		/* Keep after MAXLOC */
	IBV_M_CALC_OP_PROD,
	IBV_M_CALC_OP_INVALID		/* Keep Last */
};

#define IBV_WR_VENDOR_GAP  0x20

enum ibv_m_wr_opcode {
	IBV_M_WR_SEND_ENABLE = IBV_WR_VENDOR_GAP,
	IBV_M_WR_RECV_ENABLE,
	IBV_M_WR_CQE_WAIT,
	IBV_M_WR_CALC, /* the value of IBV_M_WR_CALC is deprecated */
	IBV_M_WR_CALC_SEND = IBV_M_WR_CALC
};

struct ibv_m_send_wr {
	uint64_t		wr_id;
	struct ibv_send_wr	*next;
	struct ibv_sge		*sg_list;
	int			num_sge;
	enum ibv_m_wr_opcode	opcode;
	enum ibv_send_flags	send_flags;
	uint32_t		imm_data;	/* in network byte order */
	union {
		struct {
			uint64_t	remote_addr;
			uint32_t	rkey;
		} rdma;
		struct {
			uint64_t	remote_addr;
			uint64_t	compare_add;
			uint64_t	swap;
			uint32_t	rkey;
		} atomic;
		struct {
			struct ibv_ah	*ah;
			uint32_t	remote_qpn;
			uint32_t	remote_qkey;
		} ud;
		struct {
			enum ibv_m_wr_calc_op	calc_op;
			enum ibv_m_wr_data_type	data_type;
		} calc;
		struct {
			struct ibv_cq	*cq;
			uint32_t	cq_count;
		} cqe_wait;
		struct {
			struct ibv_qp	*qp;
			uint32_t	wqe_count;
		} wqe_enable;
	} wr;
};

enum {
	IBV_M_WQE_SQ_ENABLE_CAP = 0x01,
	IBV_M_WQE_RQ_ENABLE_CAP = 0x02,
	IBV_M_WQE_CQE_WAIT_CAP  = 0x04,
	IBV_M_WQE_CALC_CAP	= 0x08
};

struct ibv_m_dev_attr {
	struct ibv_device_attr ibv_dev_attr;
	uint64_t m_device_cap_flags;
};

struct ibv_m_context_ops {
	int (*query_device)(struct ibv_context *, struct ibv_m_dev_attr *);
	int (*post_send)(struct ibv_qp *, struct ibv_m_send_wr *,
				struct ibv_m_send_wr **);
	int (*query_calc_cap)(struct ibv_context *, enum ibv_m_wr_calc_op,
				enum ibv_m_wr_data_type, int *, int *);
	void (*ring_doorbell)(struct ibv_qp *);
};

struct ibv_m_context {
	struct ibv_context   v_ctx;
	struct ibv_m_context_ops m_ops;
};


/**
 * ibv_query_device - Get device properties
 */
int ibv_m_query_device(struct ibv_context *context,
		       struct ibv_m_dev_attr *device_attr);

/**
 * ibv_m_dev_query_calc_cap
 *
 * return true if calc_op is supported by the device for the specific data type
 * false otherwise.
 */
int ibv_m_query_calc_cap(struct ibv_context *context,
			 enum ibv_m_wr_calc_op calc_op,
			 enum ibv_m_wr_data_type data_type,
			 int *operands_per_gather,
			 int *max_num_operands);

/**
 * ibv_m_post_send - Post a list of work requests to a send queue.
 *
 * If IBV_SEND_INLINE flag is set, the data buffers can be reused
 * immediately after the call returns.
 */
int ibv_m_post_send(struct ibv_qp *qp, struct ibv_m_send_wr *wr,
		    struct ibv_m_send_wr **bad_wr);

/**
 * ibv_m_ring_doorbell - Ring the door bell for the queue
 *
 * Used for optimizations
 */
void ibv_m_ring_doorbell(struct ibv_qp *qp);

enum ibv_m_qp_attr_mask {
	IBV_M_QP_EXT_IGNORE_SQ_OVERFLOW = 1 << 26,
	IBV_M_QP_EXT_IGNORE_RQ_OVERFLOW = 1 << 27,
	IBV_M_QP_EXT_CLASS_1 = 1 << 28,
	IBV_M_QP_EXT_CLASS_2 = 1 << 29,
	IBV_M_QP_EXT_CLASS_3 = 1 << 30,
};

static inline int ibv_m_modify_qp(struct ibv_qp *qp, struct ibv_qp_attr *attr,
				  uint32_t attr_mask) {
	return ibv_modify_qp(qp, attr, attr_mask);
}

enum ibv_m_device_cap_flags {
	IB_DEVICE_COLL_OFFLOAD = (1 << 31)
};

/*
 * Sometimes due to hardware limitations we need to change the data in the user
 * buffer in order to perform the calculations.
 * E.g.,ConnectX B0 supports only calculation of MIN. In order to perform MAX we
 * need to perform negation on each number, perform MIN, and do a negation on
 * the result.
 *
 * There are two functions: pack_data_for_calc and unpack_data_from_calc that
 * are being used to perform the needed changes on the buffer so the calcution
 * can done by the offload mechanism.
 */

/* pack_data_for_calc - modify the format of the data read from the source
 * buffer so calculation can be done on it.
 * The output data size will always be 64bit and the output data format will
 * either be int64_t (for integer input) or double (for floating point).
 * The function may also modify the operation, to match the modified data.
 * All the parameters are input params, unless stated otherwise.
 * @op - calc operation to perform
 * @type - input data type
 * @host_buffer_in_net_order - 	'true' if the data in the input (host) buffer is
 *				already in in network byte order.
 *				'false' - if not.
 * @host_buffer - input buffer
 * @id - will be set in the buffer, in case of MINLOC/MAXLOC operations
 * @out_op - out param. the operation that will be performed
 * @out_type - out param. the data type that will be used to perform the operation
 * @network_buffer - out param. will contain the formated data. It's size is
 * 		     assumed to be 128bit for MINLOC/MAXLOC operations and
 * 		     64bit for all other operations
 *		     Must be 16B aligned
 */
int pack_data_for_calc(struct ibv_context *context, enum ibv_m_wr_calc_op op,
		       enum ibv_m_wr_data_type type,
		       bool host_buffer_in_net_order, const void *host_buffer,
		       uint64_t id, enum ibv_m_wr_calc_op *out_op,
		       enum ibv_m_wr_data_type *out_type, void *network_buffer);

/* unpack_data_from_calc - modify the format of the data read from the network
 * to the format in which the host expects it.
 * The function may also modify the operation, to match the modified data.
 * All the parameters are input params, unless stated otherwise.
 * @op - calc operation to perform
 * @type - expected output (host) data type
 * @host_buffer_in_net_order -  'true' if the data in the output (host) buffer is
 *                              expected to be in in network byte order.
 *                              'false' - if not.
 * @network_buffer - input buffer.
 * 		     The size of the buffer is assumed to be 128bit for
		     MINLOC/MAXLOC operation and 64bit for all other operations
 * @id - will be returned in case of MINLOC/MAXLOC operations
 * @out_op - out param. the operation that was actually performed
 * @host_buffer - out param. will contain the formated data.
 *		  The data format will be set according to the 'type' and the
 * 		  buffer is assumed to be big enough to contain this data.
 */
int unpack_data_from_calc(struct ibv_context *context, enum ibv_m_wr_calc_op op,
			  enum ibv_m_wr_data_type type,
			  bool host_buffer_in_net_order,
			  const void *network_buffer,
			  uint64_t *id, void *host_buffer);

END_C_DECLS

#  undef __attribute_const

#endif /* INFINIBAND_MVERBS_H */
