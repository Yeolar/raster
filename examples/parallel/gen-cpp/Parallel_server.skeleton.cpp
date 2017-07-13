// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "Parallel.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace  ::rdd::parallel;

class ParallelHandler : virtual public ParallelIf {
 public:
  ParallelHandler() {
    // Your initialization goes here
  }

  void run(Result& _return, const Query& query) {
    // Your implementation goes here
    printf("run\n");
  }

};

int main(int argc, char **argv) {
  int port = 9090;
  shared_ptr<ParallelHandler> handler(new ParallelHandler());
  shared_ptr<TProcessor> processor(new ParallelProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
  return 0;
}
