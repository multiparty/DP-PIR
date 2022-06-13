#ifndef DPPIR_SOCKETS_COMMON_H_
#define DPPIR_SOCKETS_COMMON_H_

#include <poll.h>

#include <cstddef>
#include <cstdint>

#include "DPPIR/sockets/consts.h"

namespace DPPIR {
namespace sockets {
namespace common {

#define MIN(x, y) x < y ? x : y

// Connect to server at given ip and port, return socket fd.
int ConnectTo(const char* ip, int port);

// Listens for count-many incoming connections on given port.
// Stores incoming connection socket fd in out_socks in order
// of connection.
void ListenOn(int port, int* out_socks, size_t count);

// Read on socket specified by fd, and put results in buf.
void Read(int fd, char* buf, size_t buf_size);        // Read exactly buf_size.
size_t ReadSome(int fd, char* buf, size_t buf_size);  // Up to buf_size.

// Read LogicalBuffer.
// Read up to min(read_count, buffer capacity).
template <typename LOGICAL_BUFFER>
void Read(int fd, unsigned read_count, LOGICAL_BUFFER* buf) {
  size_t cap = buf->BufferCapacity();
  size_t count = read_count * buf->UnitSize() - buf->Leftover();
  size_t b = ReadSome(fd, buf->ToBuffer(), MIN(cap, count));
  buf->Update(b);
}

// Send size-many bytes in buf via fd.
void Send(int fd, char* buf, size_t size);

// Send LogicalBuffer.
template <typename LOGICAL_BUFFER>
void Send(int fd, LOGICAL_BUFFER* buf) {
  Send(fd, buf->ToBuffer(), buf->BufferSize());
}

// Poll many fds, either blocking or non-blocking.
size_t Poll(pollfd* pollfds, size_t count, int timeout, bool* result);

}  // namespace common
}  // namespace sockets
}  // namespace DPPIR

#endif  // DPPIR_SOCKETS_COMMON_H_
