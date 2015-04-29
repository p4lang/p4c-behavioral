
/*
 * C/C++ header file for calling server start function from C code
 */

#ifndef _P4_SAI_RPC_SERVER_H_
#define _P4_SAI_RPC_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define P4_SAI_RPC_SERVER_PORT 9091

extern int start_p4_sai_rpc_server(int port);

#ifdef __cplusplus
}
#endif



#endif /* _P4_SAI_RPC_SERVER_H_ */
