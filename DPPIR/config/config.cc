#include "DPPIR/config/config.h"

#include <cassert>
#include <fstream>

namespace DPPIR {
namespace config {

namespace {

// Write.
inline std::string Int2Bin(int v) {
  return std::string(reinterpret_cast<const char*>(&v), sizeof(int));
}
inline std::string Double2Bin(double v) {
  return std::string(reinterpret_cast<const char*>(&v), sizeof(double));
}
// Read.
inline int Bin2Int(const char** data, size_t* n) {
  int v = *reinterpret_cast<const int*>(*data);
  *data += sizeof(int);
  *n -= sizeof(int);
  return v;
}
inline double Bin2Double(const char** data, size_t* n) {
  double v = *reinterpret_cast<const double*>(*data);
  *data += sizeof(double);
  *n -= sizeof(double);
  return v;
}
std::string Bin2Str(const char** data, size_t* n) {
  for (size_t i = 0; i < *n; i++) {
    if ((*data)[i] == '\0') {
      std::string v = std::string(*data, i);
      *data += i + 1;
      *n -= i + 1;
      return v;
    }
  }
  assert(false);
}

template <std::size_t SZ>
void ReadIntoBuffer(std::array<unsigned char, SZ>* buff, const char** data,
                    size_t* n) {
  assert(*n >= SZ);
  for (size_t i = 0; i < SZ; i++) {
    (*buff)[i] = (*data)[i];
  }
  *data += SZ;
  *n -= SZ;
}

}  // namespace

// Serialize to a string.
std::string Serialize(const Config& config) {
  std::string data = "";
  // Database config.
  data += Int2Bin(config.db_size);
  // DP parameters.
  data += Double2Bin(config.epsilon);
  data += Double2Bin(config.delta);
  // Parties config.
  data += Int2Bin(config.party_count);
  data += Int2Bin(config.server_count);
  assert(config.parties.size() == config.party_count);
  for (const PartyConfig& party : config.parties) {
    data += Int2Bin(party.shared_seed);
    data += std::string(reinterpret_cast<const char*>(party.onion_pkey.data()),
                        party.onion_pkey.size());
    data += std::string(reinterpret_cast<const char*>(party.onion_skey.data()),
                        party.onion_skey.size());
    assert(party.servers.size() == config.server_count);
    for (const ServerConfig& server : party.servers) {
      data += Int2Bin(server.local_seed);
      data += Int2Bin(server.port);
      data += Int2Bin(server.parallel_port);
      data += server.ip;
      data.push_back('\0');
    }
  }
  return data;
}

// Deserialize from string.
Config Deserialize(const char* str, size_t n) {
  Config config;
  // Database config.
  config.db_size = Bin2Int(&str, &n);
  // DP parameters.
  config.epsilon = Bin2Double(&str, &n);
  config.delta = Bin2Double(&str, &n);
  // Parties config.
  config.party_count = Bin2Int(&str, &n);
  config.server_count = Bin2Int(&str, &n);
  for (party_id_t i = 0; i < config.party_count; i++) {
    PartyConfig party;
    party.shared_seed = Bin2Int(&str, &n);
    ReadIntoBuffer(&party.onion_pkey, &str, &n);
    ReadIntoBuffer(&party.onion_skey, &str, &n);
    for (server_id_t j = 0; j < config.server_count; j++) {
      ServerConfig server;
      server.local_seed = Bin2Int(&str, &n);
      server.port = Bin2Int(&str, &n);
      server.parallel_port = Bin2Int(&str, &n);
      server.ip = Bin2Str(&str, &n);
      party.servers.push_back(server);
    }
    config.parties.push_back(party);
  }
  // Should have consumed all buffer.
  assert(n == 0);
  return config;
}

// Reading/Writing to file.
Config ReadFile(const std::string& file) {
  std::ifstream f(file.c_str());
  std::string data = "";
  char c;
  while (f.get(c)) {
    data.push_back(c);
  }
  return Deserialize(data.data(), data.size());
}
void WriteToFile(const Config& config, const std::string& file) {
  std::string data = Serialize(config);
  std::ofstream f(file.c_str());
  f.write(data.data(), data.size());
}

}  // namespace config
}  // namespace DPPIR
