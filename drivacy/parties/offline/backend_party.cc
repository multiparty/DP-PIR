// Copyright 2020 multiparty.org

// This file defines the "BackendParty" class. A specialized Party that
// sits at the deepest level in the protocol chain and responds to queries
// through the table.
//
// All comments about deploying/running parties in simulation or deployment
// in drivacy/parties/party.h apply here too.

#include "drivacy/parties/offline/backend_party.h"

#include "drivacy/primitives/crypto.h"

namespace drivacy {
namespace parties {
namespace offline {

// Explicit protocol flow.
void BackendParty::Start() {
#ifdef DEBUG_MSG
  std::cout << "Backend Starting ... " << machine_id_ << std::endl;
#endif
  this->inter_party_socket_.ReadBatchSize();
  this->first_query_ = true;
  this->listener_.ListenToMessages();
  this->SaveCommonReference();
}

// Store batch size and reset counters.
void BackendParty::OnReceiveBatchSize(uint32_t batch_size) {
#ifdef DEBUG_MSG
  std::cout << "On receive batch size (backend) " << machine_id_ << " = "
            << batch_size << std::endl;
#endif
  this->input_batch_size_ = batch_size;
  this->output_batch_size_ = batch_size;
}

// Processing incoming messages.
void BackendParty::OnReceiveMessage(const types::CipherText &message) {
#ifdef DEBUG_MSG
  std::cout << "On receive message (backend) " << machine_id_ << std::endl;
#endif
  // Start timing as soon as first query is received.
  if (this->first_query_) {
    this->first_query_ = false;
    this->OnStart();
  }

  // Process message.
  types::OnionMessage onion_message =
      primitives::crypto::SingleLayerOnionDecrypt(this->party_id_, message,
                                                  this->config_);

  // Add the common reference to the map.
  // TODO(babman):
  // assert(this->common_references_.count(onion_message.tag()) == 0);
  // this->common_references_.insert(
  //    {onion_message.tag(), onion_message.common_reference()});
}

}  // namespace offline
}  // namespace parties
}  // namespace drivacy
