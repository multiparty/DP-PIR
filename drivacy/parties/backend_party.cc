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

// Explicit protocol flow.
void BackendParty::Start() {
#ifdef DEBUG_MSG
  std::cout << "Backend Starting ... " << machine_id_ << std::endl;
#endif
  while (this->batches_ == 0 || this->batch_counter_++ < this->batches_) {
    this->inter_party_socket_.ReadBatchSize();
    this->listener_.ListenToQueries();
    this->SendResponses();
  }
}

// Store batch size and reset counters.
void BackendParty::OnReceiveBatchSize(uint32_t batch_size) {
#ifdef DEBUG_MSG
  std::cout << "On receive batch size (backend) " << machine_id_ << " = "
            << batch_size << std::endl;
#endif
  this->processed_queries_ = 0;
  this->input_batch_size_ = batch_size;
  this->output_batch_size_ = batch_size;
  this->responses_ = new types::Response[batch_size];
}

// Handle query and reply back immediately when a query is received.
void BackendParty::OnReceiveQuery(const types::IncomingQuery &query) {
#ifdef DEBUG_MSG
  std::cout << "On receive query (backend) " << machine_id_ << std::endl;
#endif
  // Process query creating a response, send it over socket.
  this->responses_[this->processed_queries_++] =
      protocol::backend::QueryToResponse(query, config_, table_);
}

// Send all responses after they are handled.
void BackendParty::SendResponses() {
#ifdef DEBUG_MSG
  std::cout << "Sending responses (backend) " << machine_id_ << std::endl;
#endif
  for (uint32_t i = 0; i < this->output_batch_size_; i++) {
    this->inter_party_socket_.SendResponse(this->responses_[i]);
  }
  delete[] this->responses_;
}

}  // namespace parties
}  // namespace drivacy
