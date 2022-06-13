// NOLINTNEXTLINE
#include <chrono>
#include <iostream>

#include "DPPIR/onion/onion.h"
#include "DPPIR/protocol/parallel_party/parallel_party.h"

namespace DPPIR {
namespace protocol {

using millis = std::chrono::milliseconds;

void ParallelParty::CollectCiphers() {
  // Listen to all incoming offline messages.
  std::cout << "Listening for offline ciphers..." << std::endl;
  size_t count = this->input_count_;
  while (count > 0) {
    CipherLogicalBuffer& buffer = this->back_.ReadCiphers(count);
    for (char* cipher : buffer) {
      if (--count % PROGRESS_RATE == 0) {
        std::cout << "Progress " << count << std::endl;
      }
      this->in_ciphers_.PushLong(cipher);
    }
    buffer.Clear();
  }
}

void ParallelParty::CreateNoiseCiphers() {
  this->in_ciphers_.Initialize(this->noise_count_, this->input_count_);

  // Make offline secrets for noise queries.
  std::cout << "Creating secrets and ciphers for noise queries..." << std::endl;

  auto start_time = std::chrono::steady_clock::now();
  for (index_t i = 0; i < this->noise_count_; i++) {
    if ((i + 1) % PROGRESS_RATE == 0) {
      std::cout << "Progress " << (i + 1) << " / " << this->noise_count_
                << std::endl;
    }
    // Sample secrets.
    std::unique_ptr<OfflineSecret[]> secrets = this->MakeNoiseSecret(i);

    // Onion encrypt secrets.
    std::unique_ptr<char[]> cipher = onion::OnionEncrypt(
        secrets.get(), this->party_id_ + 1, this->party_count_, this->pkeys_);
    this->in_ciphers_.PushShort(cipher.get());
  }

  auto end_time = std::chrono::steady_clock::now();
  auto d = std::chrono::duration_cast<millis>(end_time - start_time).count();
  std::cout << "Took " << d << "ms" << std::endl;
}

void ParallelParty::InstallSecrets() {
  std::cout << "Decrypting offline ciphers..." << std::endl;

  auto start_time = std::chrono::steady_clock::now();
  index_t counter = 0;
  while (this->in_ciphers_.HasLong()) {
    if (++counter % PROGRESS_RATE == 0) {
      std::cout << "Progress " << counter << "/" << this->input_count_
                << std::endl;
    }

    // Decrypt cipher.
    char* cipher = this->in_ciphers_.PopLong();
    onion::OnionLayer layer = onion::OnionDecrypt(
        cipher, this->party_count_ - this->party_id_,
        this->party_config_.onion_pkey, this->party_config_.onion_skey);

    // Install secret.
    this->queries_state_.Store(layer.Msg());

    // Save next cipher layer to send.
    this->in_ciphers_.PushShort(layer.NextLayer());
  }

  auto end_time = std::chrono::steady_clock::now();
  auto d = std::chrono::duration_cast<millis>(end_time - start_time).count();
  std::cout << "Took " << d << "ms" << std::endl;
}

void ParallelParty::SendCiphers() {
  std::cout << "Sending offline ciphers..." << std::endl;
  index_t counter = 0;
  for (char* cipher : this->out_ciphers_) {
    if (++counter % PROGRESS_RATE == 0) {
      std::cout << "Progress " << counter << "/" << this->shuffled_count_
                << std::endl;
    }
    this->next_.SendCipher(cipher);
  }
  this->next_.FlushCiphers();
  this->out_ciphers_.Free();
}

void ParallelParty::BroadcastSecrets() {
  std::cout << "Broadcasting secrets..." << std::endl;

  // Figure out how many secrets to read from each server.
  this->siblings_.BroadcastCount(this->queries_state_.size());

  index_t total_count = 0;
  ServersMap<index_t> from_counts(this->server_id_, this->server_count_);
  for (server_id_t id = 0; id < this->server_count_; id++) {
    if (id != this->server_id_) {
      from_counts[id] = this->siblings_.ReadCount(id);
      total_count += from_counts[id];
    }
  }

  // Broadcast all secrets with parallel servers.
  using SecretPair = PartyState::const_iterator::value_type;
  PartyState copy = this->queries_state_;
  size_t poll_rate = POLL_RATE / sizeof(OfflineSecret);
  this->SendAndPoll<OfflineSecret, SecretPair, PartyState>(
      &copy, poll_rate, copy.size(), total_count, from_counts,
      std::function<void(const SecretPair&)>([this](const SecretPair& s) {
        this->siblings_.BroadcastSecret({s.first, s.second.next_tag,
                                         s.second.incremental,
                                         s.second.preshare});
      }),
      std::function<index_t(server_id_t, index_t)>(
          [this](server_id_t source, index_t remaining) {
            LogicalBuffer<OfflineSecret>& buffer =
                this->siblings_.ReadSecrets(source, remaining);
            for (OfflineSecret& secret : buffer) {
              this->queries_state_.Store(secret);
            }
            index_t size = buffer.Size();
            this->received_from_sibling_counts_[source] += size;
            buffer.Clear();
            return size;
          }));
}

void ParallelParty::StartOffline() {
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

  // Do the offline protocol.
  this->CreateNoiseCiphers();

  // Wait until the next server and siblings have initialized.
  this->next_.WaitForReady();
  this->siblings_.BroadcastReady();
  this->siblings_.WaitForReady();
  this->back_.SendReady();

  // Stop timing while collecting ciphers from client.
  auto end_time = std::chrono::steady_clock::now();
  auto d = std::chrono::duration_cast<millis>(end_time - start_time).count();

  // Collect offline messages from previous party or client.
  this->CollectCiphers();

  // Wait for siblings.
  this->siblings_.BroadcastReady();
  this->siblings_.WaitForReady();

  // Batch has been received completely; continue timing.
  start_time = std::chrono::steady_clock::now();

  // Initialize the (offline) shuffler.
  this->InitializeShufflers();
  this->InstallSecrets();
  this->ShuffleCiphers();
  this->SendCiphers();
  this->BroadcastSecrets();  // Each sibling server must have ALL secrets.

  // Offline protocol is over; Initialize the online stage.
  // Initialize the (online) shuffler.
  this->InitializeShufflers();

  // Create storage for online stage.
  // Only store tag for queries from previous parties.
  this->in_tags_.Initialize(this->input_count_);
  // Query batch includes both previous queries and new noise queries.
  this->in_queries_.Initialize(this->input_count_ + this->noise_count_);
  this->out_queries_.Initialize(this->shuffled_count_);

  // Create noise queries using noise samples and noise state, and inject
  // them into the batch.
  this->InitializeNoiseQueries();

  // Wait until the next server has initialized.
  std::cout << "Waiting for next party..." << std::endl;
  this->next_.WaitForReady();
  std::cout << "Waiting for siblings..." << std::endl;
  this->siblings_.BroadcastReady();
  this->siblings_.WaitForReady();
  this->back_.SendReady();

  // Offline stage is done!
  end_time = std::chrono::steady_clock::now();
  d += std::chrono::duration_cast<millis>(end_time - start_time).count();
  std::cout << "Offline time: " << d << "ms" << std::endl;
}

// Simulated offline protocol.
void ParallelParty::SimulateOffline() {
  std::cout << "Simulating offline stage..." << std::endl;
  auto start_time = std::chrono::steady_clock::now();

  // Initialization: these steps should be done offline.
  this->InitializeNoiseSamples();
  this->InitializeCounts();

  // Initialize the (online) shufflers.
  this->InitializeShufflers();

  // Simulate the offline state.
  this->queries_state_.Initialize(true);
  this->noise_state_.Initialize(this->party_count_ - this->party_id_ - 1,
                                this->noise_count_, true, true);

  // Create storage for online stage.
  // Only store tag for queries from previous parties.
  this->in_tags_.Initialize(this->input_count_);
  // Query batch includes both previous queries and new noise queries.
  this->in_queries_.Initialize(this->input_count_ + this->noise_count_);
  this->out_queries_.Initialize(this->shuffled_count_);

  // Create noise queries using noise samples and noise state, and inject
  // them into the batch.
  this->InitializeNoiseQueries();

  // Wait until the next server has initialized.
  this->next_.WaitForReady();
  this->siblings_.BroadcastReady();
  this->siblings_.WaitForReady();
  this->back_.SendReady();

  auto end_time = std::chrono::steady_clock::now();
  auto d = std::chrono::duration_cast<millis>(end_time - start_time).count();
  std::cout << "Offline (simulated) time: " << d << "ms" << std::endl;
}

}  // namespace protocol
}  // namespace DPPIR
