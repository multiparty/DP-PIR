// Our Incremental and non-malleable secret sharing scheme.

#ifndef DPPIR_SHARING_INCREMENTAL_H_
#define DPPIR_SHARING_INCREMENTAL_H_

#include <vector>

#include "DPPIR/types/types.h"

namespace DPPIR {
namespace sharing {

// Generates random preshares of an value that is yet to be determined.
std::vector<incremental_share_t> PreIncrementalSecretShares(size_t n);

// Compute the tally to secret share query using the given preshares.
incremental_tally_t GenerateIncrementalTally(
    key_t query, const std::vector<incremental_share_t> &preshares);

// (Partial/incremental) reconstruction: given a current tally and a share,
// the function returns a new tally that includes this share.
// If this function is called on all the shares returned by
// "PreIncrementalSecretShares(n)" and the tally returned by
// "GenerateIncrementalSecretShares(query, shares)", such that the
// resulting tally of every call is fed to the next, then the final output will
// be equal to query (complete reconstruction).
incremental_tally_t IncrementalReconstruct(incremental_tally_t tally,
                                           const incremental_share_t &share);

}  // namespace sharing
}  // namespace DPPIR

#endif  // DPPIR_SHARING_INCREMENTAL_H_
