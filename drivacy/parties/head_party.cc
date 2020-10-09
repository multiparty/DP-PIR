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
  this->client_socket_->Listen();
}

void HeadParty::SendQueries() {
  Party::SendQueries();
  // This blocks until as many responses are received as queries, and then
  // returns, causing SendQueries to return to Party::OnReceiveQuery,
  // which in tern returns into the client_socket_ and starts listening again
  // for queries from clients.
  Party::Listen();
}

void HeadParty::SendResponses() {
  for (uint32_t i = 0; i < this->batch_size_; i++) {
    types::Response response = this->shuffler_.NextResponse();
    this->client_socket_->SendResponse(response);
    response.Free();
  }
}

}  // namespace parties
}  // namespace drivacy
