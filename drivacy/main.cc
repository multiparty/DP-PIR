// Copyright 2020 multiparty.org

// Main entry point to our protocol.
// This file must be run from the command line once per server:
// every time this file is run, it is logically equivalent to a new
// server.
// The server's configurations, including which party it belongs to, is
// set by the input configuration file passed to it via the command line.

#include <iostream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "drivacy/io/file.h"
#include "drivacy/proto/config.pb.h"
#include "drivacy/proto/table.pb.h"
#include "drivacy/util/status.h"

ABSL_FLAG(std::string, json, "", "The path to input JSON file (required)");
ABSL_FLAG(std::string, config, "", "The path to configuration file (required)");

absl::Status Protocol(const std::string &json_path,
                      const std::string &config_path) {
  // Read configuration.
  drivacy::proto::Configuration config;
  CHECK_STATUS(drivacy::io::file::ReadProtobufFromJson(config_path, &config));
  std::cout << config.DebugString() << std::endl;

  // Read input table.
  ASSIGN_OR_RETURN(std::string json,
                   drivacy::io::file::ReadFile(json_path.c_str()));
  ASSIGN_OR_RETURN(drivacy::proto::Table table,
                   drivacy::io::file::ParseTable(json));

  std::cout << table.rows_size() << std::endl;
  std::cout << table.rows(0).DebugString() << std::endl;
  return absl::OkStatus();
}

int main(int argc, char *argv[]) {
  // Verify protobuf library was linked properly.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // Command line usage message.
  absl::SetProgramUsageMessage(
      absl::StrFormat("usage: %s %s", argv[0],
                      "--json=path/to/data.json --config=path/to/config.json"));
  absl::ParseCommandLine(argc, argv);

  // Get command line flags.
  const std::string &json_path = absl::GetFlag(FLAGS_json);
  const std::string &config_path = absl::GetFlag(FLAGS_config);
  if (json_path.empty()) {
    std::cout << "Please provide a valid input json file using --json"
              << std::endl;
    return 1;
  }
  if (config_path.empty()) {
    std::cout << "Please provide a valid config file using --config"
              << std::endl;
    return 1;
  }

  // Read input JSON
  absl::Status output = Protocol(json_path, config_path);
  if (!output.ok()) {
    std::cout << output << std::endl;
    return 1;
  }

  // Clean up protobuf.
  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
