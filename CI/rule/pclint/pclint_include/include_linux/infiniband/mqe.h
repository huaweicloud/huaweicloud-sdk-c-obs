/*
 * Copyright (c) 2009-2010 Mellanox Technologies.  All rights reserved.
 */

#ifndef H_MQE_H
#define H_MQE_H

#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include <infiniband/mverbs.h>

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


enum mqe_mwr_opcode {
	MQE_WR_SEND,
	MQE_WR_RECV,
	MQE_WR_CQE_WAIT,
	MQE_WR_EXEC_SEND,
	MQE_WR_EXEC_RECV
};

struct mqe_context_attr {
	void		*mq_context;
	uint32_t	max_mqe_tasks; /* max number of tasks in a mqe */
	uint32_t	max_mq_size;   /* max number of entries */
	struct		ibv_cq *cq;    /* pointer to user defined CQ,
					If user specify NULL mqe will
					create own private CQ	*/
};

enum mqe_wr_flags {
	MQE_WR_FLAG_IMM_EXE  = 1 << 0x00,
	MQE_WR_FLAG_SIGNAL   = 1 << 0x01,
	MQE_WR_FLAG_BLOCK    = 1 << 0x03
};

enum mqe_wc_status {
	MWQ_WC_OK,
	MWQ_WC_ERROR
};

struct mqe_wc {
	uint64_t		wr_id;
	enum mqe_wc_status	status;
};

struct mqe_qp_entry {
	struct	ibv_qp 		*qp;
	struct 	mqe_qp_entry 	*next;
};

struct mqe_task {
	enum mqe_mwr_opcode	opcode;

	uint64_t		wr_id;
	enum mqe_wr_flags  	flags;

	union {
		struct ibv_post_mwr_task {
			struct ibv_qp 		*qp;
			union {
				struct ibv_m_send_wr  *send_wr;
				struct ibv_recv_wr    *recv_wr;
			};
		} post;

		struct ibv_wait_mwr_task {
			int 			count;
			struct ibv_cq       	*cq;
			struct mqe_qp_entry 	*mqe_qp;
		} wait;

		struct ibv_exec_qp_task {
			struct ibv_qp 		*qp;
			int 	       		count;
		} exec_qp;
	};

	struct mqe_task 	*next;
};

struct mqe_context *mqe_context_create(struct ibv_context *ibv_ctx,
				       struct ibv_pd *pd,
				       struct mqe_context_attr *attr);

int mqe_context_destroy(struct mqe_context *ctx);

int mqe_post_task(struct mqe_context *ctx, struct mqe_task *task_list,
		  struct mqe_task **bad_task);

int mqe_poll_context(struct mqe_context *ctx, struct mqe_wc *wc);



END_C_DECLS

#  undef __attribute_const

#endif /* H_MQE_H */
