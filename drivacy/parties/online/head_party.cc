// Copyright 2020 multiparty.org

// This file defines the "HeadParty" class. A specialized Party that interfaces
// with clients.
//
// All comments about deploying/running parties in simulation or deployment
// in drivacy/parties/party.h apply here too.

#include "drivacy/parties/online/head_party.h"

namespace drivacy {
namespace parties {
namespace online {

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
  this->listener_.ListenToQueries();
  // After all queries are handled, broadcast ready.
  this->intra_party_socket_.BroadcastQueriesReady();
  this->intra_party_socket_.CollectQueriesReady();
  // All queries are ready, we can move to the next party now!
  this->SendQueries();
  // Now we listen to incoming responses (from previous party or from parallel
  // machines).
  this->listener_.ListenToResponses();
  // After all responses are handled, broadcast ready.
  this->intra_party_socket_.BroadcastResponsesReady();
  this->intra_party_socket_.CollectResponsesReady();
  // All responses are ready, we can forward them to the previous party!
  this->OnEnd();
  this->SendResponses();
  // This returns back into client_socket->Listen() called in Start().
  this->client_socket_.Stop();
}

// Handle a query from the previous party (i.e. clients) normally, but
// immediately check if parallel machines sent us queries afterwards.
void HeadParty::OnReceiveQuery(const types::Query &query) {
  // Process query.
  Party::OnReceiveQuery(query);
  if (++this->processed_client_requests_ == this->initial_batch_size_) {
    this->OnStart();  // Start timing here now that the batch has been received.
    this->processed_client_requests_ = 0;
    this->Continue();
  } else {
    this->listener_.ListenToQueriesNonblocking();
  }
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
  for (uint32_t i = this->noise_size_; i < this->input_batch_size_; i++) {
    types::Response &response = this->shuffler_.NextResponse();
    this->client_socket_.SendResponse(response);
  }
}

}  // namespace online
}  // namespace parties
}  // namespace drivacy
