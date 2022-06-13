#ifndef DPPIR_TYPES_TYPES_H_
#define DPPIR_TYPES_TYPES_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
#include <type_traits>

// NOLINTNEXTLINE
#include "sodium.h"

#define SIG_T_SIZE 48  // 48 bytes.
#define PRESHARE_T_SIZE (4 + SIG_T_SIZE)
#define INCREMENTAL_PRIME 2147483647u  // 2^31 - 1.

namespace DPPIR {

// Entities.
using party_id_t = uint8_t;
using server_id_t = uint8_t;
using index_t = uint32_t;

// Database: key_t -> (value_t, sig_t).
using key_t = uint32_t;
using value_t = uint32_t;
using sig_t = std::array<char, SIG_T_SIZE>;

// Noise / sample type.
using sample_t = uint32_t;

// libsodium keys.
using pkey_t = std::array<unsigned char, crypto_box_PUBLICKEYBYTES>;
using skey_t = std::array<unsigned char, crypto_box_SECRETKEYBYTES>;

// Signatures.
static_assert(sizeof(sig_t) == sizeof(char) * SIG_T_SIZE);

// components.
using tag_t = uint64_t;
struct __attribute__((__packed__)) incremental_share_t {
  uint32_t x;
  uint32_t y;
};
using incremental_tally_t = uint32_t;
// using preshare_seed_t = int32_t;
using preshare_t = std::array<char, PRESHARE_T_SIZE>;
static_assert(sizeof(preshare_t) == sizeof(char) * PRESHARE_T_SIZE);
static_assert(PRESHARE_T_SIZE % sizeof(uint32_t) == 0);

// Offline message.
struct __attribute__((__packed__)) OfflineSecret {
  tag_t tag;
  tag_t next_tag;
  incremental_share_t share;
  preshare_t preshare;
};
static_assert(sizeof(OfflineSecret) == 24 + PRESHARE_T_SIZE);

// Online query.
struct __attribute__((__packed__)) Query {
  tag_t tag;
  incremental_tally_t tally;
};
static_assert(sizeof(Query) == 12);

// Online response.
struct __attribute__((__packed__)) Response {
  value_t value;
  sig_t sig;
};
static_assert(sizeof(Response) == SIG_T_SIZE + 4);
static_assert(sizeof(Response) == PRESHARE_T_SIZE);
static_assert(sizeof(Response) == sizeof(preshare_t));

// Debugging/Printing.
std::ostream& operator<<(std::ostream& o, const Query& q);
std::ostream& operator<<(std::ostream& o, const Response& r);

// Equality of responses.
bool operator==(const Response& l, const Response& r);
bool operator!=(const Response& l, const Response& r);

}  // namespace DPPIR

#endif  // DPPIR_TYPES_TYPES_H_
