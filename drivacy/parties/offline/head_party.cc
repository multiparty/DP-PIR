// Copyright 2020 multiparty.org

// This file defines the "HeadParty" class. A specialized Party that interfaces
// with clients.
//
// All comments about deploying/running parties in simulation or deployment
// in drivacy/parties/party.h apply here too.

#include "drivacy/parties/offline/head_party.h"

namespace drivacy {
namespace parties {
namespace offline {

void HeadParty::Start() {
#ifdef DEBUG_MSG
  std::cout << "Frontend Starting ... " << machine_id_ << std::endl;
#endif
  // Handle everything up to noise sampling and shuffling.
  this->processed_client_requests_ = 0;
  this->OnReceiveBatchSize(this->initial_batch_size_);
  this->intra_party_socket_.CollectBatchSizes();
  // inject the noise queries in.
  this->InjectNoise();
  // Now we can listen to incoming queries (from previous party or from
  // machines parallel).
  this->first_query_ = true;
  this->client_socket_.Listen();
}

void HeadParty::Continue() {
#ifdef DEBUG_MSG
  std::cout << "Protocol continue (Frontend) " << machine_id_ << std::endl;
#endif
  // Now we can listen to incoming queries (from previous party or from
  // machines parallel).
  this->listener_.ListenToMessages();
  // After all queries are handled, broadcast ready.
  this->intra_party_socket_.BroadcastMessagesReady();
  this->intra_party_socket_.CollectMessagesReady();
  // All queries are ready, we can move to the next party now!
  this->SendMessages();
  // Save the common reference we got.
  this->SaveCommonReference();
  // This cause client_socket->Listen() called in Start() to return.
  this->client_socket_.Stop();
}

// Handle a query from the previous party (i.e. clients) normally, but
// immediately check if parallel machines sent us queries afterwards.
void HeadParty::OnReceiveMessage(const types::CipherText &message) {
  // Process query.
  std::cout << "Parent" << std::endl;
  Party::OnReceiveMessage(message);
  std::cout << "Back" << std::endl;
  if (processed_client_requests_ % 10000 == 0) {
    std::cout << this->processed_client_requests_ << " // "
              << this->initial_batch_size_ << std::endl;
  }

  if (++this->processed_client_requests_ == this->initial_batch_size_) {
    this->processed_client_requests_ = 0;
    this->Continue();
  } else {
    std::cout << "Listen to non blocking" << std::endl;
    this->listener_.ListenToMessagesNonblocking();
    std::cout << "Done listening" << std::endl;
  }
}

}  // namespace offline
}  // namespace parties
}  // namespace drivacy
