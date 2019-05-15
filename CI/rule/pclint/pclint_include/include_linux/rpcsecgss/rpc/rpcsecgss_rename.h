#ifndef _RPCSECGSS_RENAME_H_
#define _RPCSECGSS_RENAME_H_


/*
 * Function names
 */

#define _authenticate		    rpcsecgss__authenticate
#if 0	/* These names shouldn't conflict with libc */
#define authgss_create		    rpcsecgss_authgss_create
#define authgss_create_default	    rpcsecgss_authgss_create_default
#define authgss_free_private_data   rpcsecgss_authgss_free_private_data
#define authgss_get_private_data    rpcsecgss_authgss_get_private_data
#define authgss_log_dbg		    rpcsecgss_authgss_log_dbg
#define authgss_log_err		    rpcsecgss_authgss_log_err
#define authgss_log_hexdump	    rpcsecgss_authgss_log_hexdump
#define authgss_log_status	    rpcsecgss_authgss_log_status
#define authgss_perror		    rpcsecgss_authgss_perror
#define authgss_service		    rpcsecgss_authgss_service
#define authgss_set_debug_level	    rpcsecgss_authgss_set_debug_level
#endif
#define callrpc			    rpcsecgss_callrpc
#define clnt_create		    rpcsecgss_clnt_create
#define clnt_pcreateerror	    rpcsecgss_clnt_pcreateerror
#define clnt_perrno		    rpcsecgss_clnt_perrno
#define clnt_perror		    rpcsecgss_clnt_perror
#define clntraw_create		    rpcsecgss_clntraw_create
#define clnt_spcreateerror	    rpcsecgss_clnt_spcreateerror
#define clnt_sperrno		    rpcsecgss_clnt_sperrno
#define clnt_sperror		    rpcsecgss_clnt_sperror
#define clnttcp_create		    rpcsecgss_clnttcp_create
#define clntudp_bufcreate	    rpcsecgss_clntudp_bufcreate
#define clntudp_create		    rpcsecgss_clntudp_create
#define print_rpc_gss_sec	    rpcsecgss_print_rpc_gss_sec
#define registerrpc		    rpcsecgss_registerrpc
#define _svcauth_gss		    rpcsecgss__svcauth_gss
#define svcauth_gss_get_principal   rpcsecgss_svcauth_gss_get_principal
#define svcauth_gss_nextverf	    rpcsecgss_svcauth_gss_nextverf
#define svcauth_gss_set_svc_name    rpcsecgss_svcauth_gss_set_svc_name
#define _svcauth_none		    rpcsecgss__svcauth_none
#define _svcauth_short		    rpcsecgss__svcauth_short
#define _svcauth_unix		    rpcsecgss__svcauth_unix
#define svcerr_auth		    rpcsecgss_svcerr_auth
#define svcerr_decode		    rpcsecgss_svcerr_decode
#define svcerr_noproc		    rpcsecgss_svcerr_noproc
#define svcerr_noprog		    rpcsecgss_svcerr_noprog
#define svcerr_progvers		    rpcsecgss_svcerr_progvers
#define svcerr_systemerr	    rpcsecgss_svcerr_systemerr
#define svcerr_weakauth		    rpcsecgss_svcerr_weakauth
#define svcfd_create		    rpcsecgss_svcfd_create
#define svc_getreq		    rpcsecgss_svc_getreq
#define svc_getreqset		    rpcsecgss_svc_getreqset
#define svc_getreqset2		    rpcsecgss_svc_getreqset2
#define svcraw_create		    rpcsecgss_svcraw_create
#define svc_register		    rpcsecgss_svc_register
#define svc_run			    rpcsecgss_svc_run
#define svc_sendreply		    rpcsecgss_svc_sendreply
#define svctcp_create		    rpcsecgss_svctcp_create
#define svcudp_bufcreate	    rpcsecgss_svcudp_bufcreate
#define svcudp_create		    rpcsecgss_svcudp_create
#define svcudp_enablecache	    rpcsecgss_svcudp_enablecache
#define svc_unregister		    rpcsecgss_svc_unregister
#if 0	/* These names shouldn't conflict with libc */
#define xdr_rpc_gss_buf		    rpcsecgss_xdr_rpc_gss_buf
#define xdr_rpc_gss_cred	    rpcsecgss_xdr_rpc_gss_cred
#define xdr_rpc_gss_data	    rpcsecgss_xdr_rpc_gss_data
#define xdr_rpc_gss_init_args	    rpcsecgss_xdr_rpc_gss_init_args
#define xdr_rpc_gss_init_res	    rpcsecgss_xdr_rpc_gss_init_res
#define xdr_rpc_gss_unwrap_data	    rpcsecgss_xdr_rpc_gss_unwrap_data
#define xdr_rpc_gss_wrap_data	    rpcsecgss_xdr_rpc_gss_wrap_data
#endif
#define xprt_register		    rpcsecgss_xprt_register
#define xprt_unregister		    rpcsecgss_xprt_unregister


/*
 * Data names
 */

#define  _null_auth		    rpcsecgss__null_auth
#define  pl			    rpcsecgss_pl
#define  rpc_createerr		    rpcsecgss_rpc_createerr
#define  svc_auth_none		    rpcsecgss_svc_auth_none
#define  svc_auth_none_ops	    rpcsecgss_svc_auth_none_ops
#define  __svc_fdset		    rpcsecgss___svc_fdset
#define  __svc_fdsetsize	    rpcsecgss___svc_fdsetsize
#define  svc_maxfd		    rpcsecgss_svc_maxfd

#endif	/* _RPCSECGSS_RENAME_H_ */
