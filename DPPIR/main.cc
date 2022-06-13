#include <cassert>
#include <iostream>
#include <string>

#include "DPPIR/config/config.h"
#include "DPPIR/protocol/backend/backend.h"
#include "DPPIR/protocol/client/client.h"
#include "DPPIR/protocol/parallel_party/parallel_party.h"
#include "DPPIR/protocol/party/party.h"
#include "DPPIR/types/database.h"
#include "DPPIR/types/types.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"

#define USAGE_MESSAGE "DPPIR Protocol entry point for parties and clients"

// Command line flags.
ABSL_FLAG(std::string, role, "", "The role: client or party");
ABSL_FLAG(std::string, stage, "", "One of: offline, online, or all (required)");
ABSL_FLAG(std::string, config, "",
          "The config file name (must be under <DPPIR DIR>/config) (required)");
ABSL_FLAG(int, server_id, -1, "The server id for parallelism (required)");
ABSL_FLAG(int, party_id, -1, "The party id (required if role is party)");
ABSL_FLAG(int64_t, queries, -1, "# of queries (required if role is client)");

int main(int argc, char** argv) {
  assert(sodium_init() >= 0);

  // Command line flags.
  absl::SetProgramUsageMessage(USAGE_MESSAGE);
  absl::ParseCommandLine(argc, argv);
  std::string configfile = absl::GetFlag(FLAGS_config);
  std::string stage = absl::GetFlag(FLAGS_stage);
  std::string role = absl::GetFlag(FLAGS_role);
  int server_id = absl::GetFlag(FLAGS_server_id);
  int party_id = absl::GetFlag(FLAGS_party_id);
  int64_t queries = absl::GetFlag(FLAGS_queries);

  // Validate flags.
  if (configfile == "") {
    std::cout << "--config is required" << std::endl;
    return 1;
  }
  if (stage == "") {
    std::cout << "--stage=[online|offline|all] is required" << std::endl;
    return 1;
  }
  if (stage != "online" && stage != "offline" && stage != "all") {
    std::cout << "Unrecognizable stage" << std::endl;
    return 1;
  }
  if (role == "") {
    std::cout << "--role=[party|client] is required" << std::endl;
    return 1;
  }
  if (role != "client" && role != "party") {
    std::cout << "Unrecognizable role" << std::endl;
    return 1;
  }
  if (role == "party" && party_id < 0) {
    std::cout << "--party_id is required for parties" << std::endl;
    return 1;
  }
  if (role == "client" && queries < 0) {
    std::cout << "--queries is required for clients" << std::endl;
    return 1;
  }
  if (server_id < 0) {
    std::cout << "--server_id is required" << std::endl;
    return 1;
  }

  // Read config.
  DPPIR::config::Config config = DPPIR::config::ReadFile(configfile);

  // Initialize database.
  DPPIR::Database db(config.db_size);

  // More validation.
  if (server_id >= config.server_count) {
    std::cout << "server_id out of range!" << std::endl;
    return 1;
  }
  if (party_id >= config.party_count) {
    std::cout << "party_id out of range!" << std::endl;
    return 1;
  }

  // Stages to run.
  bool offline = stage == "offline" || stage == "all";
  bool online = stage == "online" || stage == "all";

  // Invoke the correct protocol.
  if (role == "client") {
    DPPIR::protocol::Client client(server_id, std::move(config), std::move(db));
    client.Start(queries, offline, online);
  } else {
    if (party_id < config.party_count - 1) {
      if (config.server_count == 1) {
        DPPIR::protocol::Party party(party_id, server_id, std::move(config),
                                     std::move(db));
        party.Start(offline, online);
      } else {
        DPPIR::protocol::ParallelParty party(party_id, server_id,
                                             std::move(config), std::move(db));
        party.Start(offline, online);
      }
    } else {
      DPPIR::protocol::BackendParty backend(server_id, std::move(config),
                                            std::move(db));
      backend.Start(offline, online);
    }
  }

  std::cout << "Done!" << std::endl;
  return 0;
}
