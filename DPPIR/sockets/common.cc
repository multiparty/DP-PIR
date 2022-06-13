#include "DPPIR/sockets/common.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>

namespace DPPIR {
namespace sockets {
namespace common {

#define __TMP_VARN_NAME(arg) __tmp##arg
#define __TMP_VAL(arg) __TMP_VARN_NAME(arg)

#define MY_ASSERT(x)             \
  int __TMP_VAL(__LINE__) = x;   \
  if (__TMP_VAL(__LINE__) < 0) { \
    perror("socket error: ");    \
    assert(false);               \
  }                              \
  assert(__TMP_VAL(__LINE__) >= 0)

int ConnectTo(const char* ip, int port) {
  // Creating socket file descriptor.
  int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  MY_ASSERT(sockfd);

  // Options.
  int y = 1;
  int rcvbuf = RCVBUF;
  int sndbuf = SNDBUF;
  MY_ASSERT(setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &y, sizeof(y)));
  MY_ASSERT(setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(int)));
  MY_ASSERT(setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(int)));

  // Validate that socket buffers sizes were set correctly.
  int v1, v2;
  socklen_t sz = sizeof(int);
  getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &v1, &sz);
  getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &v2, &sz);
  assert(v1 == 2 * rcvbuf);
  assert(v2 == 2 * sndbuf);

  // Filling server information.
  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;  // IPv4
  servaddr.sin_port = htons(port);
  assert(inet_pton(AF_INET, ip, &servaddr.sin_addr) >= 0);

  // Connect to the server.
  while (connect(sockfd, reinterpret_cast<sockaddr*>(&servaddr),
                 sizeof(servaddr)) < 0) {
    sleep(1);
  }

  return sockfd;
}

void ListenOn(int port, int* out_socks, size_t count) {
  // Socket setup.
  int y = 1;
  int rcvbuf = RCVBUF;
  int sndbuf = SNDBUF;
  int srvfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  MY_ASSERT(srvfd);
  MY_ASSERT(setsockopt(srvfd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(y)));
  MY_ASSERT(setsockopt(srvfd, IPPROTO_TCP, TCP_NODELAY, &y, sizeof(y)));
  MY_ASSERT(setsockopt(srvfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(int)));
  MY_ASSERT(setsockopt(srvfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(int)));
  // MY_ASSERT(setsockopt(sfd, SOL_TCP, TCP_QUICKACK, &y, sizeof(y)));

  // Validate that socket buffers sizes were set correctly.
  int v1, v2;
  socklen_t sz = sizeof(int);
  getsockopt(srvfd, SOL_SOCKET, SO_RCVBUF, &v1, &sz);
  getsockopt(srvfd, SOL_SOCKET, SO_SNDBUF, &v2, &sz);
  assert(v1 == 2 * rcvbuf);
  assert(v2 == 2 * sndbuf);

  // Filling server information.
  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;  // IPv4
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(port);
  sockaddr* servaddr_ptr = reinterpret_cast<sockaddr*>(&servaddr);

  // Bind the socket to the port address.
  MY_ASSERT(bind(srvfd, servaddr_ptr, sizeof(servaddr)));

  // Listening.
  MY_ASSERT(listen(srvfd, count));

  // Accept connection (blocking).
  for (size_t i = 0; i < count; i++) {
    int sockfd = accept(srvfd, NULL, NULL);
    MY_ASSERT(sockfd);
    MY_ASSERT(setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &y, sizeof(y)));

    // Validate that socket buffers sizes were set correctly.
    getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &v1, &sz);
    getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &v2, &sz);
    assert(v1 == 2 * rcvbuf);
    assert(v2 == 2 * sndbuf);

    // Ok!
    out_socks[i] = sockfd;
  }
}

void Read(int fd, char* buf, size_t buf_size) {
  size_t bytes = 0;
  while (bytes < buf_size) {
    auto status = read(fd, buf + bytes, buf_size - bytes);
    MY_ASSERT(status);
    bytes += status;
  }
}

size_t ReadSome(int fd, char* buf, size_t buf_size) {
  auto status = read(fd, buf, buf_size);
  MY_ASSERT(status);
  return status;
}

void Send(int fd, char* buf, size_t size) {
  size_t sent = 0;
  while (sent < size) {
    auto status = send(fd, buf + sent, size - sent, 0);
    MY_ASSERT(status);
    sent += status;
  }
}

// Poll many fds, either blocking or non-blocking.
size_t Poll(pollfd* pollfds, size_t count, int timeout, bool* result) {
  int nfds = poll(pollfds, count, timeout);
  MY_ASSERT(nfds);
  assert(nfds <= static_cast<int>(count));
  size_t res = nfds;
  for (size_t id = 0; nfds > 0; id++) {
    auto& revents = pollfds[id].revents;
    if (revents & POLLIN) {
      revents = 0;
      result[id] = true;
      nfds--;
    }
  }
  return res;
}

}  // namespace common
}  // namespace sockets
}  // namespace DPPIR
