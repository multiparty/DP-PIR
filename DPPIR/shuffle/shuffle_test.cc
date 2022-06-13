#include <cassert>
// NOLINTNEXTLINE
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "DPPIR/shuffle/local_shuffle.h"
#include "DPPIR/shuffle/parallel_shuffle.h"
#include "DPPIR/types/types.h"

#define SERVER_COUNT 8
#define TOTAL_COUNT 10000006

// Whether to print intermediate batches.
#define PRINT false

namespace DPPIR {
namespace shuffle {

index_t InputCountForServer(server_id_t server) {
  index_t uniform = TOTAL_COUNT / SERVER_COUNT;
  if (server == SERVER_COUNT - 1) {
    uniform = TOTAL_COUNT - (uniform * server);
  }

  // Introduce ~1% variance in input size.
  index_t offset = (TOTAL_COUNT / SERVER_COUNT) / 20;
  assert(offset < uniform && offset > 0);
  if (server % 2 == 0) {
    return uniform + offset;
  } else {
    return uniform - offset;
  }
}

// Printing utils.
template <typename T>
void Print(server_id_t sid, const std::string& label, const std::vector<T>& v) {
  if (PRINT) {
    std::cout << "(server " << int(sid) << ") " << label << ": [";
    for (const auto& q : v) {
      std::cout << q << ", ";
    }
    std::cout << "] @ " << v.size();
    std::cout << std::endl;
  }
}
template <typename T>
void Print2D(server_id_t sid, const std::string& label,
             const std::vector<std::vector<T>>& v) {
  if (PRINT) {
    for (size_t i = 0; i < v.size(); i++) {
      std::cout << "(server " << int(sid) << ") " << label << " from " << i
                << ": [";
      for (const auto& q : v.at(i)) {
        std::cout << q << ", ";
      }
      std::cout << "] @ " << v.at(i).size();
      std::cout << std::endl;
    }
  }
}

// Server struct.
struct Server {
  server_id_t id;

  // Shufflers.
  ParallelShuffler pshuffler;
  LocalShuffler lshuffler;
  std::vector<key_t> inputs;

  // Going forward.
  std::vector<std::vector<key_t>> forward_from_servers;
  std::vector<key_t> outbox;

  // Output of shuffling.
  // Going backward.
  std::vector<key_t> inbox;
  std::vector<std::vector<key_t>> backward_from_servers;

  // Should be == inputs.
  std::vector<key_t> outputs;

