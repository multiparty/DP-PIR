#ifndef DPPIR_SOCKETS_CLIENT_SOCKET_H_
#define DPPIR_SOCKETS_CLIENT_SOCKET_H_

#include <string>

#include "DPPIR/sockets/consts.h"
#include "DPPIR/types/containers.h"
#include "DPPIR/types/types.h"

namespace DPPIR {
namespace sockets {

class ClientSocket {
 public:
  explicit ClientSocket(size_t outgoing_cipher_size)
      : sockfd_(-1),
        rbuffer_(),
        wbuffer_(),
        cipher_wbuf_(&wbuffer_, outgoing_cipher_size),
        query_wbuf_(&wbuffer_),
        response_rbuf_(&rbuffer_) {}

  // Initialize / connect to server at ip:port.
  void Initialize(const std::string& ip, int port);

  // Send number of queries to be sent.
  void SendCount(index_t count);

  // Wait until ready signal is received.
  void WaitForReady();

  // Writing offline messages.
  void SendCipher(const char* onion_cipher);
  void FlushCiphers();

  // Only writes queries.
  void SendQuery(const Query& query);
  void FlushQueries();

  // Only reads responses.
  LogicalBuffer<Response>& ReadResponses(index_t read_count);

 private:
  int sockfd_;
  // Physical buffers for reading and writing.
  PhysicalBuffer<BUFFER_SIZE> rbuffer_;
  PhysicalBuffer<BUFFER_SIZE> wbuffer_;
  // Logical buffers that wrap the physical buffers with message types.
  CipherLogicalBuffer cipher_wbuf_;
  LogicalBuffer<Query> query_wbuf_;
  LogicalBuffer<Response> response_rbuf_;
};

}  // namespace sockets
}  // namespace DPPIR

#endif  // DPPIR_SOCKETS_CLIENT_SOCKET_H_
