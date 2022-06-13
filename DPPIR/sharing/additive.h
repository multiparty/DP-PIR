// Our Incremental and non-malleable secret sharing scheme.

#ifndef DPPIR_SHARING_ADDITIVE_H_
#define DPPIR_SHARING_ADDITIVE_H_

#include <vector>

#include "DPPIR/types/types.h"

namespace DPPIR {
namespace sharing {

// Creates n secret shares of zero.
std::vector<preshare_t> GenerateAdditiveSecretShares(size_t n);

// (Partial/incremental) reconstruction: given a current tally and a share,
// the function returns a new tally that includes this share.
// If this function is called on all the shares returned by
// "GenerateAdditiveSecretShares(n)", in order, such that the
// resulting tally of every call is fed to the next, and the tally was initially
// x, then the final output will be equal to x.
void AdditiveReconstruct(const Response& tally, const preshare_t& share,
                         Response* target);

}  // namespace sharing
}  // namespace DPPIR

#endif  // DPPIR_SHARING_ADDITIVE_H_
