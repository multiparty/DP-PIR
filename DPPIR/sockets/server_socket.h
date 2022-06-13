#ifndef DPPIR_SOCKETS_SERVER_SOCKET_H_
#define DPPIR_SOCKETS_SERVER_SOCKET_H_

#include "DPPIR/sockets/consts.h"
#include "DPPIR/types/containers.h"
#include "DPPIR/types/types.h"

namespace DPPIR {
namespace sockets {

class ServerSocket {
 public:
  explicit ServerSocket(size_t incoming_cipher_size)
      : sockfd_(-1),
        rbuffer_(),
        wbuffer_(),
        cipher_rbuf_(&rbuffer_, incoming_cipher_size),
        query_rbuf_(&rbuffer_),
        response_wbuf_(&wbuffer_) {}

  // Listen on port and accept the incoming connection from client.
  void Initialize(int port);

  // Read count of queries to expect.
  index_t ReadCount();

  // Send ready signal.
  void SendReady();

  // Read offline messages.
  CipherLogicalBuffer& ReadCiphers(index_t read_count);

  // Only reads queries.
  LogicalBuffer<Query>& ReadQueries(index_t read_count);

  // Only writes responses.
  void SendResponse(const Response& response);
  void FlushResponses();

 private:
  int sockfd_;
  // Physical buffers for reading and writing.
  PhysicalBuffer<BUFFER_SIZE> rbuffer_;
  PhysicalBuffer<BUFFER_SIZE> wbuffer_;
  // Logical buffers that wrap the physical buffers with message types.
  CipherLogicalBuffer cipher_rbuf_;
  LogicalBuffer<Query> query_rbuf_;
  LogicalBuffer<Response> response_wbuf_;
};

}  // namespace sockets
}  // namespace DPPIR

#endif  // DPPIR_SOCKETS_SERVER_SOCKET_H_
