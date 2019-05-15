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

#ifndef PSM_H
#define PSM_H

#ifdef __cplusplus
extern "C" {
#endif





/* Local endpoint handle (opaque)
 *  
 *
 * Handle returned to the user when a new local endpoint is created.  The
 * handle is a local handle to be used in all communication functions and is
 * not intended to globally identify the opened endpoint in any way.  
 *
 * All open endpoint handles can be globally identified using the endpoint id
 * integral type (psm_epid_t) and all communication must use an endpoint
 * address (psm_epaddr_t) that can be obtained by connecting a local
 * endpoint to one or more endpoint identifiers.
 *
 * @remark The local endpoint handle is opaque to the user.  */
typedef struct psm_ep *psm_ep_t;

/* MQ handle (opaque)
 * 
 *
 * Handle returned to the user when a new Matched queue is created (@ref
 * psm_mq_init).  */
typedef struct psm_mq *psm_mq_t;

#define PSM_VERNO       0x010e 
#define PSM_VERNO_MAJOR 0x01   
#define PSM_VERNO_MINOR 0x0e   

enum psm_error {
    
    PSM_OK = 0,
    
    PSM_OK_NO_PROGRESS = 1,
    
    PSM_PARAM_ERR = 3,
    
    PSM_NO_MEMORY = 4,
    
    PSM_INIT_NOT_INIT = 5, 
    
    PSM_INIT_BAD_API_VERSION = 6,
    
    PSM_NO_AFFINITY = 7,
    
    PSM_INTERNAL_ERR = 8,
    
    PSM_SHMEM_SEGMENT_ERR = 9,
    
    PSM_OPT_READONLY = 10,
    
    PSM_TIMEOUT = 11,
    
    PSM_TOO_MANY_ENDPOINTS = 12,

    
    PSM_IS_FINALIZED = 13,

    
    PSM_EP_WAS_CLOSED = 20, 
    
    PSM_EP_NO_DEVICE = 21,
    
    PSM_EP_UNIT_NOT_FOUND = 22,
    
    PSM_EP_DEVICE_FAILURE = 23, 
    
    PSM_EP_CLOSE_TIMEOUT = 24,  
    
    PSM_EP_NO_PORTS_AVAIL = 25, 
    
    PSM_EP_NO_NETWORK = 26,  
    
    PSM_EP_INVALID_UUID_KEY = 27,
    
    PSM_EP_NO_RESOURCES = 28,

    
    PSM_EPID_UNKNOWN = 40,
    
    PSM_EPID_UNREACHABLE = 41,
    
    PSM_EPID_INVALID_NODE = 43,
    
    PSM_EPID_INVALID_MTU =  44,
    
    PSM_EPID_INVALID_UUID_KEY = 45,
    
    PSM_EPID_INVALID_VERSION = 46,
    
    PSM_EPID_INVALID_CONNECT = 47,
    
    PSM_EPID_ALREADY_CONNECTED = 48,
    
    PSM_EPID_NETWORK_ERROR = 49,
    
    PSM_EPID_INVALID_PKEY = 50,
    
    PSM_EPID_PATH_RESOLUTION = 51,

    
    PSM_MQ_NO_COMPLETIONS = 60,
    
    PSM_MQ_TRUNCATION = 61,

    
    PSM_AM_INVALID_REPLY = 70,
    
    PSM_ERROR_LAST = 80
};

/* Backwards header compatibility for a confusing error return name */
#define PSM_MQ_INCOMPLETE PSM_MQ_NO_COMPLETIONS

typedef enum psm_error psm_error_t;

enum psm_component {
  
  PSM_COMPONENT_CORE = 0,
  
  PSM_COMPONENT_MQ = 1,
  
  PSM_COMPONENT_AM = 2,
  
  PSM_COMPONENT_IB = 3
};

typedef enum psm_component psm_component_t;

enum psm_path_res {  
  
  PSM_PATH_RES_NONE = 0,
  
  PSM_PATH_RES_OPP = 1,
  
  PSM_PATH_RES_UMAD = 2
};

typedef enum psm_path_res psm_path_res_t;
  
/* Initialize PSM interface
 *
 * Call to initialize the PSM library for a desired API revision number.
 *
 * [in,out] api_verno_major As input a pointer to an integer that holds
 *                                PSM_VERNO_MAJOR. As output, the pointer
 *                                is updated with the major revision number of
 *                                the loaded library.
 * [in,out] api_verno_minor As intput, a pointer to an integer that holds
 *                                PSM_VERNO_MINOR.  As output, the pointer
 *                                is updated with the minor revision number of
 *                                the loaded library.
 *
 * [pre] The user has not called any other PSM library call except @ref
 *      psm_error_register_handler to register a global error handler.
 *
 * [warning] PSM initialization is a precondition for all functions used in the
 *          PSM library.
 *
 * [returns] PSM_OK The PSM interface could be opened and the desired API
 *                 revision can be provided.
 * [returns] PSM_INIT_BAD_API_VERSION The PSM library cannot compatibility for
 *                                   the desired API version.  
 * 
 */
psm_error_t
psm_init(int *api_verno_major, int *api_verno_minor);

/* Finalize PSM interface
 *
 * Single call to finalize PSM and close all unclosed endpoints 
 *
 * [post] The user guarantees not to make any further PSM calls, including @ref
 * psm_init.
 *
 * [returns] PSM_OK Always returns PSM_OK */
psm_error_t
psm_finalize(void);

/* Error handling opaque token
 *
 * A token is required for users that register their own handlers and wish to
 * defer further error handling to PSM. */
typedef struct psm_error_token	*psm_error_token_t;

/* Error handling function
 *
 * Users can handle errors explicitly instead of relying on PSM's own error
 * handler.  There is one global error handler and error handlers that can be
 * individually set for each opened endpoint.  By default, endpoints will
 * inherit the global handler registered at the time of open. 
 *
 * [in] ep Handle associated to the endpoint over which the error occured
 *               or NULL if the error is being handled by the global error
 *               handler.
 * [in] error PSM error identifier
 * [in] error_string A descriptive error string of maximum length @ref
 *                         PSM_ERRSTRING_MAXLEN.
 * [in] token Opaque PSM token associated with the particular event that
 *		    generated the error.  The token can be used to extract the
 *		    error string and can be passed to psm_error_defer to
 *		    defer any remaining or unhandled error handling to PSM. 
 *
 * [post] If the error handler returns, the error returned is propagated to the
 *       caller.  */
typedef psm_error_t (*psm_ep_errhandler_t)(psm_ep_t ep, 
					   const psm_error_t error, 
					   const char *error_string,
					   psm_error_token_t token);

/* Obsolete names, only here for backwards compatibility */
#define PSM_ERRHANDLER_DEFAULT	((psm_ep_errhandler_t)-1)
#define PSM_ERRHANDLER_NOP	((psm_ep_errhandler_t)-2)

#define PSM_ERRHANDLER_PSM_HANDLER  ((psm_ep_errhandler_t)-1)
/* PSM error handler as explained in error_handling */

#define PSM_ERRHANDLER_NO_HANDLER   ((psm_ep_errhandler_t)-2)
/* Bypasses the default PSM error handler and returns all errors to the user
 * (this is the default) */

#define PSM_ERRSTRING_MAXLEN	512 /* Maximum error string length. */

/* PSM error handler registration
 *
 * Function to register error handlers on a global basis and on a per-endpoint
 * basis.  PSM_ERRHANDLER_PSM_HANDLER and PSM_ERRHANDLER_NO_HANDLER are special
 * pre-defined handlers to respectively enable use of the default PSM-internal
 * handler or the no-handler that disables registered error handling and
 * returns all errors to the caller (both are documented in error_handling).
 *
 * [in] ep Handle of the endpoint over which the error handler should be
 *               registered.  With ep set to NULL, the behavior of the
 *               global error handler can be controlled.
 * [in] errhandler Handler to register.  Can be a user-specific error
 *                       handling function or PSM_ERRHANDLER_PSM_HANDLER or
 *                       PSM_ERRHANDLER_NO_HANDLER.
 *
 * @remark When ep is set to NULL, this is the only function that can be
 * called before psm_init
 */
psm_error_t
psm_error_register_handler(psm_ep_t ep, const psm_ep_errhandler_t errhandler);

/* PSM deferred error handler 
 *
 * Function to handle fatal PSM errors if no error handler is installed or if
 * the user wishes to defer further error handling to PSM.  Depending on the
 * type of error, PSM may or may not return from the function call.
 *
 * [in] err_token Error token initially passed to error handler
 *
 * [pre] The user is calling into the function because it has decided that PSM
 *      should handle an error case.  
 *
 * [post] The function may or may not return depending on the error
 */
psm_error_t
psm_error_defer(psm_error_token_t err_token);

/* Get generic error string from error
 *
 * Function to return the default error string associated to a PSM error.
 *
 * While a more detailed and precise error string is usually available within
 * error handlers, this function is available to obtain an error string out of
 * an error handler context or when a no-op error handler is registered.
 *
 * [in] error PSM error
 */
const char *
psm_error_get_string(psm_error_t error);

/* Option key/pair structure 
 * 
 * Currently only used in MQ.
 */
struct psm_optkey
{
    uint32_t key;
    void     *value;
};



/* Endpoint ID
 *
 * Integral type of size 8 bytes that can be used by the user to globally
 * identify a successfully opened endpoint.  Although the contents of the
 * endpoint id integral type remains opaque to the user, unique network id and
 * InfiniPath port number can be extracted using psm_epid_nid and @ref
 * psm_epid_context.
 */
typedef uint64_t psm_epid_t;

/* Endpoint Address (opaque)
 *
 * Remote endpoint addresses are created when the user binds an endpoint ID
 * to a particular endpoint handle using psm_ep_connect.  A given endpoint
 * address is only guaranteed to be valid over a single endpoint.  
 */
typedef struct psm_epaddr *psm_epaddr_t;

/* PSM Unique UID 
 *
 * PSM type equivalent to the DCE-1 uuid_t, used to uniquely identify an
 * endpoint within a particular job.  Since PSM does not participate in job
 * allocation and management, users are expected to generate a unique ID to
 * associate endpoints to a particular parallel or collective job.
 * [see] psm_uuid_generate
 */
typedef uint8_t psm_uuid_t[16];

/* Get Endpoint identifier's Unique Network ID */
uint64_t
psm_epid_nid(psm_epid_t epid);

/* Get Endpoint identifier's InfiniPath context number */
uint64_t
psm_epid_context(psm_epid_t epid);

/* Get Endpoint identifier's InfiniPath port (deprecated, use
 * psm_epid_context instead) */
uint64_t
psm_epid_port(psm_epid_t epid);

/* List the number of available InfiniPath units
 *
 * Function used to determine the amount of locally available InfiniPath units.
 * For N units, valid unit numbers in psm_ep_open are 0 to N-1.
 *
 * [returns] PSM_OK unless the user has not called psm_init
 */
psm_error_t
psm_ep_num_devunits(uint32_t *num_units);

/* Utility to generate UUIDs for psm_ep_open
 *
 * This function is available as a utility for generating unique job-wide ids.
 * See discussion in psm_ep_open for further information.
 *
 * @remark This function does not require PSM to be initialized.
 */
void
psm_uuid_generate(psm_uuid_t uuid_out);

/* Affinity modes for the affinity member of struct psm_ep_open_opts */
#define PSM_EP_OPEN_AFFINITY_SKIP     0	/* Disable setting affinity */
#define PSM_EP_OPEN_AFFINITY_SET      1	/* Enable setting affinity unless
					  already set */ 
#define PSM_EP_OPEN_AFFINITY_FORCE    2 /* Enable setting affinity regardless
					  of current affinity setting */

/* Default values for some constants */
#define PSM_EP_OPEN_PKEY_DEFAULT    0xffffffffffffffffULL 
				    /* Default protection key */

/* Endpoint Open Options
 *
 * These options are available for opening a PSM endpoint.  Each is
 * individually documented and setting each option to -1 or passing NULL as the
 * options parameter in psm_ep_open instructs PSM to use
 * implementation-defined defaults.
 *
 * Each option is documented in psm_ep_open */
struct psm_ep_open_opts {
    int64_t   timeout;	    /* timeout in nanoseconds to open device */
    int	      unit;	    /* InfiniPath Unit ID to open on */
    int	      affinity;     /* How PSM should set affinity */
    int	      shm_mbytes;   /* Megabytes used for intra-node communication */
    int	      sendbufs_num; /* Preallocated send buffers */
#if PSM_VERNO >= 0x0101
    uint64_t  network_pkey; /* Network Protection Key (v1.01) */
#endif
#if PSM_VERNO >= 0x0107
    int	      port;	      /* IB port to use (1...N) */
#if PSM_VERNO <= 0x010a
    int	      outvl;	      /* IB VL to use when sending pkts */
#endif
    int	      outsl;	      /* IB SL to use when sending pkts */
#endif
#if PSM_VERNO >= 0x010d
    uint64_t  service_id;     /* IB Service ID to use for endpoint */
    psm_path_res_t path_res_type;  /* Path resolution type */
#endif
#if PSM_VERNO >= 0x010e
    int       senddesc_num;   /* Preallocated send descriptors */
    int       imm_size;       /* Immediate data size for endpoint */
#endif

};

/* InfiniPath endpoint creation
 *
 * Function used to create a new local communication endpoint on an InfiniPath
 * adapter.  The returned endpoint handle is required in all PSM communication
 * operations, as PSM can manage communication over multiple endpoints.  An
 * opened endpoint has no global context until the user connects the endpoint
 * to other global endpoints by way of psm_ep_connect.  All local endpoint
 * handles are globally identified by endpoint IDs (psm_epid_t) which are
 * also returned when an endpoint is opened.  It is assumed that the user can
 * provide an out-of-band mechanism to distribute the endpoint IDs in order to
 * establish connections between endpoints (psm_ep_connect for more
 * information).
 *
 * [in] unique_job_key Endpoint key, to uniquely identify the endpoint in
 *                           a parallel job.  It is up to the user to ensure
 *                           that the key is globally unique over a period long
 *                           enough to prevent duplicate keys over the same set
 *                           of endpoints (see comments below).
 *
 * [in] opts Open options of type psm_ep_open_opts 
 *                 (see psm_ep_open_opts_get_defaults).
 *
 * [out] ep User-supplied storage to return a pointer to the newly
 *                created endpoint.  The returned pointer of type psm_ep_t
 *                is a local handle and cannot be used to globally identify the
 *                endpoint.
 * [out] epid User-supplied storage to return the endpoint ID associated
 *                  to the newly created local endpoint returned in the ep
 *                  handle.  The endpoint ID is an integral type suitable for
 *                  uniquely identifying the local endpoint.
 *
 * PSM does not internally verify the consistency of the uuid, it is up to the
 * user to ensure that the uid is unique enough not to collide with other
 * currently-running jobs.  Users can employ three mechanisms to obtain a uuid.
 *
 * 1. Use the supplied psm_uuid_generate utility
 *
 * 2. Use an OS or library-specific uuid generation utility, that complies with
 *    OSF DCE 1.1, such as uuid_generate on Linux or uuid_create on FreeBSD.
 *    (see http://www.opengroup.org/onlinepubs/009629399/uuid_create.htm)
 *
 * 3. Manually pack a 16-byte string using a utility such as /dev/random or
 *    other source with enough entropy and proper seeding to prevent two nodes
 *    from generating the same uuid_t.
 *
 * The following options are relevent when opening an endpoint:
 *   * timeout establishes the amount of nanoseconds to wait before
 *                  failing to open a port (with -1, defaults to 15 secs).
 *   * unit sets the InfiniPath unit number to use to open a port (with
 *               -1, PSM determines the best unit to open the port).  If @c
 *               IPATH_UNIT is set in the environment, this setting is ignored.
 *   * affinity enables or disables PSM setting processor affinity.  The
 *                   option can be controlled to either disable (@ref
 *                   PSM_EP_OPEN_AFFINITY_SKIP) or enable the affinity setting
 *                   only if it is already unset (@ref
 *                   PSM_EP_OPEN_AFFINITY_SET) or regardless of affinity begin
 *                   set or not (PSM_EP_OPEN_AFFINITY_FORCE).
 *                   If IPATH_NO_CPUAFFINITY is set in the environment, this
 *                   setting is ignored.
 *   * shm_mbytes sets a maximum amount of megabytes that can be allocated
 *		       to each local endpoint ID connected through this
 *		       endpoint (with -1, defaults to 10 MB).
 *   * sendbufs_num sets the number of send buffers that can be
 *                       pre-allocated for communication (with -1, defaults to
 *                       512 buffers of MTU size).
 *   * network_pkey sets the protection key to employ for point-to-point
 *                       PSM communication.  Unless a specific value is used,
 *                       this parameter should be set to
 *                       PSM_EP_OPEN_PKEY_DEFAULT.
 *
 * [warning] Currently, PSM limits the user to calling psm_ep_open only once
 * per process and subsequent calls will fail.  Multiple endpoints per process
 * will be enabled in a future release.
 *
 */
psm_error_t
psm_ep_open(const psm_uuid_t unique_job_key, const struct psm_ep_open_opts *opts,
	    psm_ep_t *ep, psm_epid_t *epid);

/* Endpoint open default options.
 *
 * Function used to initialize the set of endpoint options to their default
 * values for use in psm_ep_open.
 *
 * [out] opts Endpoint Open options.
 *
 * [warning] For portable operation, users should always call this function
 * prior to calling psm_ep_open.
 *
 * [return] PSM_OK If result could be updated
 * [return] PSM_INIT_NOT_INIT If psm has not been initialized.
 */
psm_error_t
psm_ep_open_opts_get_defaults(struct psm_ep_open_opts *opts);

/* Endpoint shared memory query
 *
 * Function used to determine if a remote endpoint shares memory with a
 * currently opened local endpiont.
 *
 * [in] ep Endpoint handle
 * [in] epid Endpoint ID
 *
 * result is non-zero if the remote endpoint shares memory with the local
 * endpoint ep, or zero otherwise.
 *
 * [return] PSM_OK If result could be updated
 * [return] PSM_EPID_UNKNOWN If the epid is not recognized
 */
psm_error_t
psm_ep_epid_share_memory(psm_ep_t ep, psm_epid_t epid, int *result);

/* Close endpoint
 * [in] ep PSM endpoint handle
 * [in] mode One of PSM_EP_CLOSE_GRACEFUL or PSM_EP_CLOSE_FORCE
 * [in] timeout How long to wait in nanoseconds if mode is
 *			PSM_EP_CLOSE_GRACEFUL, 0 waits forever.  If mode is
 *			PSM_EP_CLOSE_FORCE, this parameter is ignored.
 *
 * The following errors are returned, others are handled by the per-endpoint
 * error handler:
 *
 * [return] PSM_OK  Endpoint was successfully closed without force or
 *                 successfully closed with force within the supplied timeout.
 * [return] PSM_EP_CLOSE_TIMEOUT Endpoint could not be successfully closed
 *                              within timeout.
 */
psm_error_t
psm_ep_close(psm_ep_t ep, int mode, int64_t timeout);

#define PSM_EP_CLOSE_GRACEFUL	0   /* Graceful close mode in psm_ep_close */
#define PSM_EP_CLOSE_FORCE	1   /* Forceful close mode in psm_ep_close */

/* Provide mappings for network id to hostname
 *
 * Since PSM does not assume or rely on the availability of an external
 * networkid-to-hostname mapping service, users can provide one or more of
 * these mappings.  The psm_map_nid_hostname function allows a list of
 * network ids to be associated to hostnames.
 *
 * This function is not mandatory for correct operation but may allow PSM to
 * provide better diagnostics when remote endpoints are unavailable and can
 * otherwise only be identified by their network id.
 *
 * [in] num Number elements in nid and hostnames arrays
 * [in] nids User-provided array of network ids (i.e. InfiniBand LIDs),
 *                 should be obtained by calling psm_epid_nid on each
 *                 epid.
 * [in] hostnames User-provided array of hostnames (array of
 *                      NUL-terimated strings) where each hostname index
 *                      maps to the provided nid hostname.
 *
 * [warning] Duplicate nids may be provided in the input nids array, only
 *          the first corresponding hostname will be remembered.
 *
 * [pre] The user may or may not have already provided a hostname mappings.  
 * [post] The user may free any dynamically allocated memory passed to the
 *       function.
 *
 */
psm_error_t
psm_map_nid_hostname(int num, const uint64_t *nids, const char **hostnames);

/* Connect one or more remote endpoints to a local endpoint
 *
 * Function to non-collectively establish a connection to a set of endpoint IDs
 * and translate endpoint IDs into endpoint addresses.  Establishing a remote
 * connection with a set of remote endpoint IDs does not imply a collective
 * operation and the user is free to connect unequal sets on each process.
 * Similarly, a given endpoint address does not imply that a pairwise
 * communication context exists between the local endpoint and remote endpoint.
 *
 * [in] ep PSM endpoint handle
 *
 * [in] num_of_epid The amount of endpoints to connect to, which
 *			  also establishes the amount of elements contained in
 *			  all of the function's array-based parameters.
 *
 * [in] array_of_epid User-allocated array that contains num_of_epid
 *                          valid endpoint identifiers.  Each endpoint id (or
 *                          epid) has been obtained through an out-of-band
 *                          mechanism and each endpoint must have been opened
 *                          with the same uuid key.
 *
 * [in] array_of_epid_mask User-allocated array that contains num_of_epid
 *			    integers.  This array of masks allows users to
 *			    select which of the epids in array_of_epid
 *			    should be connected.  If the integer at index i is
 *			    zero, psm does not attempt to connect to the epid
 *			    at index i in array_of_epid.  If this parameter
 *			    is NULL, psm will try to connect to each epid.
 *
 * [out] array_of_errors User-allocated array of at least num_of_epid
 *                             elements. If the function does not return
 *                             PSM_OK, this array can be consulted for each
 *                             endpoint not masked off by array_of_epid_mask
 *                             to know why the endpoint could not be connected.
 *                             Endpoints that could not be connected because of
 *                             an unrelated failure will be marked as @ref
 *                             PSM_EPID_UNKNOWN.  If the function returns
 *                             PSM_OK, the errors for all endpoints will also
 *                             contain PSM_OK.
 *
 * [out] array_of_epaddr User-allocated array of at least num_of_epid
 *                             elements of type psm_epaddr_t.  Each
 *                             successfully connected endpoint is updated with
 *                             an endpoint address handle that corresponds to
 *                             the endpoint id at the same index in @c
 *                             array_of_epid.  Handles are only updated if the
 *                             endpoint could be connected and if its error in
 *                             array_of_errors is PSM_OK.
 *
 * [in] timeout Timeout in nanoseconds after which connection attempts will
 *                    be abandoned.  Setting this value to 0 disables timeout
 *                    and waits until all endpoints have been successfully
 *                    connected or until an error is detected.
 *
 * [pre] The user has opened a local endpoint and obtained a list of endpoint
 *      IDs to connect to a given endpoint handle using an out-of-band
 *      mechanism not provided by PSM.
 *
 * [post] If the connect is successful, array_of_epaddr is updated with valid
 *       endpoint addresses.  
 *
 * [post] If unsuccessful, the user can query the return status of each
 *       individual remote endpoint in array_of_errors.
 * 
 * [post] The user can call into psm_ep_connect many times with the same
 *       endpoint ID and the function is guaranteed to return the same output
 *       parameters.
 *
 * [post] PSM does not keep any reference to the arrays passed into the
 *       function and the caller is free to deallocate them.
 *
 * The error value with the highest importance is returned by
 * the function if some portion of the communication failed.  Users should
 * always refer to individual errors in array_of_errors whenever the
 * function cannot return PSM_OK.
 *
 * [returns] PSM_OK  The entire set of endpoint IDs were successfully connected
 *                  and endpoint addresses are available for all endpoint IDs.
 *
 */
psm_error_t
psm_ep_connect(psm_ep_t ep, int num_of_epid, const psm_epid_t *array_of_epid,
	       const int *array_of_epid_mask, psm_error_t *array_of_errors, 
	       psm_epaddr_t *array_of_epaddr, int64_t timeout);

/* Ensure endpoint communication progress 
 *
 * Function to ensure progress for all PSM components instantiated on an
 * endpoint (currently, this only includes the MQ component).  The function
 * never blocks and is typically required in two cases:
 *
 * * Allowing all PSM components instantiated over a given endpoint to make
 *     communication progress. Refer to mq_progress for a detailed
 *     discussion on MQ-level progress issues.
 *
 * * Cases where users write their own synchronization primitives that
 *     depend on remote communication (such as spinning on a memory location
 *     which's new value depends on ongoing communication).
 *
 * The poll function doesn't block, but the user can rely on the @ref
 * PSM_OK_NO_PROGRESS return value to control polling behaviour in terms of
 * frequency (poll until an event happens) or execution environment (poll for a
 * while but yield to other threads of CPUs are oversubscribed).
 *
 * [returns] PSM_OK	       Some communication events were progressed
 * [returns] PSM_OK_NO_PROGRESS Polling did not yield any communication progress
 *
 */
psm_error_t
psm_poll(psm_ep_t ep);

/* Set a user-determined ep address label.
 *
 * [in] epaddr Endpoint address, obtained from psm_ep_connect
 * [in] epaddr_label_string User-allocated string to print when
 *                   identifying endpoint in error handling or other verbose
 *                   printing.  The NULL-terminated string must be allocated by
 *                   the user since PSM only keeps a pointer to the label.  If
 *                   users do not explicitly set a label for each endpoint,
 *                   endpoints will identify themselves as hostname:port.
 */
void
psm_epaddr_setlabel(psm_epaddr_t epaddr, const char *epaddr_label_string);

/* Set a user-determined ep address context.
 *
 * [in] epaddr Endpoint address, obtained from psm_ep_connect
 * [in] ctxt   Opaque user defined state to associate with an endpoint
 *                   address. This state can be retrieved via 
 *                   psm_epaddr_getctxt.
 */
void 
psm_epaddr_setctxt(psm_epaddr_t epaddr, void *ctxt);

/* Get the user-determined ep address context. Users can associate an
 *  opaque context with each endpoint via psm_epaddr_setctxt.
 *
 * [in] epaddr Endpoint address, obtained from psm_ep_connect.
 */  
void *
psm_epaddr_getctxt(psm_epaddr_t epaddr);

/* Below are all component specific options. The component object for each of
 * the options is also specified. 
 */
  
/* PSM_COMPONENT_CORE options */
/* PSM debug level */
#define PSM_CORE_OPT_DEBUG     0x101
  /* [uint32_t ] Set/Get the PSM debug level. This option can be set
   * before initializing the PSM library.
   *
   * component object: (null)
   * option value: PSM Debug mask to set or currently active debug level.
   */
  
/* PSM endpoint address context */  
#define PSM_CORE_OPT_EP_CTXT   0x102
  /* [uint32_t ] Set/Get the context associated with a PSM endpoint
   * address (psm_epaddr_t).
   *
   * component object: PSM endpoint (psm_epaddr_t) address.
   * option value: Context associated with PSM endpoint address.
   */

/* PSM_COMPONENT_IB options */
/* Default service level to use to communicate with remote endpoints */
#define PSM_IB_OPT_DF_SL 0x201
  /* [uint32_t ] Default Infiniband SL to use for all remote communication.
   * If unset defaults to Service Level 0. 
   *
   * component object: Opened PSM endpoint id (psm_ep_t).
   * option value: Default IB SL to use for endpoint. (0 <= SL < 15)
   */

/* Set IB service level to use for communication to an endpoint */
#define PSM_IB_OPT_EP_SL 0x202
  /* [uint32_t ] Infiniband SL to use for communication to specified
   * remote endpoint.
   * 
   * component object: PSM endpoint (@ ref psm_epaddr_t) address.
   * option value: SL used to communicate with remote endpoint. (0 <= SL < 15)
   */

/* PSM_COMPONENT_MQ options (deprecates psm_mq_set|getopt) */
/* MQ options that can be set in psm_mq_init and psm_{set,get}_opt */
#define PSM_MQ_OPT_RNDV_IB_SZ       0x301
#define PSM_MQ_RNDV_IPATH_SZ	    PSM_MQ_OPT_RNDV_IB_SZ 
  /* [uint32_t ] Size at which to start enabling rendezvous
   * messaging for InfiniPath messages (if unset, defaults to values
   * between 56000 and 72000 depending on the system configuration) 
   *
   * component object: PSM Matched Queue (psm_mq_t).
   * option value: Size at which to switch to rendezvous protocol.
   */
  
#define PSM_MQ_OPT_RNDV_SHM_SZ      0x302
#define PSM_MQ_RNDV_SHM_SZ	    PSM_MQ_OPT_RNDV_SHM_SZ
  /* [uint32_t ] Size at which to start enabling
   * rendezvous messaging for shared memory (intra-node) messages (If
   * unset, defaults to 64000 bytes). 
   *
   * component object: PSM Matched Queue (psm_mq_t).
   * option value: Size at which to switch to rendezvous protocol.
   */
  
#define PSM_MQ_OPT_SYSBUF_MYBYTES   0x303
#define PSM_MQ_MAX_SYSBUF_MBYTES    PSM_MQ_OPT_SYSBUF_MYBYTES
  /* [uint32_t ] Maximum amount of bytes to allocate for unexpected
   * messages.
   *
   * component object: PSM Matched Queue (psm_mq_t).
   * option value: Maximum amount of bytes to allocate for unexpected messages.
   * Mesages that would cause memory allocation to exceed this amount will be
   * dropped.
   */


/* PSM_COMPONENT_AM options */
#define PSM_AM_OPT_FRAG_SZ          0x401
  
  
/* Set an option for a PSM component
 *
 * Function to set the value of a PSM component option
 *
 * [in] component Type of PSM component for which to set the option
 * [in] component_obj Opaque component specify object to apply the set
 *                          operation on. These are passed uninterpreted to the
 *                          appropriate component for interpretation.
 * [in] optname Name of component option to set. These are component
 *                    specific and passed uninterpreted to the appropriate
 *                    component for interpretation.
 * [in] optval Pointer to storage that contains the value to be updated
 *                   for the supplied option.  It is up to the user to
 *                   ensure that the pointer points to a memory location with a
 *                   correct size and format.
 * [in] optlen Size of the memory region pointed to by optval.
 *
 * [returns] PSM_OK if option could be set.
 * [returns] PSM_PARAM_ERR if the component or optname are not valid.
 * [returns] PSM_OPT_READONLY if the option to be set is a read-only option.
 *                           
 */
psm_error_t
psm_setopt(psm_component_t component, const void *component_obj,
	   int optname, const void *optval, uint64_t optlen);

/* Get an option for a PSM component
 *
 * Function to get the value of a PSM component option
 *
 * [in] component Type of PSM component for which to get the option
 * [in] component_obj Opaque component specify object to apply the get
 *                          operation on. These are passed uninterpreted to the
 *                          appropriate component for interpretation.
 * [in] optname Name of component option to get. These are component
 *                    specific and passed uninterpreted to the appropriate
 *                    component for interpretation.
 * [out] optval Pointer to storage that contains the value to be updated
 *                    for the supplied option.  It is up to the user to
 *                    ensure that the pointer points to a valid memory region.
 * [inout] optlen This is a value result parameter initially containing 
 *                      the size of the memory region pointed to by optval and 
 *                      modified to return the actual size of optval.
 *
 * [returns] PSM_OK if option value could be retrieved successfully.
 * [returns] PSM_PARAM_ERR if the component or optname are not valid.
 * [returns] PSM_NO_MEMORY if the memory region optval is of insufficient size.
 *                         optlen contains the required memory region size for
 *                         optname value.
 *
 */
psm_error_t
psm_getopt(psm_component_t component, const void *component_obj,
	   int optname, void *optval, uint64_t *optlen);
  

#ifdef __cplusplus
}				/* extern "C" */
#endif
#endif
