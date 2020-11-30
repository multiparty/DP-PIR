// Copyright 2020 multiparty.org

// C++ types used in the protocol.

#ifndef DRIVACY_TYPES_TYPES_H_
#define DRIVACY_TYPES_TYPES_H_

#include <cstdint>
#include <list>
#include <memory>
#include <unordered_map>
#include <utility>

namespace drivacy {
namespace types {

// The table we are doing PIR against.
using Table = std::unordered_map<uint32_t, uint32_t>;

// Primitive datatypes.
using Tag = uint32_t;
using CipherText = unsigned char *;

struct IncrementalSecretShare {
  uint32_t x;  // The additive component.
  uint32_t y;  // The multiplicative component.
};

// The struct representing the common reference message between a querier and
// a server. This message is installed at the server during the offline stage,
// and use during the online stage.
struct CommonReference {
  Tag next_tag;
  IncrementalSecretShare incremental_share;
  uint32_t preshare;
  // Constructors..
  explicit CommonReference() {}
  explicit CommonReference(Tag next_tag, IncrementalSecretShare share,
                           uint32_t preshare)
      : next_tag(next_tag), incremental_share(share), preshare(preshare) { }
};

// This is what gets encrypted.
struct Message {
  Tag tag;
  CommonReference reference;
  // Constructors..
  explicit Message() {}
  explicit Message(Tag tag, Tag next_tag, IncrementalSecretShare share,
                   uint32_t preshare)
      : tag(tag), reference(next_tag, share, preshare) {}
};

// A map representing all common references installed at a server.
using CommonReferenceMap = std::unordered_map<Tag, CommonReference>;

// How common references are onion-installed during the offline stage.
class OnionMessage {
 public:
  explicit OnionMessage(std::unique_ptr<unsigned char[]> buffer)
      : buffer_(std::move(buffer)) {}

  const Tag &tag() const;
  const CommonReference &common_reference() const;
  CipherText onion_cipher() const;
 private:
  std::unique_ptr<unsigned char[]> buffer_;
};

// Online stage datatypes.
struct Query {
  Tag tag;
  uint32_t tally;
};
using Response = uint32_t;
using BufferedQuery = const unsigned char *;
using BufferedResponse = const unsigned char *;

// A client state. This survives between a query and its response.
struct ClientState {
  // A copy of previously made queries for sanity checks.
  std::list<uint32_t> queries;
  // The corresponding stored preshare (matches queries by index).
  std::list<uint32_t> preshares;
};

}  // namespace types
}  // namespace drivacy

#endif  // DRIVACY_TYPES_TYPES_H_