  // Constructor.
  Server(server_id_t sid, index_t* server_counts)
      : id(sid),
        pshuffler(sid, SERVER_COUNT, SERVER_COUNT),
        lshuffler(sid),
        forward_from_servers(SERVER_COUNT, std::vector<key_t>()),
        backward_from_servers(SERVER_COUNT, std::vector<key_t>()) {
    // Initialize parallel shuffler.
    pshuffler.Initialize(server_counts, 0);
    index_t in_slice = server_counts[sid];
    index_t out_slice = pshuffler.GetServerSliceSize();

    // Initialize local shuffler.
    lshuffler.Initialize(out_slice);

    // Initialize vectors.
    inputs.reserve(in_slice);
    outbox = std::vector<key_t>(out_slice, -1);
    inbox = std::vector<key_t>(out_slice, -1);
    outputs = std::vector<key_t>(in_slice, -1);
    for (index_t i = 0; i < in_slice; i++) {
      inputs.push_back(sid * TOTAL_COUNT + i);
    }
  }
};

// Measure time for a single preshuffle.
void SingleOffline() {
  // Give every server an input size.
  std::vector<index_t> input_counts;
  for (server_id_t sid = 0; sid < SERVER_COUNT; sid++) {
    input_counts.push_back(InputCountForServer(sid));
  }

  // Create shuffler.
  ParallelShuffler _pshuffler(SERVER_COUNT - 1, SERVER_COUNT, SERVER_COUNT);
  LocalShuffler _lshuffler(0);

  // Time initialization.
  auto s = std::chrono::steady_clock::now();
  _pshuffler.Initialize(&input_counts.at(0), 0);
  _lshuffler.Initialize(_pshuffler.GetServerSliceSize());
  auto e = std::chrono::steady_clock::now();
  auto d = std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count();

  // Done.
  std::cout << "Preprocessing: " << d << "ms" << std::endl;
  std::cout << std::endl;
}

bool SimpleProtocol() {
  // Give every server an input size.
  std::vector<index_t> input_counts;
  for (server_id_t sid = 0; sid < SERVER_COUNT; sid++) {
    input_counts.push_back(InputCountForServer(sid));
  }

  // Create server structs.
  std::vector<Server> servers;
  servers.reserve(SERVER_COUNT);
  for (server_id_t sid = 0; sid < SERVER_COUNT; sid++) {
    servers.emplace_back(sid, &input_counts.at(0));
  }

  // First stage.
  std::cout << "First stage" << std::endl;
  for (auto& server : servers) {
    Print(server.id, "inputs", server.inputs);
    for (auto& q : server.inputs) {
      server_id_t tserver = server.pshuffler.ShuffleOne();
      servers.at(tserver).forward_from_servers.at(server.id).push_back(q);
    }
    server.pshuffler.FinishForward();
  }

  // Print state.
  for (auto& server : servers) {
    Print2D(server.id, "forward", server.forward_from_servers);
  }
  std::cout << std::endl;

  // Second stage.
  std::cout << "Second stage" << std::endl;
  for (auto& server : servers) {
    std::vector<key_t> before_shuffle;
    for (auto& v : server.forward_from_servers) {
      for (auto& q : v) {
        index_t target_idx = server.lshuffler.Shuffle(before_shuffle.size());
        server.outbox.at(target_idx) = q;
        before_shuffle.push_back(q);
      }
    }
    server.lshuffler.FinishForward();
    Print(server.id, "outbox (no shuffle)", before_shuffle);
    Print(server.id, "outbox: ", server.outbox);
  }
  std::cout << std::endl;

  // Reverse second stage.
  std::cout << "Reverse second stage" << std::endl;
  for (auto& server : servers) {
    for (index_t idx = 0; idx < server.outbox.size(); idx++) {
      index_t target_idx = server.lshuffler.Deshuffle(idx);
      server.inbox.at(target_idx) = server.outbox.at(idx);
    }
    server.lshuffler.FinishBackward();
    Print(server.id, "inbox (deshuffled)", server.inbox);
    // Send to servers.
    index_t acc = 0;
    for (server_id_t tserver = 0; tserver < SERVER_COUNT; tserver++) {
      index_t count = server.forward_from_servers.at(tserver).size();
      for (index_t i = 0; i < count; i++) {
        auto& q = server.inbox.at(i + acc);
        servers.at(tserver).backward_from_servers.at(server.id).push_back(q);
      }
      acc += count;
    }
  }

  // Print state.
  for (auto& server : servers) {
    Print2D(server.id, "backward", server.backward_from_servers);
  }
  std::cout << std::endl;

  // Reverse first stage.
  std::cout << "Reverse first stage" << std::endl;
  for (auto& server : servers) {
    for (server_id_t tserver = 0; tserver < SERVER_COUNT; tserver++) {
      auto& v = server.backward_from_servers.at(tserver);
      for (auto& q : v) {
        index_t target_idx = server.pshuffler.DeshuffleOne(tserver);
        server.outputs.at(target_idx) = q;
      }
    }
    server.pshuffler.FinishBackward();
    Print(server.id, "output", server.outputs);
    if (server.outputs != server.inputs) {
      return false;
    }
  }
  std::cout << std::endl;
  return true;
}

}  // namespace shuffle
}  // namespace DPPIR

// Main function.
int main(int argc, char** argv) {
  // Start.
  std::cout << "starting... " << std::endl;

  // Preprocessing.
  DPPIR::shuffle::SingleOffline();

  // Protocol.
  if (!DPPIR::shuffle::SimpleProtocol()) {
    std::cout << "error!" << std::endl;
    return 1;
  }

  // Done.
  std::cout << "Success!" << std::endl;
  return 0;
}
