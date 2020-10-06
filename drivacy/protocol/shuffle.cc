// Copyright 2020 multiparty.org

// This file implements an efficient shuffling functionality
// based on knuth shuffle.
// The shuffler is incremental: it shuffles the queries as they come.
// The shuffler remembers the generated order, which can be used to
// de-shuffle responses as they come.

#include "drivacy/protocol/shuffle.h"

#include <cassert>

#include "drivacy/primitives/util.h"

namespace drivacy {
namespace protocol {

void Shuffler::Initialize(uint32_t size) {
  this->query_index_ = 0;
  this->response_index_ = 0;
  this->size_ = size;
}

bool Shuffler::ShuffleQuery(types::OutgoingQuery query) {
  assert(this->query_index_ < this->size_);

  // If this->query_index_ should have been swapped already, but was not
  // because the query was not yet available.
  if (this->shuffling_swaps_.count(this->query_index_)) {
    uint32_t swap = this->shuffling_swaps_.at(this->query_index_);
    this->shuffling_swaps_.erase(this->query_index_);
    this->shuffled_queries_.insert({swap, query});
    query = this->shuffled_queries_.at(this->query_index_);
  }

  // Find a new position to swap with.
  uint32_t pos = primitives::util::Rand32(this->query_index_, this->size_);
  if (this->shuffled_queries_.count(pos)) {
    types::OutgoingQuery tmp = this->shuffled_queries_.at(pos);
    this->shuffled_queries_.at(pos) = query;
    this->shuffled_queries_.insert_or_assign(this->query_index_, tmp);
  } else {
    this->shuffled_queries_.erase(this->query_index_);
    this->shuffled_queries_.insert({pos, query});
    if (pos != this->query_index_) {
      this->shuffling_swaps_.insert({pos, this->query_index_});
    }
  }

  // Determine inverse shuffling order.
  uint32_t index_order = this->query_index_;
  uint32_t pos_order = pos;
  if (this->shuffling_order_.count(this->query_index_)) {
    index_order = this->shuffling_order_[this->query_index_];
  }
  if (this->shuffling_order_.count(pos)) {
    pos_order = this->shuffling_order_[pos];
  }
  this->shuffling_order_[pos] = index_order;
  this->shuffling_order_[this->query_index_] = pos_order;

  // Increment index.
  this->query_index_ = (this->query_index_ + 1) % this->size_;
  return this->query_index_ == 0;
}

bool Shuffler::DeshuffleResponse(types::Response response) {
  // Find deshuffled index.
  uint32_t pos = this->shuffling_order_.at(this->response_index_);

  // Insert response and associated query state at deshuffled index.
  this->deshuffled_responses_.insert({pos, response});

  // Free memory.
  this->shuffling_order_.erase(this->response_index_);

  // Increment index.
  this->response_index_ = (this->response_index_ + 1) % this->size_;
  return this->response_index_ == 0;
}

types::OutgoingQuery Shuffler::NextQuery() {
  types::OutgoingQuery result = this->shuffled_queries_.at(this->query_index_);
  // Increment Index.
  this->query_index_ = (this->query_index_ + 1) % this->size_;
  return result;
}

types::QueryState Shuffler::NextQueryState() {
  types::QueryState query_state =
      this->shuffled_queries_.at(this->response_index_).query_state();
  // Free memory.
  this->shuffled_queries_.erase(this->response_index_);
  // Dont increment index: DeshuffleResponse() will do it.
  return query_state;
}

types::Response Shuffler::NextResponse() {
  types::Response result =
      this->deshuffled_responses_.at(this->response_index_);
  // Free memory.
  this->deshuffled_responses_.erase(this->response_index_);
  // Increment index.
  this->response_index_ = (this->response_index_ + 1) % this->size_;
  return result;
}

}  // namespace protocol
}  // namespace drivacy
