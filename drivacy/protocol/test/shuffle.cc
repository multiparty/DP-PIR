// Copyright 2020 multiparty.org

// Testing the shuffling implementation!

#include "drivacy/protocol/shuffle.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <list>
#include <vector>

#include "absl/status/status.h"
#include "drivacy/types/types.h"

#define PARTY_COUNT 3
#define PAD_SIZE 152 - 24

struct TestQuery {
  uint32_t query;
  char pad[PAD_SIZE];
  drivacy::types::QueryState state;
  drivacy::types::Response response;

  explicit TestQuery(uint32_t q) : query(q), state(q * 10), response(q * 20) {}
  drivacy::types::ForwardQuery Cast() {
    return reinterpret_cast<drivacy::types::ForwardQuery>(this);
  }
  static const TestQuery *Upcast(drivacy::types::ForwardQuery q) {
    return reinterpret_cast<const TestQuery *>(q);
  }
};

bool Compare(const TestQuery &query, const drivacy::types::Response &response) {
  return query.response.tally() + query.state == response.tally();
}

absl::Status Test(uint32_t parallelism,
                  const std::unordered_map<uint32_t, uint32_t> &sizes) {
  std::cout << "Testing..." << std::endl;

  // input
  std::vector<TestQuery> input;
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    for (uint32_t i = 0; i < sizes.at(machine_id); i++) {
      input.emplace_back(input.size());
    }
  }

  // Setup shufflers.
  std::cout << "Initializing..." << std::endl;
  std::vector<drivacy::protocol::Shuffler> shufflers;
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
      std::unordered_map<uint32_t, std::list<drivacy::types::ForwardQuery>>>
      exchange;
  // Phase 1.
  uint32_t index = 0;
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    drivacy::protocol::Shuffler &shuffler = shufflers[machine_id - 1];
    for (uint32_t i = 0; i < sizes.at(machine_id); i++) {
      TestQuery &query = input.at(index++);
      uint32_t m = shuffler.MachineOfNextQuery(query.state);
      // Exchange...
      exchange[m][machine_id].push_back(query.Cast());
    }
  }

  // Phase 2.
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    bool done = false;
    drivacy::protocol::Shuffler &shuffler = shufflers[machine_id - 1];
    for (uint32_t m = 1; m <= parallelism; m++) {
      std::list<drivacy::types::ForwardQuery> &incoming =
          exchange[machine_id][m];
      for (drivacy::types::ForwardQuery q : incoming) {
        assert(!done);
        done = shuffler.ShuffleQuery(m, q);
      }
    }
    assert(done);
  }

  // Shuffling is done, some stuff happen and we end up with responses.
  std::unordered_map<uint32_t, std::list<drivacy::types::Response>> responses;
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    std::list<drivacy::types::Response> &list = responses[machine_id];
    drivacy::protocol::Shuffler &shuffler = shufflers[machine_id - 1];
    for (uint32_t i = 0; i < shuffler.batch_size(); i++) {
      drivacy::types::ForwardQuery q = shuffler.NextQuery();
      list.push_back(TestQuery::Upcast(q)->response);
      delete[] q;
    }
  }

  // Now, we do de-shuffling.
  std::unordered_map<
      uint32_t,
      std::unordered_map<uint32_t, std::list<drivacy::types::Response>>>
      exchange2;
  // Phase 1.
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    std::list<drivacy::types::Response> list = responses.at(machine_id);
    drivacy::protocol::Shuffler &shuffler = shufflers[machine_id - 1];
    for (drivacy::types::Response response : list) {
      uint32_t m = shuffler.MachineOfNextResponse();
      // Exchange...
      exchange2[m][machine_id].push_back(response);
    }
  }

  // Phase 2.
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    bool done = false;
    drivacy::protocol::Shuffler &shuffler = shufflers[machine_id - 1];
    for (uint32_t m = 1; m <= parallelism; m++) {
      std::list<drivacy::types::Response> &incoming = exchange2[machine_id][m];
      for (drivacy::types::Response r : incoming) {
        assert(!done);
        drivacy::types::QueryState &state = shuffler.NextQueryState(m);
        drivacy::types::Response processed(r.tally() + state);
        done = shuffler.DeshuffleResponse(m, processed);
      }
    }
    assert(done);
  }

  // De-Shuffling is done, validate all is the way it should be.
  index = 0;
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    drivacy::protocol::Shuffler &shuffler = shufflers[machine_id - 1];
    for (uint32_t i = 0; i < sizes.at(machine_id); i++) {
      drivacy::types::Response &r = shuffler.NextResponse();
      TestQuery q = input[index++];
      assert(Compare(q, r));
    }
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
