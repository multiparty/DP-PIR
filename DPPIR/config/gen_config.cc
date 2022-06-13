#include <cassert>
#include <iostream>
#include <string>

#include "DPPIR/config/config.h"
#include "DPPIR/onion/onion.h"

namespace DPPIR {
namespace config {

Config Interactive() {
  Config config;

  // Database config.
  std::cout << "Enter database size: ";
  std::cin >> config.db_size;

  // DP parameters.
  std::cout << "Enter epsilon and delta: ";
  std::cin >> config.epsilon;
  std::cin >> config.delta;

  // Parties and servers.
  std::cout << "Enter number of parties and servers per party: ";
  int party_count, server_count;
  std::cin >> party_count;
  std::cin >> server_count;

  std::string dummy;
  std::getline(std::cin, dummy);

  config.party_count = party_count;
  config.server_count = server_count;
  config.parties = std::vector<PartyConfig>(party_count);
  for (party_id_t i = 0; i < config.party_count; i++) {
    // One party at a time.
    PartyConfig& party = config.parties.at(i);
    party.shared_seed = i + 1000;
    party.servers = std::vector<ServerConfig>(server_count);
    for (server_id_t j = 0; j < config.server_count; j++) {
      std::cout << "Party " << int(i) << " - Server " << int(j) << std::endl;
      // One server at a time.
      ServerConfig& server = party.servers.at(j);
      server.local_seed = i * config.server_count + j;
      server.port = 3000 + (i * config.server_count + j);
      server.parallel_port = 4000 + (i * config.server_count + j);
      // IP.
      std::cout << "Enter ip (empty for 127.0.0.1): ";
      std::getline(std::cin, server.ip);
      if (server.ip == "") {
        server.ip = "127.0.0.1";
      }
    }
    // Generate onion encryption keys.
    onion::GenerateKeyPair(&party.onion_pkey, &party.onion_skey);
  }

  return config;
}

Config FromArgs(int argc, char** argv) {
  assert(argc >= 6);
  Config config;
  // Database config.
  config.db_size = std::stoi(argv[1]);
  // DP parameters.
  config.epsilon = std::stod(argv[2]);
  config.delta = std::stod(argv[3]);
  // Parties and servers.
  config.party_count = std::stoi(argv[4]);
  config.server_count = std::stoi(argv[5]);
  config.parties = std::vector<PartyConfig>(config.party_count);
  // Party/server details.
  int args = 6 + (config.party_count * config.server_count);
  assert(argc == args || argc == 6);
  for (party_id_t i = 0; i < config.party_count; i++) {
    // One party at a time.
    PartyConfig& party = config.parties.at(i);
    party.shared_seed = i + 1000;
    party.servers = std::vector<ServerConfig>(config.server_count);
    for (server_id_t j = 0; j < config.server_count; j++) {
      // One server at a time.
      ServerConfig& server = party.servers.at(j);
      server.local_seed = i * config.server_count + j;
      // Port.
      server.port = 3000 + (i * config.server_count + j);
      server.parallel_port = 4000 + (i * config.server_count + j);
      // IP.
      if (argc == 6) {
        server.ip = "127.0.0.1";
      } else {
        server.ip = std::string(argv[6 + (i * config.server_count) + j]);
      }
    }
    // Generate onion encryption keys.
    onion::GenerateKeyPair(&party.onion_pkey, &party.onion_skey);
  }

  return config;
}

}  // namespace config
}  // namespace DPPIR

int main(int argc, char** argv) {
  assert(sodium_init() >= 0);

  // Build config struct.
  DPPIR::config::Config config;
  if (argc < 2) {
    std::cout << "Please provide output file as the first command line argument"
              << std::endl;
    return 0;
  } else if (argc == 2) {
    config = DPPIR::config::Interactive();
  } else {
    config = DPPIR::config::FromArgs(argc - 1, argv + 1);
  }

  // Write to file.
  std::string file = argv[1];
  DPPIR::config::WriteToFile(config, file);
  std::cout << "Written to " << file << std::endl;
  return 0;
}
