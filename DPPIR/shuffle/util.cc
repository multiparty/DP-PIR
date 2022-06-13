#include "DPPIR/shuffle/util.h"

namespace DPPIR {
namespace shuffle {
namespace util {

void seed(int s) { srand(s); }

// https://lemire.me/blog/2016/06/30/fast-random-shuffling/
uint32_t random_bounded(uint32_t range) {
  // NOLINTNEXTLINE
  uint64_t random32bit = static_cast<uint32_t>(rand());  // 32-bit random number
  uint64_t multiresult = random32bit * range;
  return multiresult >> 32;
}

}  // namespace util
}  // namespace shuffle
}  // namespace DPPIR
