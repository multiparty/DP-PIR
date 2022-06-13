#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_format.h"
// SealPIR
#include "pir.hpp"
#include "pir_client.hpp"
#include "pir_server.hpp"
// Seal
#include "seal/seal.h"

// Default SealPIR paramters.
#define ITEM_SIZE 8  // in bytes.
#define N 4096       // poly degree.
#define LOGT 20      // bits of plaintext coefficient.
#define D 2          // dimension of database (2 columns).

// Benchmarking macors
#define TS_VARNAME_NESTED(suffix) __TIMESTAMP_##suffix
#define TS_VARNAME(suffix) TS_VARNAME_NESTED(suffix)
#define BENCHMARK(cmd, duration)                                            \
  auto TS_VARNAME(__LINE__) = std::chrono::high_resolution_clock::now();    \
  cmd;                                                                      \
  duration +=                                                               \
      chrono::duration_cast<chrono::microseconds>(                          \
          std::chrono::high_resolution_clock::now() - TS_VARNAME(__LINE__)) \
          .count()

ABSL_FLAG(uint64_t, table, 0, "The table size (required)");
ABSL_FLAG(uint64_t, queries, 0, "The number of queries (required)");

namespace {

// Creating the table.
std::unique_ptr<uint8_t[]> CreateTable(uint64_t table_size) {
  std::unique_ptr<uint8_t[]> db =
      std::make_unique<uint8_t[]>(table_size * ITEM_SIZE);

  std::random_device rd;
  for (uint64_t i = 0; i < table_size; i++) {
    for (uint64_t j = 0; j < ITEM_SIZE; j++) {
      auto val = rd() % 256;  // random byte.
      db.get()[(i * ITEM_SIZE) + j] = val;
    }
  }

  return db;
}
std::unique_ptr<uint8_t[]> Copy(uint8_t *table, uint64_t table_size) {
  std::unique_ptr<uint8_t[]> copy =
      std::make_unique<uint8_t[]>(table_size * ITEM_SIZE);

  for (uint64_t i = 0; i < table_size; i++) {
    for (uint64_t j = 0; j < ITEM_SIZE; j++) {
      copy.get()[(i * ITEM_SIZE) + j] = table[(i * ITEM_SIZE) + j];
    }
  }

  return copy;
}

// Server side preprocessing.
void PreprocessTable(PIRServer *server, uint64_t table_size,
                     std::unique_ptr<uint8_t[]> &&db) {
  server->set_database(std::move(db), table_size, ITEM_SIZE);
  server->preprocess_database();
}

// PIR Protocol stages.
PirQuery MakeQuery(PIRClient *client, uint64_t row) {
  return client->generate_query(client->get_fv_index(row));
}
PirReply HandleQuery(PIRServer *server, PirQuery *query) {
  return server->generate_reply(*query, 0);
}
std::vector<uint8_t> MakeOutput(PIRClient *client, PirReply *reply,
                                      uint64_t row) {
  uint64_t offset = client->get_fv_offset(row);
  std::vector<uint8_t> result = client->decode_reply(*reply, offset);
  assert(result.size() == ITEM_SIZE);
  return result;
}

// Validate that the output is correct.
bool ValidateCorrectness(PIRClient *client, uint8_t *table,
                         const std::vector<uint8_t> &output, uint64_t row) {
  uint8_t *expected = table + (row * ITEM_SIZE);
  for (uint32_t i = 0; i < ITEM_SIZE; i++) {
    if (output.at(i) != expected[i]) {
      std::cout << "Main: PIR output is wrong!" << std::endl;
      return false;
    }
  }

  return true;
}

}  // namespace

int main(int argc, char *argv[]) {
  // Command line usage message.
  absl::SetProgramUsageMessage(absl::StrFormat("usage: %s %s", argv[0],
                                               "--table=number_of_rows "
                                               "--queries=number_of_queries"));
  absl::ParseCommandLine(argc, argv);

  // Get command line flags.
  uint64_t table_size = absl::GetFlag(FLAGS_table);
  uint64_t query_count = absl::GetFlag(FLAGS_queries);
  if (table_size == 0) {
    std::cout << "Please provide the table size using --table" << std::endl;
    return 1;
  }
  if (query_count == 0) {
    std::cout << "Please provide a valid query count using --queries"
              << std::endl;
    return 1;
  }

  // Benchmarking variables.
  uint64_t preprocessing_time = 0;
  uint64_t query_time = 0;
  uint64_t server_time = 0;
  uint64_t response_time = 0;

  // Configure parameters.
  cout << "Main: Generating all parameters" << endl;
  seal::EncryptionParameters enc_params{seal::scheme_type::bfv};
  PirParams pir_params;
  // Encryption parameters.
  gen_encryption_params(N, LOGT, enc_params);
  verify_encryption_params(enc_params);
  // PIR parameters.
  gen_pir_params(table_size, ITEM_SIZE, D, enc_params, pir_params, true, true, true);

  // Generate table.
  std::cout << "Main: Generate database" << std::endl;
  std::unique_ptr<uint8_t[]> db = CreateTable(table_size);

  // Initialize PIR Server
  std::cout << "Main: Initializing server and client" << std::endl;
  PIRServer server(enc_params, pir_params);

  // Initialize PIR client....
  PIRClient client(enc_params, pir_params);
  seal::GaloisKeys galois_keys = client.generate_galois_keys();
  server.set_galois_key(0, galois_keys);

  // Preprocess database on server side.
  std::cout << "Main: Preprocess database on server side" << std::endl;
  BENCHMARK(PreprocessTable(&server, table_size, Copy(db.get(), table_size)),
            preprocessing_time);

  // Make queries.
  std::random_device rd;
  for (uint64_t i = 0; i < query_count; i++) {
    if (i % 500 == 0) {
      std::cout << "Main: Query " << i << std::endl;
    }

    // Generate query on client side.
    uint64_t row = rd() % table_size;  // random row to query.

    BENCHMARK(PirQuery query = MakeQuery(&client, row), query_time);
    BENCHMARK(PirReply reply = HandleQuery(&server, &query), server_time);
    BENCHMARK(auto output = MakeOutput(&client, &reply, row), response_time);
    // if (!ValidateCorrectness(&client, db.get(), output, row)) {
       // return 1;
    // }
  }
  std::cout << "Main: Query " << query_count << std::endl;

  // Turn to milliseconds.
  preprocessing_time /= 1000;
  query_time /= 1000;
  server_time /= 1000;
  response_time /= 1000;

  std::cout << std::endl;
  std::cout << "Time taken: " << std::endl;
  std::cout << "Preprocessing: " << preprocessing_time << "ms" << std::endl;
  std::cout << "Query time: " << query_time << "ms" << std::endl;
  std::cout << "Server time: " << server_time << "ms" << std::endl;
  std::cout << "Response handling: " << response_time << "ms" << std::endl;
  std::cout << "Done!" << std::endl;

  return 0;
}
