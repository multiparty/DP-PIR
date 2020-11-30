// Copyright 2020 multiparty.org

#include "drivacy/util/fake.h"

namespace drivacy {
namespace fake {

std::vector<types::Message> FakeIt(uint32_t count) {
  std::vector<types::Message> result;
  for (uint32_t i = 0; i < count; i++) {
    result.emplace_back(0, 0, types::IncrementalSecretShare{0, 1}, 0);
  }
  return result;
}

types::CommonReference FakeItOnce() {
  return types::CommonReference{0, {0, 1}, 0};
}

}  // namespace fake
}  // namespace drivacy
