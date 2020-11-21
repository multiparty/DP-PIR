// Copyright 2020 multiparty.org

// This file implements an efficient shuffling functionality
// based on knuth shuffle.
// The shuffler is incremental: it shuffles the queries as they come.
// The shuffler remembers the generated order, which can be used to
// de-shuffle responses as they come.

#include "drivacy/protocol/shuffle.h"

namespace drivacy {
namespace protocol {

void Shuffler::Initialize(uint32_t size) {
  // Reset indices.
  this->query_index_ = 0;
  this->response_index_ = 0;
  // Store batch size.
  this->size_ = size;
  this->total_size_ = size * this->parallelism_;
  this->shuffled_query_count_ = 0;
  this->deshuffled_response_count_ = 0;
  // Resize vectors.
  this->shuffled_queries_.resize(size);
  this->deshuffled_responses_.resize(size, types::Response(0));
  // clear maps.
  this->query_machine_ids_.clear();
  this->response_machine_ids_.clear();
  this->query_order_.clear();
  this->query_indices_.clear();
  this->response_indices_.clear();
  this->query_states_.clear();
}

}  // namespace protocol
}  // namespace drivacy
