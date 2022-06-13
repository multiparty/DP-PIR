// NOLINTNEXTLINE
#include <chrono>
#include <iostream>

#include "DPPIR/onion/onion.h"
#include "DPPIR/protocol/parallel_party/parallel_party.h"

namespace DPPIR {
namespace protocol {

using millis = std::chrono::milliseconds;

/*
 * Online protocol.
 */
void ParallelParty::CollectQueries() {
  std::cout << "Listening for queries..." << std::endl;
  index_t read = this->noise_count_;
  while (read < this->in_queries_.Capacity()) {
    size_t remaining = this->in_queries_.Capacity() - read;
    LogicalBuffer<Query>& buffer = this->back_.ReadQueries(remaining);
    for (Query& in_query : buffer) {
      // Store tag for response handling.
      this->in_tags_.PushBack(in_query.tag);
      Query& out_query = this->in_queries_[read++];
      // Handle query.
      this->HandleQuery(in_query, &out_query);
    }
    buffer.Clear();
  }
}

void ParallelParty::SendQueries() {
  std::cout << "Sending queries..." << std::endl;
  for (Query& query : this->out_queries_) {
    this->next_.SendQuery(query);
  }
  this->next_.FlushQueries();
  this->out_queries_.Free();
}

void ParallelParty::CollectResponses() {
  std::cout << "Listening for responses..." << std::endl;
  index_t noise = this->noise_from_sibling_prefixsum_[this->server_count_ - 1];
  index_t non_noise = this->shuffled_count_ - noise;
  this->in_responses_.Initialize(non_noise);

  index_t read = 0;
  while (read < this->shuffled_count_) {
    size_t remaining = this->shuffled_count_ - read;
    LogicalBuffer<Response>& buffer = this->next_.ReadResponses(remaining);
    for (Response& in_response : buffer) {
      // Deshuffle response.
      index_t target = this->lshuffler_.Deshuffle(read++);
      server_id_t source = this->pshuffler_.FindSourceOf(target);
      index_t start = this->pshuffler_.PrefixSumCountFromServer(source);
      if (target >= start + this->noise_from_sibling_counts_[source]) {
        index_t idx = target - this->noise_from_sibling_prefixsum_[source];
        this->in_responses_[idx] = in_response;
      }
    }
    buffer.Clear();
  }

  // Free memory.
  this->lshuffler_.FinishBackward();
}

void ParallelParty::SendResponses() {
  std::cout << "Sending responses..." << std::endl;
  for (Response& response : this->out_responses_) {
    this->back_.SendResponse(response);
  }
  this->back_.FlushResponses();
  this->out_responses_.Free();
}

// The online protocol steps.
void ParallelParty::StartOnline() {
  // Collect queries from previous party or client.
  this->CollectQueries();

  // Wait until siblings collected their batches as well!
  this->siblings_.BroadcastReady();
  this->siblings_.WaitForReady();

  // Batch has been received completely.
  // Start timing.
  auto start_time = std::chrono::steady_clock::now();

  this->ShuffleQueries();      // Shuffle (across servers then locally).
  this->SendQueries();         // Send to next party.
  this->CollectResponses();    // Read responses from next party.
  this->DeshuffleResponses();  // Deshuffle (across servers and locally).

  // Wait until siblings finished everything (to avoid timing side channel).
  this->siblings_.BroadcastReady();
  this->siblings_.WaitForReady();

  // Responses completely handled.
  auto end_time = std::chrono::steady_clock::now();
  auto d = std::chrono::duration_cast<millis>(end_time - start_time).count();
  std::cout << "Online time: " << d << "ms" << std::endl;

  // Send responses to previous party or client.
  this->SendResponses();  // Send responses to previous party or client.
}

}  // namespace protocol
}  // namespace DPPIR
