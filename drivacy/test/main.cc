// Copyright 2020 multiparty.org

// Main entry point to our protocol.
// This file must be run from the command line once per server:
// every time this file is run, it is logically equivalent to a new
// server.
// The server's configurations, including which party it belongs to, is
// set by the input configuration file passed to it via the command line.

#define BATCH_SIZE 3

#include <cstdint>
#include <iostream>
#include <iterator>
#include <list>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "drivacy/io/simulated_socket.h"
#include "drivacy/parties/backend_party.h"
#include "drivacy/parties/client.h"
#include "drivacy/parties/head_party.h"
#include "drivacy/parties/party.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"
#include "drivacy/util/file.h"
#include "drivacy/util/status.h"

ABSL_FLAG(std::string, table, "", "The path to table JSON file (required)");
ABSL_FLAG(std::string, config, "", "The path to configuration file (required)");

absl::Status Test(const drivacy::types::Configuration &config,
                  const drivacy::types::Table &table) {
  std::cout << "Testing..." << std::endl;
  uint32_t parallelism = config.parallelism();

  // Create all clients.
  std::list<drivacy::parties::Client> clients;
  std::list<std::list<uint64_t>> queries;
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    clients.emplace_back(machine_id, config,
                         drivacy::io::socket::SimulatedClientSocket::Factory);
    drivacy::parties::Client &client = clients.back();

    // Verify correctness of query / response.
    queries.emplace_back();
    std::list<uint64_t> &client_queries = queries.back();
    client.SetOnResponseHandler([&](uint64_t query, uint64_t response) {
      assert(query == client_queries.front());
      assert(response == table.at(query));
      client_queries.pop_front();
    });
  }

  // Query everything in the table.
  auto client_it = clients.begin();
  auto queries_it = queries.begin();
  for (const auto &[query, response] : table) {
    // Record and make query.
    queries_it->push_back(query);
    client_it->MakeQuery(query);
    // Move to the next client.
    client_it = std::next(client_it);
    queries_it = std::next(queries_it);
    if (client_it == clients.end()) {
      // Restart from the first client again.
      client_it = clients.begin();
      queries_it = queries.begin();
    }
  }

  // Make sure all queries were handled.
  for (const auto &client_queries : queries) {
    assert(client_queries.size() == 0);
  }
  std::cout << "All tests passed!" << std::endl;
  return absl::OkStatus();
}

absl::Status Setup(const std::string &table_path,
                   const std::string &config_path) {
  std::cout << "Setting up..." << std::endl;

  // Read configuration.
  drivacy::types::Configuration config;
  CHECK_STATUS(drivacy::util::file::ReadProtobufFromJson(config_path, &config));

  // Read input table.
  ASSIGN_OR_RETURN(std::string json,
                   drivacy::util::file::ReadFile(table_path.c_str()));
  ASSIGN_OR_RETURN(drivacy::types::Table table,
                   drivacy::util::file::ParseTable(json));

  // Setup parties.
  // Setup the head party's machines!
  uint32_t parallelism = config.parallelism();
  std::list<drivacy::parties::HeadParty> head_party;
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    head_party.emplace_back(
        1, machine_id, config, table,
        drivacy::io::socket::SimulatedSocket::Factory,
        drivacy::io::socket::SimulatedIntraPartySocket::Factory,
        drivacy::io::socket::SimulatedClientSocket::Factory, BATCH_SIZE);
  }

  // Setup middle-of-the-chain parties' machines.
  std::list<drivacy::parties::Party> parties;
  for (uint32_t party_id = 2; party_id < config.parties(); party_id++) {
    for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
      parties.emplace_back(
          party_id, machine_id, config, table,
          drivacy::io::socket::SimulatedSocket::Factory,
          drivacy::io::socket::SimulatedIntraPartySocket::Factory);
    }
  }

  // Setup the backend party's machines.
  std::list<drivacy::parties::BackendParty> backend_party;
  for (uint32_t machine_id = 1; machine_id <= parallelism; machine_id++) {
    backend_party.emplace_back(config.parties(), machine_id, config, table,
                               drivacy::io::socket::SimulatedSocket::Factory);
  }

  // Listen.
  for (auto &machine : head_party) {
    machine.Listen();
  }
  for (auto &machine : parties) {
    machine.Listen();
  }
  for (auto &machine : backend_party) {
    machine.Listen();
  }

  // Make a query.
  return Test(config, table);
}

int main(int argc, char *argv[]) {
  // Verify protobuf library was linked properly.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // Command line usage message.
  absl::SetProgramUsageMessage(absl::StrFormat("usage: %s %s", argv[0],
                                               "--table=path/to/table.json "
                                               "--config=path/to/config.json"));
  absl::ParseCommandLine(argc, argv);

  // Get command line flags.
  const std::string &table_path = absl::GetFlag(FLAGS_table);
  const std::string &config_path = absl::GetFlag(FLAGS_config);
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

  // Do the testing!
  absl::Status output = Setup(table_path, config_path);
  if (!output.ok()) {
    std::cout << output << std::endl;
    return 1;
  }

  // Clean up protobuf.
  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
