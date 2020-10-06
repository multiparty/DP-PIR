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
  Party::Listen();
  this->client_socket_->Listen();
}

void HeadParty::SendResponses() {
  for (uint32_t i = 0; i < this->batch_size_; i++) {
    this->client_socket_->SendResponse(this->shuffler_.NextResponse());
  }
}

}  // namespace parties
}  // namespace drivacy
