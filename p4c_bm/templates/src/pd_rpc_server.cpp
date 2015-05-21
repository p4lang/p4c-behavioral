//:: # Generate the handler routines for the interfaces

//:: pd_prefix = "p4_pd_" + p4_prefix + "_"
//:: pd_static_prefix = "p4_pd_"
//:: api_prefix = p4_prefix + "_"

#include <stdlib.h>
#include "p4_sim/pd_rpc_server.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/processor/TMultiplexedProcessor.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

#include "conn_mgr_pd_rpc_server.ipp"
#include "mc_pd_rpc_server.ipp"
#include "p4_pd_rpc_server.ipp"

/*
 * Thread wrapper for starting the server
 */

static void *rpc_server_thread(void *arg) {
    int port = *(int *) arg;

    shared_ptr<${p4_prefix}Handler> ${p4_prefix}_handler(new ${p4_prefix}Handler());
    shared_ptr<mcHandler> mc_handler(new mcHandler());
    shared_ptr<conn_mgrHandler> conn_mgr_handler(new conn_mgrHandler());

    shared_ptr<TMultiplexedProcessor> processor(new TMultiplexedProcessor());
    processor->registerProcessor(
        "${p4_prefix}",
	shared_ptr<TProcessor>(new ${p4_prefix}Processor(${p4_prefix}_handler))
    );
    processor->registerProcessor(
        "mc",
	shared_ptr<TProcessor>(new mcProcessor(mc_handler))
    );
    processor->registerProcessor(
        "conn_mgr",
	shared_ptr<TProcessor>(new conn_mgrProcessor(conn_mgr_handler))
    );

    shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    TThreadedServer server(processor, serverTransport, transportFactory, protocolFactory);

    server.serve();

    return NULL;
}


static pthread_t rpc_thread;

int start_p4_pd_rpc_server(int port)
{
    std::cerr << "\nP4 Program:  ${p4_name}\n\n";
    std::cerr << "Starting RPC server on port " << port << std::endl;

    int *port_arg = (int *) malloc(sizeof(int));
    *port_arg = port;

    return pthread_create(&rpc_thread, NULL, rpc_server_thread, port_arg);
}
