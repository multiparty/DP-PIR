// NOLINTNEXTLINE
#include <chrono>
#include <iostream>

#include "DPPIR/protocol/party/party.h"

namespace DPPIR {
namespace protocol {

using millis = std::chrono::milliseconds;

// Listen to all incoming queries from the previous party.
// Shuffle them as they come.
void Party::CollectQueries() {
  std::cout << "Listening for queries..." << std::endl;
  index_t read = this->noise_count_;
  while (read < this->shuffled_count_) {
    index_t remaining = this->shuffled_count_ - read;
    LogicalBuffer<Query>& buffer = this->back_.ReadQueries(remaining);
    for (Query& in_query : buffer) {
      // Store tag for response handling.
      this->tags_.PushBack(in_query.tag);
      // Shuffle query.
      index_t target = this->lshuffler_.Shuffle(read++);
      Query& out_query = this->queries_[target];
      // Handle query.
      this->HandleQuery(in_query, &out_query);
    }
    buffer.Clear();
  }
}

// Send shuffled queries to the next party.
void Party::SendQueries() {
  // Send shuffled queries.
  std::cout << "Sending queries..." << std::endl;
  for (Query& query : this->queries_) {
    this->next_.SendQuery(query);
  }
  this->next_.FlushQueries();

  // Clear memory.
  this->lshuffler_.FinishForward();
  this->queries_.Free();
}

// Collect responses from the next party.
// Deshuffle them as they are come.
void Party::CollectResponses() {
  // Listen to responses.
  std::cout << "Listening for responses..." << std::endl;
  this->responses_.Initialize(this->input_count_);

  index_t read = 0;
  while (read < this->shuffled_count_) {
    index_t remaining = this->shuffled_count_ - read;
    LogicalBuffer<Response>& buffer = this->next_.ReadResponses(remaining);
    for (Response& in_response : buffer) {
      // Deshuffle response.
      index_t target = this->lshuffler_.Deshuffle(read++);
      // We do not need to handle responses to noise queries that we inject.
      if (target >= this->noise_count_) {
        index_t target_index = target - this->noise_count_;
        // Handle the response and store it at deshuffled index.
        Response& out_response = this->responses_[target_index];
        tag_t& tag = this->tags_[target_index];
        this->HandleResponse(tag, in_response, &out_response);
      }
    }
    buffer.Clear();
  }

  // Free memory.
  this->lshuffler_.FinishBackward();
  this->tags_.Free();
}

// Send responses to the previous party.
void Party::SendResponses() {
  std::cout << "Sending responses..." << std::endl;
  for (Response& response : this->responses_) {
    this->back_.SendResponse(response);
  }
  this->back_.FlushResponses();

  // Clean up.
  this->responses_.Free();
}

void Party::StartOnline() {
  // Collect queries from previous party or client.
  this->CollectQueries();

  // Batch has been received completely.
  // Start timing.
  auto start_time = std::chrono::steady_clock::now();

  // Timed portion.
  this->SendQueries();       // Handle the queries and send them to next party.
  this->CollectResponses();  // Collect and handle responses from next party.

  // Responses completely handled.
  auto end_time = std::chrono::steady_clock::now();
  auto d = std::chrono::duration_cast<millis>(end_time - start_time).count();
  std::cout << "Online time: " << d << "ms" << std::endl;

  // Send responses to previous party or client.
  this->SendResponses();
}

}  // namespace protocol
}  // namespace DPPIR
