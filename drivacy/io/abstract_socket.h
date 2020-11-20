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

// Inter-party portion.
class SocketListener {
 public:
  // Indicates that a batch start message was received specifying the
  // size of the batch.
  virtual void OnReceiveBatch(uint32_t batch_size) = 0;
  // Handlers for when a query or response are received.
  virtual void OnReceiveQuery(const types::IncomingQuery &query) = 0;
  virtual void OnReceiveResponse(const types::ForwardResponse &response) = 0;
};

// Base class for socket connecting machines from different parties.
class AbstractSocket {
 public:
  AbstractSocket(uint32_t party_id, uint32_t machine_id,
                 const types::Configuration &config, SocketListener *listener)
      : party_id_(party_id),
        machine_id_(machine_id),
        config_(config),
        listener_(listener) {
    this->party_count_ = config.parties();
    this->incoming_query_msg_size_ =
        types::IncomingQuery::Size(party_id, this->party_count_);
    this->outgoing_query_msg_size_ =
        types::OutgoingQuery::Size(party_id, this->party_count_);
    this->response_msg_size_ = types::Response::Size();
  }

  virtual void Listen() = 0;

  virtual void SendBatch(uint32_t batch_size) = 0;
  virtual void SendQuery(const types::ForwardQuery &query) = 0;
  virtual void SendResponse(const types::Response &response) = 0;

  virtual void FlushQueries() = 0;
  virtual void FlushResponses() = 0;

 protected:
  uint32_t party_id_;
  uint32_t machine_id_;
  uint32_t party_count_;
  types::Configuration config_;
  SocketListener *listener_;
  // Stores the sizes of messages.
  uint32_t incoming_query_msg_size_;
  uint32_t outgoing_query_msg_size_;
  uint32_t response_msg_size_;
};

using SocketFactory = std::function<std::unique_ptr<AbstractSocket>(
    uint32_t, uint32_t, const types::Configuration &, SocketListener *)>;

// Intra-party portion.
class IntraPartySocketListener {
 public:
  virtual void OnReceiveQuery(uint32_t machine_id,
                              const types::ForwardQuery &query) = 0;
  virtual void OnReceiveResponse(uint32_t machine_id,
                                 const types::Response &response) = 0;
};

// Base class for sockets connecting machines within the same party.
class AbstractIntraPartySocket {
 public:
  AbstractIntraPartySocket(uint32_t party_id, uint32_t machine_id,
                           const types::Configuration &config,
                           IntraPartySocketListener *listener)
      : party_id_(party_id),
        machine_id_(machine_id),
        config_(config),
        listener_(listener) {
    this->party_count_ = config.parties();
    this->parallelism_ = config.parallelism();
    this->query_msg_size_ =
        types::OutgoingQuery::Size(party_id, this->party_count_);
    this->response_msg_size_ = types::Response::Size();
    // The backend party does not need Intra-party sockets.
    assert(party_id < this->party_count_);
  }

  virtual void Listen() = 0;

  virtual void SendQuery(uint32_t machine_id,
                         const types::OutgoingQuery &query) = 0;
  virtual void SendResponse(uint32_t machine_id,
                            const types::ForwardResponse &response) = 0;

  virtual void FlushQueries() = 0;
  virtual void FlushResponses() = 0;

 protected:
  uint32_t party_id_;
  uint32_t machine_id_;
  uint32_t party_count_;
  uint32_t parallelism_;
  types::Configuration config_;
  IntraPartySocketListener *listener_;
  // Both incoming and outgoing queries, as well as responses, have the
  // same size.
  uint32_t query_msg_size_;
  uint32_t response_msg_size_;
};

using IntraPartySocketFactory =
    std::function<std::unique_ptr<AbstractIntraPartySocket>(
        uint32_t, uint32_t, const types::Configuration &,
        IntraPartySocketListener *)>;

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_ABSTRACT_SOCKET_H_
