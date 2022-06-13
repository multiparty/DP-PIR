// NOLINTNEXTLINE
#include <chrono>
#include <iostream>
// NOLINTNEXTLINE
#include <thread>

#include "DPPIR/sockets/client_socket.h"
#include "DPPIR/sockets/server_socket.h"

#define OFFLINE_MSG_SIZE 96
#define OFFLINE_COUNT 1023688
#define QUERY_COUNT 10236881
#define RESPONSE_COUNT 10236881

namespace DPPIR {
namespace sockets {

char offline[OFFLINE_MSG_SIZE * OFFLINE_COUNT];
char roffline[OFFLINE_MSG_SIZE * OFFLINE_COUNT];
Query queries[QUERY_COUNT];
Query rqueries[QUERY_COUNT];
Response responses[RESPONSE_COUNT];
Response rresponses[RESPONSE_COUNT];

void Initialize() {
  for (size_t i = 0; i < OFFLINE_MSG_SIZE * OFFLINE_COUNT; i++) {
    offline[i] = i % 69;
  }
  for (uint32_t i = 0; i < QUERY_COUNT; i++) {
    queries[i] = {i + 100, 2 * i + 5};
  }
  for (uint32_t i = 0; i < RESPONSE_COUNT; i++) {
    sig_t sig;
    for (size_t j = 0; j < sig.size(); j++) {
      sig[j] = i ^ j;
    }
    responses[i] = {i + 100, sig};
  }
}

bool IsCorrect() {
  for (size_t i = 0; i < OFFLINE_COUNT; i++) {
    char* q = offline + i * OFFLINE_MSG_SIZE;
    char* r = roffline + i * OFFLINE_MSG_SIZE;
    for (size_t j = 0; j < OFFLINE_MSG_SIZE; j++) {
      if (q[j] != r[j]) {
        std::cout << "Offline message " << i << " is wrong!" << std::endl;
        return false;
      }
    }
  }
  for (size_t i = 0; i < QUERY_COUNT; i++) {
    auto& q = queries[i];
    auto& r = rqueries[i];
    if (q.tag != r.tag || q.tally != r.tally) {
      std::cout << "Query " << i << " is wrong!" << std::endl;
      std::cout << "Expected: " << q << std::endl;
      std::cout << "Found: " << r << std::endl;
      return false;
    }
  }
  for (size_t i = 0; i < RESPONSE_COUNT; i++) {
    auto& q = responses[i];
    auto& r = rresponses[i];
    if (q != r) {
      std::cout << "Response " << i << " is wrong!" << std::endl;
      std::cout << "Expected: " << q << std::endl;
      std::cout << "Found: " << r << std::endl;
      return false;
    }
  }
  return true;
}

void Client() {
  ClientSocket client_socket(OFFLINE_MSG_SIZE);
  client_socket.Initialize("127.0.0.1", 3000);
  std::cout << "(Client) Client connected!" << std::endl;

  // Write offline messages.
  auto s = std::chrono::steady_clock::now();
  for (size_t i = 0; i < OFFLINE_COUNT; i++) {
    client_socket.SendCipher(offline + i * OFFLINE_MSG_SIZE);
  }
  client_socket.FlushCiphers();
  auto e = std::chrono::steady_clock::now();
  auto d = std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count();
  std::cout << "(Client) Sending offline done in " << d << "ms" << std::endl;

  // Wait until server reads all offline.
  client_socket.WaitForReady();

  // Write queries.
  s = std::chrono::steady_clock::now();
  for (size_t i = 0; i < QUERY_COUNT; i++) {
    client_socket.SendQuery(queries[i]);
  }
  client_socket.FlushQueries();
  e = std::chrono::steady_clock::now();
  d = std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count();
  std::cout << "(Client) Sending queries done in " << d << "ms" << std::endl;

  // Read queries.
  s = std::chrono::steady_clock::now();
  index_t read = 0;
  while (read != RESPONSE_COUNT) {
    auto& buffer = client_socket.ReadResponses(RESPONSE_COUNT - read);
    for (Response& r : buffer) {
      rresponses[read++] = r;
    }
  }
  e = std::chrono::steady_clock::now();
  d = std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count();
  std::cout << "(Client) Reading responses done in " << d << "ms" << std::endl;
}

void Server() {
  ServerSocket server_socket(OFFLINE_MSG_SIZE);
  server_socket.Initialize(3000);
  std::cout << "(Server) Server connected!" << std::endl;

  // Read offline.
  auto s = std::chrono::steady_clock::now();
  index_t read = 0;
  while (read != OFFLINE_COUNT) {
    auto& buffer = server_socket.ReadCiphers(OFFLINE_COUNT - read);
    for (char* cipher : buffer) {
      memcpy(roffline + (read++ * OFFLINE_MSG_SIZE), cipher, OFFLINE_MSG_SIZE);
    }
  }
  auto e = std::chrono::steady_clock::now();
  auto d = std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count();
  std::cout << "(Server) Reading offline done in " << d << "ms" << std::endl;

  // Send ready to mark end of offline.
  server_socket.SendReady();

  // Read queries.
  s = std::chrono::steady_clock::now();
  read = 0;
  while (read != QUERY_COUNT) {
    auto& buffer = server_socket.ReadQueries(QUERY_COUNT - read);
    for (size_t i = 0; i < buffer.Size(); i++) {
      rqueries[read++] = buffer[i];
    }
  }
  e = std::chrono::steady_clock::now();
  d = std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count();
  std::cout << "(Server) Reading queries done in " << d << "ms" << std::endl;

  // Write responses.
  s = std::chrono::steady_clock::now();
  for (size_t i = 0; i < RESPONSE_COUNT; i++) {
    server_socket.SendResponse(responses[i]);
  }
  server_socket.FlushResponses();
  e = std::chrono::steady_clock::now();
  d = std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count();
  std::cout << "(Server) Sending responses done in " << d << "ms" << std::endl;
}

}  // namespace sockets
}  // namespace DPPIR

int main() {
  // Initialize buffer.
  DPPIR::sockets::Initialize();

  // Server in a parallel thread.
  std::thread server(DPPIR::sockets::Server);

  // Perform client code.
  DPPIR::sockets::Client();

  // Wait until read is done.
  server.join();

  // Check correctness.
  if (!DPPIR::sockets::IsCorrect()) {
    return 1;
  }

  std::cout << "All correct!" << std::endl;
  return 0;
}
