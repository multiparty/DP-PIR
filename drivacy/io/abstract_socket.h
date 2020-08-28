// Copyright 2020 multiparty.org

// Defines the socket API used by implementations for simulated local
// environemnts, and real over-the-wire deployments.

#ifndef DRIVACY_IO_ABSTRACT_SOCKET_H_
#define DRIVACY_IO_ABSTRACT_SOCKET_H_

#include <cstdint>
#include <functional>

#include "drivacy/types/messages.pb.h"

namespace drivacy {
namespace io {
namespace socket {

class AbstractSocket {
 public:
  virtual void SendQuery(uint32_t party, const types::Query &query) const = 0;
  virtual void SendResponse(uint32_t party,
                            const types::Response &response) const = 0;

  AbstractSocket() {}
  virtual ~AbstractSocket() = default;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_ABSTRACT_SOCKET_H_
