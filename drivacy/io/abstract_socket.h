// Copyright 2020 multiparty.org

// Defines the socket API used by implementations for simulated local
// environemnts, and real over-the-wire deployments.

#ifndef DRIVACY_IO_ABSTRACT_SOCKET_H_
#define DRIVACY_IO_ABSTRACT_SOCKET_H_

#include <cstdint>
#include <functional>

#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace io {
namespace socket {

using QueryListener = std::function<void(const types::IncomingQuery &)>;

using ResponseListener = std::function<void(const types::Response &)>;

class AbstractSocket {
 public:
  virtual void Listen() = 0;
  virtual void SendQuery(const types::OutgoingQuery &query) = 0;
  virtual void SendResponse(const types::Response &response) = 0;

  AbstractSocket(uint32_t party_id, QueryListener query_listener,
                 ResponseListener response_listener,
                 const types::Configuration &config)
      : party_id_(party_id),
        query_listener_(query_listener),
        response_listener_(response_listener),
        config_(config) {
    this->party_count_ = config.parties();
  }

 protected:
  uint32_t party_id_;
  uint32_t party_count_;
  QueryListener query_listener_;
  ResponseListener response_listener_;
  types::Configuration config_;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_ABSTRACT_SOCKET_H_
