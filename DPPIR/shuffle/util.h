#ifndef DPPIR_SHUFFLE_UTIL_H_
#define DPPIR_SHUFFLE_UTIL_H_

#include <cstdint>
#include <cstdlib>
#include <utility>

namespace DPPIR {
namespace shuffle {
namespace util {

// Random sampling.
void seed(int s);
uint32_t random_bounded(uint32_t range);

// Shuffle.
template <typename T>
void shuffle(T arr[], size_t n) {
  for (size_t i = n; i > 1; i--) {
    size_t p = random_bounded(i);  // number in [0,i)
    std::swap(arr[i - 1], arr[p]);
  }
}

}  // namespace util
}  // namespace shuffle
}  // namespace DPPIR

#endif  // DPPIR_SHUFFLE_UTIL_H_
