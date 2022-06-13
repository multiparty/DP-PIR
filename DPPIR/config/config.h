#ifndef DPPIR_CONFIG_CONFIG_H_
#define DPPIR_CONFIG_CONFIG_H_

#include <string>
#include <vector>

#include "DPPIR/types/types.h"

namespace DPPIR {
namespace config {

struct ServerConfig {
  int local_seed;
  int port;
  int parallel_port;
  std::string ip;
};

struct PartyConfig {
  // Seed for global shuffle.
  int shared_seed;
  std::vector<ServerConfig> servers;
  // Keys for onion encryption.
  pkey_t onion_pkey;
  skey_t onion_skey;
};

struct Config {
  // Database config.
  index_t db_size;
  // DP parameters.
  double epsilon;
  double delta;
  // Parties config.
  party_id_t party_count;
  server_id_t server_count;  // # server per party (parallelism).
  std::vector<PartyConfig> parties;
};

// Serialize/Deserialize.
std::string Serialize(const Config& config);
Config Deserialize(const char* str, size_t n);

// Reading/Writing to file.
Config ReadFile(const std::string& file);
void WriteToFile(const Config& config, const std::string& file);

}  // namespace config
}  // namespace DPPIR

#endif  // DPPIR_CONFIG_CONFIG_H_
