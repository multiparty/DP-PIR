// Copyright 2020 multiparty.org

// Defines the type of a Batch.
//
// A batch represents an array of queries fed into a party during the protocol.
// The batch is responsible for shuffling these queries, and serializing them
// to be sent over the wire.
//
// The batch class is optimized so it never has any memory copies. When a query
// is read by a socket, it is processed along side a reference to where the
// partial result of it should be stored in the batch.
//
// The batch class remembers the shuffling order and can be used to de-shuffle
// the corresponding to responses when they are received.

#ifndef DRIVACY_TYPES_BATCH_H_
#define DRIVACY_TYPES_BATCH_H_

#include <cstring>
#include <utility>

#include "drivacy/protocol/query.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace types {

class Batch {
 public:
  Batch(uint32_t party_id, uint32_t party_count) : party_id_(party_id) {
    this->single_process_query_size_ =
        protocol::query::QuerySize(party_id, party_count);
    this->single_process_response_size_ = sizeof(uint64_t);
    this->processed_queries_ = nullptr;
    this->processed_responses_ = nullptr;
    this->query_stored_ = nullptr;
    this->responses_stored_ = nullptr;
    this->query_states_ = nullptr;
  }
  ~Batch() {
    assert(this->processed_queries_ == nullptr);
    assert(this->processed_responses_ == nullptr);
    assert(this->query_stored_ == nullptr);
    assert(this->responses_stored_ == nullptr);
    assert(this->query_states_ == nullptr);
  }

  // Reserve memory for the batch, called prior to processing starting.
  void Initialize(uint32_t query_count) {
    assert(this->processed_queries_ == nullptr);

    this->query_count_ = query_count;
    this->current_query_count_ = 0;
    this->current_response_count_ = 0;
    this->query_serialize_index_ = 0;
    this->response_serialize_index_ = 0;

    this->processed_queries_ =
        new unsigned char[query_count * this->single_process_query_size_];
    this->processed_responses_ =
        new unsigned char[query_count * this->single_process_response_size_];
    this->query_states_ = new QueryState[queyr_count];
    this->query_stored_ = new unsigned char[query_count / 8 + 1];
    memset(this->query_stored_, 0, query_count / 8 + 1);
  }

  // Returns a holder for adding/storing a query at the given index.
  QueryHolder AddQuery(uint32_t index) {
    assert(current_query_count_ < this->query_count_);

    // Store old index for de-shuffling later.
    QueryState *query_state = this->query_states + index;
    query_state->index = current_query_count_;
    current_query_count_++;

    // Mark index as stored.
    this->MarkStored(this->query_stored_, index);

    // Return a place holder.
    uint32_t actual_index = index * this->single_process_query_size_;
    unsigned char *query_ptr = this->processed_queries_ + actual_index;
    return QueryHolder{*reinterpret_cast<uint32_t *>(query_ptr),
                       query_state->preshare, query_ptr + sizeof(uint32_t)};
  }

  // Serializes all contigous available queries from the current serialization
  // location (initially 0).
  std::pair<unsigned char *, uint32_t> SerializeQuery() {
    uint32_t size = 0;
    unsigned char *ptr =
        this->processed_queries_ +
        (this->query_serialize_index_ * this->single_process_query_size_);
    while (this->query_serialize_index_ < this->query_count_ &&
           this->IsStored(this->query_stored_, this->query_serialize_index_)) {
      this->query_serialize_index_ += 1 size +=
          this->single_process_query_size_;
    }
    return std::make_pair(ptr, size);
  }

  // Call this when all queries have been processed and serialized.
  void AfterQuerySerialization() {
    delete this->processed_queries_;
    delete this->query_stored_;
  }

  // Get the stored query state to process its associated response.
  QueryState *GetQueryState() {
    assert(this->current_response_count_ < this->query_count_);
    QueryState *ptr = this->query_states_[this->current_response_count_];
    this->current_response_count_++;
    return ptr;
  }

  // Returns a holder for adding/storing a response at the given index.
  ResponseHolder AddResponse(uint32_t index) {
    // Mark index as stored.
    this->MarkStored(this->responses_stored_, index);

    // Return a place holder.
    uint32_t actual_index = index * this->single_process_response_size_;
    unsigned char *response_ptr = this->processed_responses_ + actual_index;
    return ResponseHolder{*reinterpret_cast<uint64_t *>(response_ptr)};
  }

  // Serializes contigious available responses from the current location.
  std::pair<unsigned char *, uint32_t> SerializeQuery() {
    uint32_t size = 0;
    unsigned char *ptr =
        this->processed_responses_ +
        (this->response_serialize_index_ * this->single_process_response_size_);
    while (this->response_serialize_index_ < this->query_count_ &&
           this->IsStored(this->responsed_stored_,
                          this->response_serialize_index_)) {
      this->response_serialize_index_ += 1 size +=
          this->single_process_response_size_;
    }
    return std::make_pair(ptr, size);
  }

  // Call this when all responses have been processed and serialized.
  void AfterResponseSerialization() {
    delete this->processed_responses_;
    delete this->responses_stored_;
    delete this->query_states_;
  }

 private:
  // The id of the party this batch belongs to.
  uint32_t party_id_;
  // The size that a single query will take in the "processed_queries_" array.
  // This is the size of onion cipher and the tally in bytes.
  uint32_t single_process_query_size_;
  // Similar to above but for responses.
  uint32_t single_process_response_size_;
  // The number of queries this batch can handle (batch does not resize).
  uint32_t query_count_;
  // All the processed queries will be stored here.
  // This is of size query_count_ * single_process_query_size_
  unsigned char[] processed_queries_;
  // Similar but for processed repsonses.
  unsigned char[] processed_responses;
  // Bit mask representing which locations in processed_queries_ have had
  // a query get stored in them. This is needed because processed_queries_
  // is filled in a random order.
  // This is of size |processed_queries_| / single_process_query_size_ / 8
  // such that *BIT* i==1 iff processed_queries[i * single_process_query_size_]
  // has a query stored in it.
  unsigned char[] query_stored_;
  // Similar but for responses, has the same length.
  unsigned char[] responsed_stored_;
  // Stores the state of the corresponding processed queries.
  // These are used to handle the corresponding responses later.
  QueryState[] query_states_;
  // Specifies the index of the next query to serialize.
  // This is needed because processed_queries_ is filled out of order but has
  // to be serialized sequentially.
  uint32_t query_serialize_index_;
  uint32_t response_serialize_index_;
  // Total count of queries stored in this batch so far
  // must always be <= query_count_.
  uint32_t current_query_count_;
  uint32_t current_response_count_;

  // Check if processed query has a stored query at
  // (index * single_process_query_size_).
  bool IsStored(unsigned char *bitmask, uint32_t index) {
    uint32_t stored_index = index / 8;
    unsigned char mask_index = index % 8;
    unsigned char mask = 1 << mask_index;
    unsigned char masked = bitmask[stored_index] & mask;
    return (masked >> mask_index) == 1;
  }

  // Check if processed query has a stored query at
  // (index * single_process_query_size_).
  void MarkStored(unsigned char *bitmask, uint32_t index) {
    uint32_t stored_index = index / 8;
    unsigned char mask_index = index % 8;
    unsigned char mask = 1 << mask_index;
    bitmask[stored_index] |= mask;
  }
};

}  // namespace types
}  // namespace drivacy

#endif  // DRIVACY_TYPES_BATCH_H_
