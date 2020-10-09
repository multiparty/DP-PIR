// Copyright 2020 multiparty.org

// This file defines the "BackendParty" class. A specialized Party that
// sits at the deepest level in the protocol chain and responds to queries
// through the table.
//
// All comments about deploying/running parties in simulation or deployment
// in drivacy/parties/party.h apply here too.

#include "drivacy/parties/backend_party.h"

#include "drivacy/protocol/backend.h"

namespace drivacy {
namespace parties {

void BackendParty::OnReceiveQuery(const types::IncomingQuery &query) {
  // Process query creating a response, send it over socket.
  types::Response response =
      protocol::backend::QueryToResponse(query, config_, table_);
  this->socket_->SendResponse(response);
  response.Free();

  // Flush socket when everything is done.
  this->processed_queries_++;
  if (this->processed_queries_ == this->batch_size_) {
    this->processed_queries_ = 0;
    this->socket_->FlushResponses();
  }
}

}  // namespace parties
}  // namespace drivacy
