/*
 * Copyright (c) 2006-2010. QLogic Corporation. All rights reserved.
 * Copyright (c) 2003-2006, PathScale, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
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
 */

#ifndef PSM_MQ_H
#define PSM_MQ_H

#ifdef __cplusplus
extern "C" {
#endif



/* Initialize the MQ component for MQ communication
 *
 * This function provides the Matched Queue handle necessary to performa all
 * Matched Queue communication operations.  
 *
 * [in] ep Endpoint over which to initialize Matched Queue
 * [in] tag_order_mask Order mask hint to let MQ know what bits of the send
 *                           tag are required to maintain MQ message order.  In
 *                           MPI parlance, this mask sets the bits that store
 *                           the context (or communicator ID).  The user can
 *                           choose to pass PSM_MQ_ORDERMASK_NONE or
 *                           PSM_MQ_ORDERMASK_ALL to tell MQ to respectively
 *                           provide no ordering guarantees or to provide
 *                           ordering over all messages by ignoring the
 *                           contexts of the send tags.
 * [in] opts Set of options for Matched Queue
 * [in] numopts Number of options passed
 * [out] mq User-supplied storage to return the Matched Queue handle
 *                associated to the newly created Matched Queue.
 *
 * @remark This function can be called many times to retrieve the MQ handle
 *         associated to an endpoint, but options are only considered the first
 *         time the function is called.
 *
 * [post] The user obtains a handle to an instantiated Match Queue.  
 *
 * The following error code is returned.  Other errors are handled by the PSM
 * error handler (psm_error_register_handler).
 *
 * [retval] PSM_OK A new Matched Queue has been instantiated across all the
 *         members of the group.
 *
 */
psm_error_t
psm_mq_init(psm_ep_t ep, uint64_t tag_order_mask, 
	    const struct psm_optkey *opts, int numopts, psm_mq_t *mq);

#define PSM_MQ_ORDERMASK_NONE	0ULL
	/* Used to initialize MQ and disable all MQ message ordering
	 * guarantees (this mask may prevent the use of MQ to maintain matched
	 * message envelope delivery required in MPI). */

#define PSM_MQ_ORDERMASK_ALL	0xffffffffffffffffULL
	/* Used to initialize MQ with no message ordering hints, which forces
	 * MQ to maintain order over all messages */

/* Finalize (close) an MQ handle
 *
 * The following error code is returned.  Other errors are handled by the PSM
 * error handler (psm_error_register_handler).
 *
 * [retval] PSM_OK A given Matched Queue has been freed and use of the future
 * use of the handle produces undefined results.
 */
psm_error_t
psm_mq_finalize(psm_mq_t mq);

/* MQ Non-blocking operation status
 *
 * Message completion status for asynchronous communication operations.
 * For wait and test functions, MQ fills in the structure upon completion.
 * Upon completion, receive requests fill in every field of the status
 * structure while send requests only return a valid error_code and context
 * pointer.
 */
typedef
struct psm_mq_status {   
    uint64_t msg_tag;    /* Sender's original message tag (receive reqs only) */
    uint32_t msg_length; /* Sender's original message length (receive reqs only) */
    uint32_t nbytes;	 /* Actual number of bytes transfered (receive reqs only) */
    psm_error_t error_code; /* MQ error code for communication operation */
    void     *context;   /* User-associated context for send or receive */
}
psm_mq_status_t;

/* PSM Communication handle (opaque) */
typedef struct psm_mq_req *psm_mq_req_t;



/* Get an MQ option (Deprecated. Use psm_getopt with PSM_COMPONENT_MQ)
 *
 * Function to retrieve the value of an MQ option.
 *
 * [in] mq Matched Queue handle
 * [in] option Index of option to retrieve.  Possible values are:
 *            * PSM_MQ_RNDV_IPATH_SZ
 *            * PSM_MQ_RNDV_SHM_SZ
 *            * PSM_MQ_MAX_SYSBUF_MBYTES
 *
 * [in] value Pointer to storage that can be used to store the value of
 *            the option to be set.  It is up to the user to ensure that the
 *            pointer points to a memory location large enough to accomodate
 *            the value associated to the type.  Each option documents the size
 *            associated to its value.
 *
 * [returns] PSM_OK if option could be retrieved.
 * [returns] PSM_PARAM_ERR if the option is not a valid option number
 */
psm_error_t
psm_mq_getopt(psm_mq_t mq, int option, void *value);

/* Set an MQ option (Deprecated. Use psm_setopt with PSM_COMPONENT_MQ)
 *
 * Function to set the value of an MQ option.
 *
 * [in] mq Matched Queue handle
 * [in] option Index of option to retrieve.  Possible values are:
 *            * PSM_MQ_RNDV_IPATH_SZ
 *            * PSM_MQ_RNDV_SHM_SZ
 *            * PSM_MQ_MAX_SYSBUF_MBYTES
 *
 * [in] value Pointer to storage that contains the value to be updated
 *                  for the supplied option number.  It is up to the user to
 *                  ensure that the pointer points to a memory location with a
 *                  correct size.
 *
 * [returns] PSM_OK if option could be retrieved.
 * [returns] PSM_PARAM_ERR if the option is not a valid option number
 * [returns] PSM_OPT_READONLY if the option to be set is a read-only option
 *                           (currently no MQ options are read-only).
 */
psm_error_t
psm_mq_setopt(psm_mq_t mq, int option, const void *value);



#define PSM_MQ_FLAG_SENDSYNC	0x01 
				/* MQ Send Force synchronous send */

#define PSM_MQ_REQINVALID	((psm_mq_req_t)(NULL)) 
				/* MQ request completion value */

/* Post a receive to a Matched Queue with tag selection criteria
 *
 * Function to receive a non-blocking MQ message by providing a preposted
 * buffer. For every MQ message received on a particular MQ, the tag and @c
 * tagsel parameters are used against the incoming message's send tag as
 * described in tagmatch.
 *
 * [in] mq Matched Queue Handle
 * [in] rtag Receive tag
 * [in] rtagsel Receive tag selector
 * [in] flags Receive flags (None currently supported)
 * [in] buf Receive buffer 
 * [in] len Receive buffer length
 * [in] context User context pointer, available in psm_mq_status_t
 *                    upon completion
 * [out] req PSM MQ Request handle created by the preposted receive, to
 *                 be used for explicitly controlling message receive
 *                 completion.
 *
 * [post] The supplied receive buffer is given to MQ to match against incoming
 *       messages unless it is cancelled via psm_mq_cancel @e before any
 *       match occurs.
 *
 * The following error code is returned.  Other errors are handled by the PSM
 * error handler (psm_error_register_handler).
 *
 * [retval] PSM_OK The receive buffer has successfully been posted to the MQ.
 */
psm_error_t
psm_mq_irecv(psm_mq_t mq, uint64_t rtag, uint64_t rtagsel, uint32_t flags,
	     void *buf, uint32_t len, void *context, psm_mq_req_t *req);

/* Send a blocking MQ message
 *
 * Function to send a blocking MQ message, whereby the message is locally
 * complete and the source data can be modified upon return.
 *
 * [in] mq Matched Queue Handle
 * [in] dest Destination EP address
 * [in] flags Message flags, currently:
 *            * PSM_MQ_FLAG_SENDSYNC tells PSM to send the message
 *            synchronously, meaning that the message will not be sent until
 *            the receiver acknowledges that it has matched the send with a
 *            receive buffer.
 * [in] stag Message Send Tag
 * [in] buf Source buffer pointer
 * [in] len Length of message starting at buf.
 *
 * [post] The source buffer is reusable and the send is locally complete.
 *
 * @note This send function has been implemented to best suit MPI_Send.
 *
 * The following error code is returned.  Other errors are handled by the PSM
 * error handler (psm_error_register_handler).
 *
 * [retval] PSM_OK The message has been successfully sent.
 */
psm_error_t
psm_mq_send(psm_mq_t mq, psm_epaddr_t dest, uint32_t flags, uint64_t stag, 
	    const void *buf, uint32_t len);

/* Send a non-blocking MQ message
 *
 * Function to initiate the send of a non-blocking MQ message, whereby the
 * user guarantees that the source data will remain unmodified until the send
 * is locally completed through a call such as psm_mq_wait or @ref
 * psm_mq_test.
 *
 * [in] mq Matched Queue Handle
 * [in] dest Destination EP address
 * [in] flags Message flags, currently:
 *            * PSM_MQ_FLAG_SENDSYNC tells PSM to send the message
 *            synchronously, meaning that the message will not be sent until
 *            the receiver acknowledges that it has matched the send with a
 *            receive buffer.
 * [in] stag Message Send Tag
 * [in] buf Source buffer pointer
 * [in] len Length of message starting at buf.
 * [in] context Optional user-provided pointer available in @ref
 *                    psm_mq_status_t when the send is locally completed.
 * [out] req PSM MQ Request handle created by the non-blocking send, to
 *                 be used for explicitly controlling message completion.
 *
 * [post] The source buffer is not reusable and the send is not locally complete
 *       until its request is completed by either psm_mq_test or @ref
 *       psm_mq_wait.
 *
 * @note This send function has been implemented to suit MPI_Isend.
 *
 * The following error code is returned.  Other errors are handled by the PSM
 * error handler (psm_error_register_handler).
 *
 * [retval] PSM_OK The message has been successfully initiated.
 *
 */
psm_error_t
psm_mq_isend(psm_mq_t mq, psm_epaddr_t dest, uint32_t flags, uint64_t stag, 
	     const void *buf, uint32_t len, void *context, psm_mq_req_t *req);

/* Try to Probe if a message is received to match tag selection
 * criteria
 *
 * Function to verify if a message matching the supplied tag and tag selectors
 * has been received.  The function is not fully matched until the user
 * provides a buffer with the successfully matching tag selection criteria
 * through psm_mq_irecv.
 * Probing for messages may be useful if the size of the
 * message to be received is unknown, in which case its size will be
 * available in the msg_length member of the returned status.
 *
 * [in] mq Matched Queue Handle
 * [in] rtag Message receive tag
 * [in] rtagsel Message receive tag selector
 * [out] status Upon return, status is filled with information
 *                    regarding the matching send.
 *
 * The following error codes are returned.  Other errors are handled by the PSM
 * error handler (psm_error_register_handler).
 *
 * [retval] PSM_OK The iprobe is successful and status is updated if non-NULL.
 * [retval] PSM_MQ_NO_COMPLETIONS The iprobe is unsuccessful and status is unchaged.
 */
psm_error_t
psm_mq_iprobe(psm_mq_t mq, uint64_t rtag, uint64_t rtagsel, 
		   psm_mq_status_t *status);

/* Query for non-blocking requests ready for completion.
 *
 * Function to query a particular MQ for non-blocking requests that are ready
 * for completion.  Requests "ready for completion" are not actually considered
 * complete by MQ until they are returned to the MQ library through @ref
 * psm_mq_wait or psm_mq_test.
 *
 * If the user can deal with consuming request completions in the order in
 * which they complete, this function can be used both for completions and for
 * ensuring progress.  The latter requirement is satisfied when the user
 * peeks an empty completion queue as a side effect of always aggressively
 * peeking and completing all an MQ's requests ready for completion.
 *
 * 
 * [in] mq Matched Queue Handle
 * [in,out] req MQ non-blocking request
 * [in] status Optional MQ status, can be NULL.
 *
 * [post] The user has ensured progress if the function returns @ref
 *       PSM_MQ_NO_COMPLETIONS
 *
 * The following error codes are returned.  Other errors are handled by the PSM
 * error handler (psm_error_register_handler).
 *
 * [retval] PSM_OK The peek is successful and req is updated with a request
 *                ready for completion.  If status is non-NULL, it is also
 *                updated.
 *
 * [retval] PSM_MQ_NO_COMPLETIONS The peek is not successful, meaning that there are
 *                            no further requests ready for completion.  The
 *                            contents of req and status remain
 *                            unchanged.
 */
psm_error_t
psm_mq_ipeek(psm_mq_t mq, psm_mq_req_t *req, psm_mq_status_t *status);

/* Wait until a non-blocking request completes
 *
 * Function to wait on requests created from either preposted receive buffers
 * or non-blocking sends.  This is the only blocking function in the MQ
 * interface and will poll until the request is complete as per the progress
 * semantics explained in mq_progress.
 *
 * [in,out] request MQ non-blocking request
 * [out] status Updated if non-NULL when request successfully completes
 *
 * [pre] The user has obtained a valid MQ request by calling psm_mq_isend
 *      or psm_mq_irecv and passes a pointer to enough storage to write
 *      the output of a psm_mq_status_t or NULL if status is to be
 *      ignored.  
 *
 * [pre] Since MQ will internally ensure progress while the user is
 *      suspended, the user need not ensure that progress is made prior to
 *      calling this function.
 *
 * [post] The request is assigned the value PSM_MQ_REQINVALID and all
 *       associated MQ request storage is released back to the MQ library.
 *       
 * [remarks] 
 *  * This function ensures progress on the endpoint as long as the request
 *      is incomplete.
 *  * status can be NULL, in which case no status is written upon
 *      completion.
 *  * If request is PSM_MQ_REQINVALID, the function returns
 *      immediately.
 *
 * The following error code is returned.  Other errors are handled by the PSM
 * error handler (psm_error_register_handler).
 *
 * [retval] PSM_OK The request is complete or the value of was 
 *                PSM_MQ_REQINVALID.
 *
 */
psm_error_t
psm_mq_wait(psm_mq_req_t *request, psm_mq_status_t *status);

/* Test if a non-blocking request is complete
 *
 * Function to test requests created from either preposted receive buffers or
 * non-blocking sends for completion.  Unlike psm_mq_wait, this function
 * tests request for completion and @e never ensures progress directly or
 * indirectly.  It is up to the user to employ some of the progress functions
 * described in mq_progress to ensure progress if the user chooses to
 * exclusively test requests for completion.
 *
 * Testing a request for completion @e never internally ensure progress in
 * order to be useful to construct higher-level completion tests over arrays to
 * test some, all or any request that has completed.  For testing arrays of
 * requests, it is preferable for performance reasons to only ensure progress
 * once before testing a set of requests for completion.
 *
 * [in,out] request MQ non-blocking request
 * [out] status Updated if non-NULL and the request successfully
 * completes
 *
 * [pre] The user has obtained a valid MQ request by calling psm_mq_isend
 *      or psm_mq_irecv and passes a pointer to enough storage to write
 *      the output of a psm_mq_status_t or NULL if status is to be
 *      ignored.  
 * 
 * [pre] The user has ensured progress on the Matched Queue if @ref
 *      psm_mq_test is exclusively used for guaranteeing request completions.
 *
 * [post] If the request is complete, the request is assigned the value @ref
 *       PSM_MQ_REQINVALID and all associated MQ request storage is released
 *       back to the MQ library. If the request is incomplete, the contents of
 *       request is unchanged.
 *
 * [post] The user will ensure progress on the Matched Queue if @ref
 *       psm_mq_test is exclusively used for guaranteeing request completions.
 *
 * The following two errors are always returned.  Other errors are handled by
 * the PSM error handler (psm_error_register_handler).
 *
 * [retval] PSM_OK The request is complete and request is set to @ref
 *                PSM_MQ_REQINVALID or the value of was PSM_MQ_REQINVALID
 *
 * [retval] PSM_MQ_NO_COMPLETIONS The request is not complete and request is
 *                           unchanged.
 *
 */
psm_error_t
psm_mq_test(psm_mq_req_t *request, psm_mq_status_t *status);

/* Cancel a preposted request
 *
 * Function to cancel a preposted receive request returned by @ref
 * psm_mq_irecv.  It is currently illegal to cancel a send request initiated
 * with psm_mq_isend.
 *
 * [pre] The user has obtained a valid MQ request by calling psm_mq_isend
 *      or psm_mq_irecv and passes a pointer to enough storage to write
 *      the output of a psm_mq_status_t or NULL if status is to be
 *      ignored.  
 *
 * [post] Whether the cancel is successful or not, the user returns the
 *       request to the library by way of psm_mq_test or @ref
 *       psm_mq_wait.
 * 
 * Only the two following errors can be returned directly, without being
 * handled by the error handler (psm_error_register_handler):
 *
 * [retval] PSM_OK The request could be successfully cancelled such that the
 *                preposted receive buffer could be removed from the preposted
 *                receive queue before a match occured. The associated @c
 *                request remains unchanged and the user must still return
 *                the storage to the MQ library.
 *
 * [retval] PSM_MQ_NO_COMPLETIONS The request could not be successfully cancelled
 *                           since the preposted receive buffer has already
 *                           matched an incoming message.  The request
 *                           remains unchanged.
 *
 */
psm_error_t
psm_mq_cancel(psm_mq_req_t *req);

struct psm_mq_stats {
    uint64_t	rx_user_bytes;/* Bytes received into a matched user buffer */
    uint64_t	rx_user_num;  /* Messages received into a matched user buffer */
    uint64_t	rx_sys_bytes; /* Bytes received into an unmatched system buffer */
    uint64_t	rx_sys_num;   /* Messages received into an unmatched system buffer */

    uint64_t	tx_num;         /* Total Messages transmitted (shm and ipath) */
    uint64_t	tx_eager_num;   /* Messages transmitted eagerly */
    uint64_t	tx_eager_bytes; /* Bytes transmitted eagerly */
    uint64_t	tx_rndv_num;    /* Messages transmitted using expected TID mechanism */
    uint64_t	tx_rndv_bytes;  /* Bytes transmitted using expected TID mechanism */
    uint64_t	tx_shm_num;     /* Messages transmitted (shm only) */
    uint64_t	rx_shm_num;     /* Messages received through shm */

    uint64_t	rx_sysbuf_num;   /* Number of system buffers allocated  */
    uint64_t	rx_sysbuf_bytes; /* Bytes allcoated for system buffers */

    uint64_t	_reserved[16];	 /* Internally reserved for future use */
};

#define PSM_MQ_NUM_STATS    13	/* How many stats are currently used in psm_mq_stats */

typedef struct psm_mq_stats	   psm_mq_stats_t;

/* Retrieve statistics from an instantied MQ */
void 
psm_mq_get_stats(psm_mq_t mq, psm_mq_stats_t *stats);


#ifdef __cplusplus
}				/* extern "C" */
#endif
#endif
