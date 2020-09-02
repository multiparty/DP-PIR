// Copyright 2020 multiparty.org

// A script for generating a config file in the appropriate format
// given configuration parameters.

#include <cstdint>
#include <iostream>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_format.h"
#include "drivacy/primitives/crypto.h"
#include "drivacy/types/config.pb.h"
#include "google/protobuf/util/json_util.h"

ABSL_FLAG(uint32_t, parties, 0, "The number of parties (required)");

int main(int argc, char *argv[]) {
  // Verify protobuf library was linked properly.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // Command line usage message.
  absl::SetProgramUsageMessage(absl::StrFormat(
      "usage: %s %s", argv[0], "--parties=<number of parties>"));
  absl::ParseCommandLine(argc, argv);

  // Get command line flags.
  uint32_t parties = absl::GetFlag(FLAGS_parties);
  if (parties < 2) {
    std::cout << "You need at least 2 parties!" << std::endl;
    return 1;
  }

  // Build configuration prototype.
  drivacy::types::Configuration config;
  config.set_parties(parties);
  for (uint32_t party_id = 1; party_id <= parties; party_id++) {
    config.mutable_keys()->insert(
        {party_id, drivacy::primitives::crypto::GenerateEncryptionKeyPair()});
  }

  // Handle the networking configuration.
  for (uint32_t party_id = 1; party_id <= parties; party_id++) {
    drivacy::types::PartyNetworkConfig party_config;
    party_config.set_ip("127.0.0.1");
    party_config.set_socket_port(3000 + party_id);
    party_config.set_webserver_port(party_id == 1 ? 3000 : -1);
    config.mutable_network()->insert({party_id, party_config});
  }

  // Make sure JSON is nice to read.
  google::protobuf::util::JsonPrintOptions options;
  options.add_whitespace = true;
  options.always_print_primitive_fields = true;
  options.preserve_proto_field_names = true;

  // Dump configuration JSON.
  std::string json;
  google::protobuf::util::Status status =
      google::protobuf::util::MessageToJsonString(config, &json, options);
  if (!status.ok()) {
    std::cout << "Cannot serialize config to JSON! Error = " << status
              << std::endl;
    return 1;
  }
  std::cout << json << std::endl;

  // Clean up protobuf.
  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
