#include "DPPIR/sharing/additive.h"

// NOLINTNEXTLINE
#include "sodium.h"

namespace DPPIR {
namespace sharing {

// Helper XOR.
namespace {

void XOR(const char* tally, const char* share, char* dst, size_t sz) {
  const uint32_t* tally_ptr = reinterpret_cast<const uint32_t*>(tally);
  const uint32_t* share_ptr = reinterpret_cast<const uint32_t*>(share);
  uint32_t* dst_ptr = reinterpret_cast<uint32_t*>(dst);
  size_t ptr_sz = sz / sizeof(uint32_t);
  for (size_t i = 0; i < ptr_sz; i++) {
    dst_ptr[i] = tally_ptr[i] ^ share_ptr[i];
  }
}

}  // namespace

std::vector<preshare_t> GenerateAdditiveSecretShares(size_t n) {
  std::vector<preshare_t> shares;

  preshare_t acc{};  // 0-initialized.
  for (size_t i = 0; i < n - 1; i++) {
    shares.emplace_back();
    preshare_t& share = shares.back();
    randombytes_buf(share.data(), share.size());
    XOR(acc.data(), share.data(), acc.data(), share.size());
  }
  shares.push_back(acc);

  return shares;
}

void AdditiveReconstruct(const Response& tally, const preshare_t& share,
                         Response* target) {
  const char* tally_ptr = reinterpret_cast<const char*>(&tally);
  char* target_ptr = reinterpret_cast<char*>(target);
  XOR(tally_ptr, share.data(), target_ptr, share.size());
}

}  // namespace sharing
}  // namespace DPPIR
