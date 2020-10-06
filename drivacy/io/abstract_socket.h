// Copyright 2020 multiparty.org

// Defines the socket API used by implementations for simulated local
// environemnts, and real over-the-wire deployments.

#ifndef DRIVACY_IO_ABSTRACT_SOCKET_H_
#define DRIVACY_IO_ABSTRACT_SOCKET_H_

#include <cstdint>
#include <functional>
#include <memory>

#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace io {
namespace socket {

class SocketListener {
 public:
  // Indicates that a batch start message was received specifying the
  // size of the batch.
  virtual void OnReceiveBatch(uint32_t batch_size) = 0;
  // Handlers for when a query or response are received.
  virtual void OnReceiveQuery(const types::IncomingQuery &query) = 0;
  virtual void OnReceiveResponse(const types::Response &response) = 0;
};

class AbstractSocket {
 public:
  AbstractSocket(uint32_t party_id, const types::Configuration &config,
                 SocketListener *listener)
      : party_id_(party_id), config_(config), listener_(listener) {
    this->party_count_ = config.parties();
  }

  virtual void Listen() = 0;

  virtual void SendBatch(uint32_t batch_size) = 0;
  virtual void SendQuery(const types::OutgoingQuery &query) = 0;
  virtual void SendResponse(const types::Response &response) = 0;

  virtual void FlushQueries() = 0;
  virtual void FlushResponses() = 0;

 protected:
  uint32_t party_id_;
  uint32_t party_count_;
  types::Configuration config_;
  SocketListener *listener_;
};

using SocketFactory = std::function<std::unique_ptr<AbstractSocket>(
    uint32_t, const types::Configuration &, SocketListener *)>;

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_ABSTRACT_SOCKET_H_
