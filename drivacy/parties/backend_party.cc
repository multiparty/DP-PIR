// Copyright 2020 multiparty.org

// This file defines the "BackendParty" class. A specialized Party that
// sits at the deepest level in the protocol chain and responds to queries
// through the table.
//
// All comments about deploying/running parties in simulation or deployment
// in drivacy/parties/party.h apply here too.

#include "drivacy/parties/backend_party.h"

#include <memory>

#include "drivacy/protocol/backend.h"

namespace drivacy {
namespace parties {
namespace {

std::unique_ptr<io::socket::AbstractIntraPartySocket> NullFactory(
    uint32_t /*party_id*/, uint32_t /*machine_id*/,
    const types::Configuration & /*config*/,
    io::socket::IntraPartySocketListener * /*listener*/) {
  return nullptr;
}

}  // namespace

// Constructor
BackendParty::BackendParty(uint32_t party, uint32_t machine,
                           const types::Configuration &config,
                           const types::Table &table, double span,
                           double cutoff,
                           io::socket::SocketFactory socket_factory)
    : Party(party, machine, config, table, span, cutoff, socket_factory,
            NullFactory) {
  this->processed_queries_ = 0;
}

// Store batch size and reset counters.
void BackendParty::OnReceiveBatchSize(uint32_t batch_size) {
#ifdef DEBUG_MSG
  std::cout << "On receive batch size (backend) " << machine_id_ << " = "
            << batch_size << std::endl;
#endif
  this->batch_size_ = batch_size;
  this->responses_ = new types::Response[batch_size];
}

// Handle query and reply back immediately when a query is received.
void BackendParty::OnReceiveQuery(const types::IncomingQuery &query) {
#ifdef DEBUG_MSG
  std::cout << "On receive query (backend) " << machine_id_ << std::endl;
#endif
  // Process query creating a response, send it over socket.
  this->responses_[processed_queries_++] =
      protocol::backend::QueryToResponse(query, config_, table_);

  // Flush socket when everything is done.
  if (this->processed_queries_ == this->batch_size_) {
    this->processed_queries_ = 0;
    this->SendResponses();
  }
}

// Send all responses after they are handled.
void BackendParty::SendResponses() {
  for (uint32_t i = 0; i < this->batch_size_; i++) {
    this->socket_->SendResponse(this->responses_[i]);
  }
  delete[] this->responses_;
}

}  // namespace parties
}  // namespace drivacy
