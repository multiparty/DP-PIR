// Copyright 2020 multiparty.org

// Main entry point to our protocol.
// This file must be run from the command line once per server:
// every time this file is run, it is logically equivalent to a new
// server.
// The server's configurations, including which party it belongs to, is
// set by the input configuration file passed to it via the command line.

#include <stdlib.h>

#include <cstdint>
#include <iostream>
#include <list>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
// NOLINTNEXTLINE
#include "drivacy/parties/client.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"
#include "drivacy/util/file.h"
#include "drivacy/util/status.h"

ABSL_FLAG(std::string, table, "", "The path to table JSON file (required)");
ABSL_FLAG(std::string, config, "", "The path to configuration file (required)");
ABSL_FLAG(uint32_t, machine, 0, "The head machine id to query (required)");
ABSL_FLAG(uint32_t, queries, 0, "The number of queries to make (required)");

absl::Status Setup(uint32_t machine_id, uint32_t query_count,
                   const std::string &table_path,
                   const std::string &config_path) {
  // Read configuration.
  drivacy::types::Configuration config;
  CHECK_STATUS(drivacy::util::file::ReadProtobufFromJson(config_path, &config));

  // Read input table.
  ASSIGN_OR_RETURN(std::string json,
                   drivacy::util::file::ReadFile(table_path.c_str()));
  ASSIGN_OR_RETURN(drivacy::types::Table table,
                   drivacy::util::file::ParseTable(json));

  // Setup party and listen to incoming queries and responses.
  drivacy::parties::Client client(machine_id, config);

  // Verify correctness of query / response.
  std::list<uint64_t> queries;
  uint32_t response_count = 0;
  client.SetOnResponseHandler([&](uint64_t query, uint64_t response) {
    assert(query == queries.front());
    assert(response == table.at(query));
    queries.pop_front();
    if (response_count++ % 10000 == 0) {
      std::cout << "Received " << (response_count - 1) << std::endl;
    }
  });

  // Query from table.
  for (uint32_t i = 0; i < query_count;) {
    for (const auto &[query, response] : table) {
      queries.push_back(query);
      client.MakeQuery(query);
      if (i % 10000 == 0) {
        std::cout << "Sent " << i << std::endl;
      }
      if (++i == query_count) {
        break;
      }
    }
  }

  // Block listening for responses.
  client.Listen();

  // Done...
  assert(queries.size() == 0);

  // Will never really get here...
  return absl::OkStatus();
}

int main(int argc, char *argv[]) {
  // Verify protobuf library was linked properly.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // Command line usage message.
  absl::SetProgramUsageMessage(absl::StrFormat("usage: %s %s", argv[0],
                                               "--table=path/to/table.json "
                                               "--config=path/to/config.json "
                                               "--queries=#queries"));
  absl::ParseCommandLine(argc, argv);

  // Get command line flags.
  const std::string &table_path = absl::GetFlag(FLAGS_table);
  const std::string &config_path = absl::GetFlag(FLAGS_config);
  uint32_t machine_id = absl::GetFlag(FLAGS_machine);
  uint32_t query_count = absl::GetFlag(FLAGS_queries);
  if (table_path.empty()) {
    std::cout << "Please provide a valid table JSON file using --table"
              << std::endl;
    return 1;
  }
  if (config_path.empty()) {
    std::cout << "Please provide a valid config file using --config"
              << std::endl;
    return 1;
  }
  if (machine_id == 0) {
    std::cout << "Please provide a valid machine id to query using --machine"
              << std::endl;
    return 1;
  }
  if (query_count == 0) {
    std::cout << "Please provide a valid query count using --queries"
              << std::endl;
    return 1;
  }

  // Execute mock protocol.
  absl::Status output = Setup(machine_id, query_count, table_path, config_path);
  if (!output.ok()) {
    std::cout << output << std::endl;
    return 1;
  }

  std::cout << "Success!" << std::endl;

  // Clean up protobuf.
  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
