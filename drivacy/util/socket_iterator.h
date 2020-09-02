// Copyright 2020 multiparty.org

// Exposes an (input) iterator around a socket.
// In particular, it transforms reading from a socket from
// recv(...) to an iteration operation.
// The Iterator internally uses recv(...) and is blocking!

#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <streambuf>
#include <tuple>

#ifndef DRIVACY_UTIL_SOCKET_ITERATOR_H_
#define DRIVACY_UTIL_SOCKET_ITERATOR_H_

// 10 milliseconds poll timeout on socket
#define POLL_TIMEOUT 100000

namespace drivacy {
namespace util {

class SocketIterator {
 public:
  SocketIterator() {
    this->buffer_ = nullptr;
    this->on_ = true;
  }
  ~SocketIterator() {
    if (this->buffer_ != nullptr) {
      delete this->buffer_;
    }
  }

  // Not copyable or movable.
  SocketIterator(SocketIterator &&other) = delete;
  SocketIterator &operator=(SocketIterator &&other) = delete;
  SocketIterator(const SocketIterator &) = delete;
  SocketIterator &operator=(const SocketIterator &) = delete;

  // Initialization.
  void Initialize(int socketfd) {
    this->socketfd_ = socketfd;
    // Initialize this once to save some time.
    memset(&this->poll_fd_, 0, sizeof(this->poll_fd_));
    this->poll_fd_.fd = this->socketfd_;
    this->poll_fd_.events = POLLIN;
  }

  // Blocks until the next message is ready or until the
  // socket is shutdown.
  bool HasNext() {
    // Read the size prefix of a message.
    char size_buff[4];
    if (!this->ReadExact(size_buff, 4)) {
      return false;
    }

    // Decode the size prefix into byte size and message type.
    uint32_t size = *reinterpret_cast<uint32_t *>(size_buff);
    this->type_ = size >> 31;
    this->size_ = size & 0x7FFFFFFF;

    // Set up buffers.
    if (this->buffer_ != nullptr) {
      delete this->buffer_;
    }

    this->buffer_ = new char[this->size_];
    if (!this->ReadExact(this->buffer_, this->size_)) {
      return false;
    }

    return true;
  }

  // Must be called after has_next() has returned true!
  std::tuple<char *, uint32_t, uint32_t> Next() {
    return std::make_tuple(this->buffer_, this->size_, this->type_);
  }

  void Close() { this->on_ = false; }

 private:
  int socketfd_;
  struct pollfd poll_fd_;
  bool on_;
  char *buffer_;
  uint32_t type_;
  uint32_t size_;

  // Block until either EXACTLY size bytes are read into buff
  // or until the socket is shut down.
  // Returns true if successful, false if the socket was shutdown prior
  // to reading size.
  bool ReadExact(char *buff, size_t size) {
    size_t offset = 0;
    size_t bytes_to_read = size;

    // Keep reading until bytes_to_read have been read!
    // Stop if the socket is closed.
    while (this->on_ && bytes_to_read > 0) {
      int rc = poll(&this->poll_fd_, 1, POLL_TIMEOUT);
      assert(rc >= 0);
      if (rc > 0) {
        int read =
            recv(this->socketfd_, buff + offset, bytes_to_read, MSG_WAITALL);
        assert(read >= 0);
        offset += read;
        bytes_to_read -= read;
      }
    }

    return bytes_to_read == 0;
  }
};

}  // namespace util
}  // namespace drivacy

#endif  // DRIVACY_UTIL_SOCKET_ITERATOR_H_
