#include <cstdint>

#ifndef PRIMITIVES_SECRET_SHARING_H_
#define PRIMITIVES_SECRET_SHARING_H_

namespace drivacy {
namespace primitives {

extern uint64_t PRIME;

uint64_t** generateShares(uint64_t query, uint64_t numparty);
uint64_t reconstructSecret(uint64_t** shares, uint64_t numparty);

}  // namespace primitives
}  // namespace drivacy

#endif  // PRIMITIVES_SECRET_SHARING_H_
