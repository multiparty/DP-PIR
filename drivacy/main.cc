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
#include "drivacy/client.h"
#include "drivacy/io/client_socket.h"
#include "drivacy/io/socket.h"
#include "drivacy/party.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"
#include "drivacy/util/file.h"
#include "drivacy/util/status.h"

ABSL_FLAG(std::string, table, "", "The path to table JSON file (required)");
ABSL_FLAG(std::string, config, "", "The path to configuration file (required)");
ABSL_FLAG(uint32_t, party, 0, "The id of the party [1-n] (required)");

absl::Status Setup(uint32_t party_id, const std::string &table_path,
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
  if (party_id == 1) {
    drivacy::PartyHead<drivacy::io::socket::UDPSocket,
                       drivacy::io::socket::ClientSocket>
        party(party_id, config, table);
    party.Listen();
  } else {
    drivacy::Party<drivacy::io::socket::UDPSocket> party(party_id, config,
                                                         table);
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

  // Execute mock protocol.
  absl::Status output = Setup(party_id, table_path, config_path);
  if (!output.ok()) {
    std::cout << output << std::endl;
    return 1;
  }

  // Clean up protobuf.
  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
