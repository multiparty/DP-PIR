// Copyright 2020 multiparty.org

// Testing the shuffling implementation!

#include "drivacy/protocol/shuffle.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <list>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "drivacy/types/types.h"

#define PARTY_COUNT 3

ABSL_FLAG(uint32_t, parallelism, 0, "Number of machines (required)");
ABSL_FLAG(uint32_t, size, 0, "Number of queries per machine (required)");

struct TestQuery {
  uint32_t query;
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

absl::Status Test(uint32_t parallelism, uint32_t size) {
  std::cout << "Testing..." << std::endl;

  // input
  std::vector<TestQuery> input;
  for (uint32_t i = 0; i < size * parallelism; i++) {
    input.emplace_back(i);
  }

  // Setup shufflers.
  std::cout << "Initializing..." << std::endl;
  std::vector<drivacy::protocol::Shuffler> shufflers;
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    shufflers.emplace_back(1, machine_id, PARTY_COUNT, parallelism);
    shufflers[machine_id - 1].Initialize(size);
  }

  // Begin shuffling.
  std::unordered_map<
      uint32_t,
      std::unordered_map<uint32_t, std::list<drivacy::types::ForwardQuery>>>
      exchange;
  // Phase 1.
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    drivacy::protocol::Shuffler &shuffler = shufflers[machine_id - 1];
    for (uint32_t i = 0; i < size; i++) {
      TestQuery &query = input.at((machine_id - 1) * size + i);
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
    for (uint32_t i = 0; i < size; i++) {
      drivacy::types::ForwardQuery &q = shuffler.NextQuery();
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
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    drivacy::protocol::Shuffler &shuffler = shufflers[machine_id - 1];
    for (uint32_t i = 0; i < size; i++) {
      drivacy::types::Response &r = shuffler.NextResponse();
      TestQuery q = input[(machine_id - 1) * size + i];
      assert(Compare(q, r));
    }
  }

  std::cout << "All tests passed!" << std::endl;
  return absl::OkStatus();
}

int main(int argc, char *argv[]) {
  // Command line usage message.
  absl::SetProgramUsageMessage(absl::StrFormat(
      "usage: %s %s", argv[0],
      "--paralllelism=number of machines --size=queries per machine"));
  absl::ParseCommandLine(argc, argv);

  // Get command line flags.
  uint32_t parallelism = absl::GetFlag(FLAGS_parallelism);
  uint32_t size = absl::GetFlag(FLAGS_size);
  if (parallelism == 0) {
    std::cout << "Please provide a valid number of machines using --parallelism"
              << std::endl;
    return 1;
  }
  if (size == 0) {
    std::cout << "Please provide a valid batch size per machine using --size"
              << std::endl;
    return 1;
  }

  // Do the testing!
  absl::Status output = Test(parallelism, size);
  if (!output.ok()) {
    std::cout << output << std::endl;
    return 1;
  }

  return 0;
}
