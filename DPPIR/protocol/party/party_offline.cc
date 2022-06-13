// NOLINTNEXTLINE
#include <chrono>
#include <iostream>

#include "DPPIR/onion/onion.h"
#include "DPPIR/protocol/party/party.h"

namespace DPPIR {
namespace protocol {

using millis = std::chrono::milliseconds;

void Party::CollectCiphers() {
  // Listen to all incoming offline messages.
  std::cout << "Listening for offline ciphers..." << std::endl;
  index_t count = this->input_count_;
  while (count > 0) {
    CipherLogicalBuffer& buffer = this->back_.ReadCiphers(count);
    for (char* cipher : buffer) {
      if (--count % PROGRESS_RATE == 0) {
        std::cout << "Progress " << count << std::endl;
      }
      this->ciphers_.PushLong(cipher);
    }
    buffer.Clear();
  }
}

void Party::CreateNoiseCiphers() {
  this->ciphers_.Initialize(this->noise_count_, this->input_count_);

  // Make offline secrets for noise queries.
  std::cout << "Creating secrets and ciphers for noise queries..." << std::endl;

  auto start_time = std::chrono::steady_clock::now();
  for (index_t i = 0; i < this->noise_count_; i++) {
    if ((i + 1) % PROGRESS_RATE == 0) {
      std::cout << "Progress " << (i + 1) << "/" << this->noise_count_
                << std::endl;
    }
    // Sample secrets.
    std::unique_ptr<OfflineSecret[]> secrets = this->MakeNoiseSecret(i);

    // Onion encrypt secrets.
    std::unique_ptr<char[]> cipher = onion::OnionEncrypt(
        secrets.get(), this->party_id_ + 1, this->party_count_, this->pkeys_);
    this->ciphers_.PushShort(cipher.get());
  }

  auto end_time = std::chrono::steady_clock::now();
  auto d = std::chrono::duration_cast<millis>(end_time - start_time).count();
  std::cout << "Took " << d << "ms" << std::endl;
}

void Party::InstallSecrets() {
  std::cout << "Decrypting offline ciphers..." << std::endl;

  auto start_time = std::chrono::steady_clock::now();
  index_t counter = 0;
  while (this->ciphers_.HasLong()) {
    if (++counter % PROGRESS_RATE == 0) {
      std::cout << "Progress " << counter << "/" << this->input_count_
                << std::endl;
    }

    // Decrypt cipher.
    char* cipher = this->ciphers_.PopLong();
    onion::OnionLayer layer = onion::OnionDecrypt(
        cipher, this->party_count_ - this->party_id_,
        this->party_config_.onion_pkey, this->party_config_.onion_skey);

    // Install secret.
    this->queries_state_.Store(layer.Msg());

    // Save next cipher layer to send.
    this->ciphers_.PushShort(layer.NextLayer());
  }

  auto end_time = std::chrono::steady_clock::now();
  auto d = std::chrono::duration_cast<millis>(end_time - start_time).count();
  std::cout << "Took " << d << "ms" << std::endl;
}

void Party::SendCiphers() {
  std::cout << "Sending offline ciphers..." << std::endl;
  // The batch now is decrypted with the noise ciphers added in.
  // However, it has not been shuffled yet.
  // We will shuffle it as we are sending it out to the next party.
  // Shuffler usually gives the shuffler index for elements one at a time.
  // We do not want to allocate more memory to store the batch after shuffling
  // and we want to avoid copies.
  // We flip the shuffler: rather than using it to get the index of element i,
  // we use it to find the element that should go to index i.
  // This is also uniform. However, it means the deshuffling order will not be
  // consistent. This is fine because we do not deshuffle in the offline stage.
  for (index_t i = 0; i < this->shuffled_count_; i++) {
    if ((i + 1) % PROGRESS_RATE == 0) {
      std::cout << "Progress " << (i + 1) << "/" << this->shuffled_count_
                << std::endl;
    }
    // Message at idx should be sent out now.
    index_t idx = this->lshuffler_.Shuffle(i);
    this->next_.SendCipher(this->ciphers_.GetShort(idx));
  }
  this->next_.FlushCiphers();

  // Clear memory.
  this->lshuffler_.FinishForward();
  this->ciphers_.Free();
}

void Party::StartOffline() {
  // Initialization.
  this->InitializeNoiseSamples();
  this->InitializeCounts();

  // The parties are initialized; start timing.
  // State initialization and noise cipher creation can be carried out after
  // CollectCipher(), but that will sequentialize noise cipher creation
  // along parties. By executing this first, we can perform this in parallel
  // across parties.
  auto start_time = std::chrono::steady_clock::now();

  // Initialize the offline states.
  this->queries_state_.Initialize(false);
  this->noise_state_.Initialize(this->party_count_ - this->party_id_ - 1,
                                this->noise_count_, true, false);

  // Create the noise ciphers.
  this->CreateNoiseCiphers();

  // Wait until the next server has initialized.
  this->next_.WaitForReady();
  this->back_.SendReady();

  // Stop timing while collecting ciphers from client.
  auto end_time = std::chrono::steady_clock::now();
  auto d = std::chrono::duration_cast<millis>(end_time - start_time).count();

  // Collect offline messages from previous party or client.
  this->CollectCiphers();

  // Batch has been received completely; resume timing.
  start_time = std::chrono::steady_clock::now();

  // Initialize the (offline) shuffler.
  this->InitializeShuffler();

  // Do the offline protocol.
  this->InstallSecrets();
  this->SendCiphers();

  // Offline protocol is over; Initialize the online stage.
  // Initialize the (online) shuffler.
  this->InitializeShuffler();

  // Create storage for online stage.
  // Only store tag for queries from previous parties.
  this->tags_.Initialize(this->input_count_);
  // Query batch includes both previous queries and new noise queries.
  this->queries_.Initialize(this->shuffled_count_);

  // Create noise queries using noise samples and noise state, and inject
  // them into the batch.
  this->InitializeNoiseQueries();

  // Wait until the next server has initialized.
  this->next_.WaitForReady();

  // Offline stage is done!
  end_time = std::chrono::steady_clock::now();
  d += std::chrono::duration_cast<millis>(end_time - start_time).count();
  std::cout << "Offline time: " << d << "ms" << std::endl;

  // Let previous party/client know that we are done.
  this->back_.SendReady();
}

void Party::SimulateOffline() {
  auto start_time = std::chrono::steady_clock::now();

  // Initialization: these steps should be done offline.
  this->InitializeNoiseSamples();
  this->InitializeCounts();

  // Initialize the (online) shuffler.
  this->InitializeShuffler();

  // Simulate the offline state.
  this->queries_state_.Initialize(true);
  this->noise_state_.Initialize(this->party_count_ - this->party_id_ - 1,
                                this->noise_count_, true, true);

  // Create storage for online stage.
  // Only store tag for queries from previous parties.
  this->tags_.Initialize(this->input_count_);
  // Query batch includes both previous queries and new noise queries.
  this->queries_.Initialize(this->shuffled_count_);

  // Create noise queries using noise samples and noise state, and inject
  // them into the batch.
  this->InitializeNoiseQueries();

  // Wait until the next server has initialized.
  this->next_.WaitForReady();
  this->back_.SendReady();

  auto end_time = std::chrono::steady_clock::now();
  auto d = std::chrono::duration_cast<millis>(end_time - start_time).count();
  std::cout << "Offline (simulated) time: " << d << "ms" << std::endl;
}

}  // namespace protocol
}  // namespace DPPIR
