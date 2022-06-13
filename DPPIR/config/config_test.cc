#include "DPPIR/config/config.h"

#include <cassert>
#include <iostream>
#include <string>

#include "DPPIR/onion/onion.h"

namespace DPPIR {
namespace config {

#define TEST_FILE "/tmp/dppir.config.test"

void EnsureEqual(const Config& c1, const Config& c2) {
  assert(c1.db_size == c2.db_size);
  assert(c1.epsilon == c2.epsilon);
  assert(c1.delta == c2.delta);
  assert(c1.party_count == c2.party_count);
  assert(c1.server_count == c2.server_count);
  assert(c1.parties.size() == c1.party_count);
  assert(c2.parties.size() == c2.party_count);
  for (size_t i = 0; i < c1.parties.size(); i++) {
    const PartyConfig& p1 = c1.parties.at(i);
    const PartyConfig& p2 = c2.parties.at(i);
    assert(p1.shared_seed == p2.shared_seed);
    assert(p1.servers.size() == c1.server_count);
    assert(p2.servers.size() == c2.server_count);
    for (size_t j = 0; j < p1.servers.size(); j++) {
      const ServerConfig& s1 = p1.servers.at(j);
      const ServerConfig& s2 = p2.servers.at(j);
      assert(s1.local_seed == s2.local_seed);
      assert(s1.port == s2.port);
      assert(s1.parallel_port == s2.parallel_port);
      assert(s1.ip == s2.ip);
    }
    for (size_t j = 0; j < p1.onion_pkey.size(); j++) {
      assert(p1.onion_pkey[j] == p2.onion_pkey[j]);
    }
    for (size_t j = 0; j < p1.onion_skey.size(); j++) {
      assert(p1.onion_skey[j] == p2.onion_skey[j]);
    }
  }
}

Config DummyConfig() {
  // Make a dummy config.
  Config config;
  // Database config.
  config.db_size = 1000000;
  // DP paramters.
  config.epsilon = 0.1;
  config.delta = 0.000001;
  // Parties config.
  config.party_count = 3;
  config.server_count = 2;
  config.parties = std::vector<PartyConfig>(config.party_count);
  for (size_t i = 0; i < config.party_count; i++) {
    // One party at a time.
    config.parties[i].shared_seed = i + 1000;
    config.parties[i].servers = std::vector<ServerConfig>(config.server_count);
    for (size_t j = 0; j < config.server_count; j++) {
      // One server at a time.
      config.parties[i].servers[j].local_seed = i * config.server_count + j;
      config.parties[i].servers[j].port = 3000 + i * config.server_count + j;
      config.parties[i].servers[j].parallel_port =
          4000 + i * config.server_count + j;
      config.parties[i].servers[j].ip =
          std::string("260.16.") + std::to_string(i) + "." + std::to_string(j);
    }
    // Generate onion encryption keys.
    onion::GenerateKeyPair(&config.parties[i].onion_pkey,
                           &config.parties[i].onion_skey);
  }
  return config;
}

// Test serializing/deserializing to a string in memory.
void TestInMemory() {
  Config config = DummyConfig();
  // Serialize then deserialize.
  std::string ser = Serialize(config);
  Config deserialized = Deserialize(ser.c_str(), ser.size());
  // Must be equals.
  EnsureEqual(config, deserialized);
}

// Test serializing/deserializing to a file.
void TestFile() {
  Config config = DummyConfig();
  // Serialize to the file then deserialize.
  WriteToFile(config, TEST_FILE);
  Config deserialized = ReadFile(TEST_FILE);
  // Must be equals.
  EnsureEqual(config, deserialized);
}

}  // namespace config
}  // namespace DPPIR

int main() {
  assert(sodium_init() >= 0);

  std::cout << "Testing in memory..." << std::endl;
  DPPIR::config::TestInMemory();
  std::cout << "Test pass!" << std::endl;

  std::cout << "Testing via file..." << std::endl;
  DPPIR::config::TestFile();
  std::cout << "Test pass!" << std::endl;

  std::cout << "All tests passed!" << std::endl;
  return 0;
}
