#include <iostream>
#include <memory>

#include "DPPIR/onion/onion.h"
#include "DPPIR/protocol/client/client.h"

namespace DPPIR {
namespace protocol {

void Client::StartOffline(index_t count) {
  this->queries_count_ = count;

  // Make count queries.
  std::cout << "Offline Queries: " << count << std::endl;
  this->next_.SendCount(count);
  this->next_.WaitForReady();

  // Initialize the state.
  this->state_.Initialize(this->party_count_, count, false, false);

  // Sample offline secrets.
  for (index_t i = 0; i < count; i++) {
    // Sample secrets.
    std::unique_ptr<OfflineSecret[]> secrets = this->MakeSecret(i);

    // Onion encrypt secrets.
    std::unique_ptr<char[]> cipher =
        onion::OnionEncrypt(secrets.get(), 0, this->party_count_, this->pkeys_);

    // Send to first party via socket.
    this->next_.SendCipher(cipher.get());
  }

  // Wait until offline stage is finished before starting online.
  this->next_.FlushCiphers();
  this->next_.WaitForReady();
}

void Client::SimulateOffline(index_t count) {
  // Simulate offline state.
  this->state_.Initialize(this->party_count_, count, false, true);

  // Send count to party and wait for ready.
  this->next_.SendCount(count);
  this->next_.WaitForReady();
}

}  // namespace protocol
}  // namespace DPPIR
