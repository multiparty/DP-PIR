// Copyright 2020 multiparty.org

#ifndef DRIVACY_UTIL_FAKE_H_
#define DRIVACY_UTIL_FAKE_H_

#include <vector>

#include "drivacy/types/types.h"

namespace drivacy {
namespace fake {

std::vector<types::Message> FakeIt(uint32_t count);
types::CommonReference FakeItOnce();

}  // namespace fake
}  // namespace drivacy

#endif  // DRIVACY_UTIL_FAKE_H_
