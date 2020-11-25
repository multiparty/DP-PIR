// Copyright 2020 multiparty.org

// This file defines the "HeadParty" class. A specialized Party that interfaces
// with clients.
//
// All comments about deploying/running parties in simulation or deployment
// in drivacy/parties/party.h apply here too.

#include "drivacy/parties/head_party.h"

namespace drivacy {
namespace parties {

void HeadParty::Listen() {
  // Listen only for queries from clients, do not listen to responses yet.
  // We will listen to responses *after* a query batch is processed,
  // and after the response batch is processed, we will go back
  // to listening for queries from clients.
  this->OnReceiveBatchSize(this->initial_batch_size_);
  this->client_socket_->Listen();
}

void HeadParty::SendQueries() {
  Party::SendQueries();
  // This blocks until as many responses are received as queries, and then
  // returns, causing SendQueries to return to Party::OnReceiveQuery,
  // which in turn returns into the client_socket_ and starts listening again
  // for queries from clients.
  Party::Listen();
}

void HeadParty::SendResponses() {
#ifdef DEBUG_MSG
  std::cout << "Send responses! (Frontend) " << machine_id_ << std::endl;
#endif
  // Ignore noise added in by this party.
  for (uint32_t i = 0; i < this->noise_size_; i++) {
    this->shuffler_.NextResponse();
  }
  // Send in remaining responses.
  for (uint32_t i = this->noise_size_; i < this->batch_size_; i++) {
    types::Response &response = this->shuffler_.NextResponse();
    this->client_socket_->SendResponse(response);
  }
  this->OnReceiveBatchSize(this->initial_batch_size_);
}

}  // namespace parties
}  // namespace drivacy
