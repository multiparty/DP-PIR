// Copyright 2020 multiparty.org

// C++ types used in the protocol.

#include "drivacy/types/types.h"

#include "drivacy/primitives/crypto.h"

namespace drivacy {
namespace types {

// IncomingQuery.
uint32_t IncomingQuery::Size(uint32_t party_id, uint32_t party_count) {
  return primitives::crypto::OnionCipherSize(party_id - 1, party_count) +
         sizeof(uint64_t);
}

IncomingQuery IncomingQuery::Deserialize(const unsigned char *buffer,
                                         uint32_t buffer_size) {
  return IncomingQuery(buffer, buffer_size - sizeof(uint64_t));
}

uint64_t IncomingQuery::tally() const {
  return *reinterpret_cast<const uint64_t *>(this->buffer_ +
                                             this->cipher_size_);
}

const unsigned char *IncomingQuery::cipher() const { return this->buffer_; }

// OutgoingQuery.
uint32_t OutgoingQuery::Size(uint32_t party_id, uint32_t party_count) {
  return primitives::crypto::OnionCipherSize(party_id, party_count) +
         sizeof(uint64_t);
}

OutgoingQuery::OutgoingQuery(uint32_t party_id, uint32_t party_count) {
  uint32_t msg_size = OutgoingQuery::Size(party_id, party_count);
  uint32_t buffer_size = msg_size + sizeof(QueryShare);
  // Bytes of [QueryShare, next onion cipher, tally].
  this->buffer_ = new unsigned char[buffer_size];
  this->cipher_size_ = msg_size - sizeof(uint64_t);
}

unsigned char *OutgoingQuery::buffer() const { return this->buffer_; }

void OutgoingQuery::set_tally(uint64_t tally) {
  unsigned char *ptr = this->buffer_ + sizeof(QueryShare) + this->cipher_size_;
  *reinterpret_cast<uint64_t *>(ptr) = tally;
}

void OutgoingQuery::set_preshare(uint64_t preshare) {
  this->preshare_ = preshare;
}

QueryState OutgoingQuery::query_state() const { return this->preshare_; }

QueryShare OutgoingQuery::share() const {
  return *reinterpret_cast<QueryShare *>(this->buffer_);
}

const unsigned char *OutgoingQuery::Serialize() const {
  return this->buffer_ + sizeof(QueryShare);
}

void OutgoingQuery::Free() { delete[] this->buffer_; }

// Response.
uint32_t Response::Size() { return sizeof(uint64_t); }

uint64_t Response::tally() const { return this->tally_; }

const unsigned char *Response::Serialize() const {
  return reinterpret_cast<const unsigned char *>(&this->tally_);
}

Response Response::Deserialize(const unsigned char *buffer) {
  const uint64_t *ptr = reinterpret_cast<const uint64_t *>(buffer);
  return Response(*ptr);
}

}  // namespace types
}  // namespace drivacy
