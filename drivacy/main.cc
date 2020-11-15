// Copyright 2020 multiparty.org

// Main entry point to our protocol.
// This file must be run from the command line once per server:
// every time this file is run, it is logically equivalent to a new
// server.
// The server's configurations, including which party it belongs to, is
// set by the input configuration file passed to it via the command line.

#include <cstdint>
#include <iostream>
#include <list>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "drivacy/io/socket.h"
#include "drivacy/io/websocket_server.h"
#include "drivacy/parties/backend_party.h"
#include "drivacy/parties/head_party.h"
#include "drivacy/parties/party.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"
#include "drivacy/util/file.h"
#include "drivacy/util/status.h"

ABSL_FLAG(std::string, table, "", "The path to table JSON file (required)");
ABSL_FLAG(std::string, config, "", "The path to configuration file (required)");
ABSL_FLAG(uint32_t, party, 0, "The id of the party [1-n] (required)");
ABSL_FLAG(uint32_t, machine, 0, "The id of the machine [1-p] (required)");
ABSL_FLAG(uint32_t, batch, 1,
          "The size of a query batch (used only for party=1)");
absl::Status Setup(uint32_t party_id, uint32_t machine_id,
                   const std::string &table_path,
                   const std::string &config_path, uint32_t batch_size) {
  // Read configuration.
  drivacy::types::Configuration config;
  CHECK_STATUS(drivacy::util::file::ReadProtobufFromJson(config_path, &config));

  // Read input table.
  ASSIGN_OR_RETURN(std::string json,
                   drivacy::util::file::ReadFile(table_path.c_str()));
  ASSIGN_OR_RETURN(drivacy::types::Table table,
                   drivacy::util::file::ParseTable(json));

  // Setup party and listen to incoming queries and responses.
  if (party_id == 1) {
    drivacy::parties::HeadParty party(
        party_id, machine_id, config, table,
        drivacy::io::socket::TCPSocket::Factory,
        drivacy::io::socket::WebSocketServer::Factory, batch_size);
    party.Listen();
  } else if (party_id == config.parties()) {
    drivacy::parties::BackendParty party(
        party_id, machine_id, config, table,
        drivacy::io::socket::TCPSocket::Factory);
    party.Listen();
  } else {
    drivacy::parties::Party party(party_id, machine_id, config, table,
                                  drivacy::io::socket::TCPSocket::Factory);
    party.Listen();
  }

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
                                               "--party=<id>"));
  absl::ParseCommandLine(argc, argv);

  // Get command line flags.
  const std::string &table_path = absl::GetFlag(FLAGS_table);
  const std::string &config_path = absl::GetFlag(FLAGS_config);
  uint32_t party_id = absl::GetFlag(FLAGS_party);
  uint32_t machine_id = absl::GetFlag(FLAGS_machine);
  uint32_t batch_size = absl::GetFlag(FLAGS_batch);
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
  if (party_id == 0) {
    std::cout << "Please provide a valid party id using --party" << std::endl;
    return 1;
  }
  if (machine_id == 0) {
    std::cout << "Please provide a valid machine id using --machine"
              << std::endl;
    return 1;
  }

  // Execute mock protocol.
  absl::Status output =
      Setup(party_id, machine_id, table_path, config_path, batch_size);
  if (!output.ok()) {
    std::cout << output << std::endl;
    return 1;
  }

  // Clean up protobuf.
  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
