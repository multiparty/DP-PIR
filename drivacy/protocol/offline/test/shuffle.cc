// Copyright 2020 multiparty.org

// Testing the shuffling implementation!

#include "drivacy/protocol/offline/shuffle.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <list>
#include <vector>
#include <unordered_set> 

#include "absl/status/status.h"
#include "drivacy/types/types.h"

#define PARTY_COUNT 3
#define PAD_SIZE 136 - 4

struct TestMessage {
  uint32_t id;
  char pad[PAD_SIZE];

  explicit TestMessage(uint32_t id) : id(id) {}
  drivacy::types::CipherText Cast() {
    return reinterpret_cast<drivacy::types::CipherText>(this);
  }
  static const TestMessage *Upcast(drivacy::types::CipherText ct) {
    return reinterpret_cast<const TestMessage *>(ct);
  }
};

absl::Status Test(uint32_t parallelism,
                  const std::unordered_map<uint32_t, uint32_t> &sizes) {
  std::cout << "Testing..." << std::endl;

  // input
  std::vector<TestMessage> input;
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    for (uint32_t i = 0; i < sizes.at(machine_id); i++) {
      input.emplace_back(input.size());
    }
  }

  // Setup shufflers.
  std::cout << "Initializing..." << std::endl;
  std::vector<drivacy::protocol::offline::Shuffler> shufflers;
  shufflers.reserve(parallelism);
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    shufflers.emplace_back(1, machine_id, PARTY_COUNT, parallelism);
    bool status = false;
    for (uint32_t o = 1; o <= parallelism; o++) {
      assert(!status);
      status = shufflers[machine_id - 1].Initialize(o, sizes.at(o));
    }
    assert(status);
    shufflers[machine_id - 1].PreShuffle();
  }

  // Begin shuffling.
  std::unordered_map<
      uint32_t,
      std::unordered_map<uint32_t, std::list<drivacy::types::CipherText>>>
      exchange;
  // Phase 1.
  uint32_t index = 0;
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    drivacy::protocol::offline::Shuffler &shuffler = shufflers[machine_id - 1];
    for (uint32_t i = 0; i < sizes.at(machine_id); i++) {
      TestMessage &message = input.at(index++);
      uint32_t m = shuffler.MachineOfNextMessage();
      // Exchange...
      exchange[m][machine_id].push_back(message.Cast());
    }
  }

  // Phase 2.
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    bool done = false;
    drivacy::protocol::offline::Shuffler &shuffler = shufflers[machine_id - 1];
    for (uint32_t m = 1; m <= parallelism; m++) {
      std::list<drivacy::types::CipherText> &incoming =
          exchange[machine_id][m];
      for (drivacy::types::CipherText msg : incoming) {
        assert(!done);
        done = shuffler.ShuffleMessage(m, msg);
      }
    }
    assert(done);
  }

  // Shuffling is done, read the shuffled messages.
  std::unordered_set<uint32_t> output;
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    drivacy::protocol::offline::Shuffler &shuffler = shufflers[machine_id - 1];
    for (uint32_t i = 0; i < shuffler.batch_size(); i++) {
      drivacy::types::CipherText msg = shuffler.NextMessage();
      output.insert(TestMessage::Upcast(msg)->id);
    }
    shuffler.FreeMessages();
  }

  // Check all messages are preserved!
  for (TestMessage &msg : input) {
    assert(output.count(msg.id) == 1);
  }

  std::cout << "All tests passed!" << std::endl;
  return absl::OkStatus();
}

int main(int argc, char *argv[]) {
  // Command line usage message.
  if (argc < 2) {
    std::cout << "usage: " << argv[0] << " <size_1> <size_2> ..." << std::endl;
    std::cout << "\t<size_i>: the size of input for machine i." << std::endl;
    return 0;
  }

  // Number of parallel machines to simulate.
  uint32_t parallelism = argc - 1;
  // Input size for every machines.
  std::unordered_map<uint32_t, uint32_t> sizes;
  for (uint32_t id = 1; id <= parallelism; id++) {
    sizes[id] = std::atoi(argv[id]);
  }

  // Do the testing!
  absl::Status output = Test(parallelism, sizes);
  if (!output.ok()) {
    std::cout << output << std::endl;
    return 1;
  }

  return 0;
}
