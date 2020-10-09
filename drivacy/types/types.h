// Copyright 2020 multiparty.org

// C++ types used in the protocol.

#ifndef DRIVACY_TYPES_TYPES_H_
#define DRIVACY_TYPES_TYPES_H_

#include <cstdint>
#include <list>
#include <memory>
#include <unordered_map>

namespace drivacy {
namespace types {

using Table = std::unordered_map<uint64_t, uint64_t>;

struct IncrementalSecretShare {
  uint64_t x;  // The additive component.
  uint64_t y;  // The multiplicative component.
};

// The struct representing the plain text single share in a query.
// This is encrypted into bytes and decrypted from bytes in the crypto
// wrapper.
struct QueryShare {
  uint64_t x;
  uint64_t y;
  uint64_t preshare;
};

// The state to keep from processing a query to use during processing its
// response: just the preshare.
using QueryState = uint64_t;

// This class represents a single read-only incoming query deserialized
// from a socket buffer.
class IncomingQuery {
 public:
  // Static functions for figuring out sizes of serialized queries.
  static uint32_t Size(uint32_t party_id, uint32_t party_count);

  // De-serialization.
  // The query instance does not own the passed buffer, and will not clean
  // it up on destruction!
  static IncomingQuery Deserialize(const unsigned char *buffer,
                                   uint32_t buffer_size);

  // Accessors.
  uint64_t tally() const;
  const unsigned char *cipher() const;

 private:
  // Constructs a query from its components.
  IncomingQuery(const unsigned char *buffer, uint32_t cipher_size)
      : buffer_(buffer), cipher_size_(cipher_size) {}

  // The onion cipher followed by the tally as bytes.
  const unsigned char *buffer_;
  const uint32_t cipher_size_;
};

// A pre-allocated outgoing query that is the result of processing
// some incoming query.
class OutgoingQuery {
 public:
  // Static functions for figuring out sizes of serialized queries.
  static uint32_t Size(uint32_t party_id, uint32_t party_count);

  // Constructs an empty outgoing query (allocates memory for buffers).
  OutgoingQuery(uint32_t party_id, uint32_t party_count);

  // Accessors.
  unsigned char *buffer() const;
  void set_tally(uint64_t tally);
  void set_preshare(uint64_t preshare);
  QueryState query_state() const;
  QueryShare share() const;

  // After the placeholder has been filled, this produces a processed query!
  const unsigned char *Serialize() const;

  // Frees the underlying buffers in the instance.
  // It is unsafe to use any of the functions related to the buffer after this
  // function is called.
  // Unsafe functions: buffer(), set_Tally(), and share().
  // Safe functions: set_preshare() and query_state()
  void Free();

 private:
  // Stores the preshare_ from the corresponding IncomingQuery.
  uint64_t preshare_;
  // One decrypted layer of the onion cipher, followed by the remaining
  // encrypted layer, followed by the tally.
  unsigned char *buffer_;
  // Store the sizes of the different components in buffer_.
  uint32_t cipher_size_;
};

// This class represents a single response.
class Response {
 public:
  // Size of a response message in bytes.
  static uint32_t Size();
  // Construct a response given its content tally.
  explicit Response(uint64_t tally) : tally_(tally) {}
  // Accessors.
  uint64_t tally() const;
  // Serialization.
  const unsigned char *Serialize() const;
  static Response Deserialize(const unsigned char *buffer);
  // Freeing.
  void Free() {}

 private:
  uint64_t tally_;
};

// A client state. This survives between a query and its response.
struct ClientState {
  // A copy of previously made queries for sanity checks.
  std::list<uint64_t> queries;
  // The corresponding stored preshare (matches queries by index).
  std::list<uint64_t> preshares;
};

}  // namespace types
}  // namespace drivacy

#endif  // DRIVACY_TYPES_TYPES_H_
