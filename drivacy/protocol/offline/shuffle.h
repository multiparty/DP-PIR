// Copyright 2020 multiparty.org

// This file implements an efficient coordination-free distributed shuffling
// based on knuth shuffle and shared seeds.
//
// The shuffling is in two phases:
// 1. First, a machine has a bunch of messages to shuffle, the machine
//    bucketizes them between the different machines (including itself) in
//    the same party.
// Machines exchange the queries as per the bucketization.
// 2. Second, a machine receives a bunch of messages from various other machines
//    (including itself), and shuffles them among themselves.
//
// Instead of each machine bucketizing independently, which introduces variance
// into how many queries every machine ends up with after shuffling, we rely on
// a shared seed between all the machines.
// The machines simulate a global shuffle over all messages, including ones a
// machine never encounters physically, and then bucketize consistently so that
// every machine ends up with the same number of messages.
// This simulation is done ahead of time at initialization, and only requires
// knowledge about the counts of the messages.
// Only the relevant pieces of this simulated shuffling is stored within the
// shuffler:
// 1. For every query this machine has in phase 1, we store which machine is
//    its bucket in "machine_ids_".
// 2. For every query this machine is meant to receive in phase 2, we store
//    its index in the global shuffling order in "message_indices_".
//
// Phase 1 is encoded in MachineOfNextMessage(), and phase 2 is encoded in
// ShuffleMessage().


#ifndef DRIVACY_PROTOCOL_OFFLINE_SHUFFLE_H_
#define DRIVACY_PROTOCOL_OFFLINE_SHUFFLE_H_

#include <cstring>
#include <unordered_map>
#include <utility>
#include <vector>

#include "drivacy/primitives/util.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {
namespace offline {

class Shuffler {
 public:
  Shuffler(uint32_t party_id, uint32_t machine_id, uint32_t party_count,
           uint32_t parallelism);

  ~Shuffler();

  // Accessors.
  uint32_t batch_size() { return this->batch_size_; }

  // Initialize the shuffler to handle a new batch.
  // Initialize must be called for every machine_id, passing the input
  // batch size of that machine.
  // When all machine input batch sizes are specified, Initialize returns
  // true, indicating that initialization is complete.
  bool Initialize(uint32_t machine_id, uint32_t size);

  // After the shuffler is initialized with the various batch size at every
  // machine, this function should be called to simulate a global shuffle,
  // record the relevant portions of the shuffling and deshuffling orders,
  // for later use during the actual online (phase 2) shuffling.
  void PreShuffle();

  // Returns a vector (logically a map) from a machine_id to the count
  // of queries expected to be received from that machine.
  std::vector<uint32_t> IncomingMessagesCount();
  std::vector<uint32_t> OutgoingNoiseMessagesCount(uint32_t noise_size);

  // Determines the machine the next message is meant to be sent to.
  uint32_t MachineOfNextMessage();

  // Shuffle message into shuffler.
  bool ShuffleMessage(uint32_t machine_id, const types::CipherText message);

  // Get the next query in the shuffled order.
  types::CipherText NextMessage();

  // Free allocated memory.
  void FreeMessages();

 private:
  // Configurations.
  uint32_t party_id_;
  uint32_t machine_id_;
  uint32_t parallelism_;
  uint32_t message_size_;
  // Batch size configurations.
  std::unordered_map<uint32_t, uint32_t> size_;
  uint32_t batch_size_;
  uint32_t total_size_;
  uint32_t shuffled_message_count_;
  // Seeded RNG.
  primitives::util::Generator generator_;
  // Current index of the next query or response to shuffle or deshuffle.
  uint32_t message_index_;
  // Shuffled messages meant for this machine.
  // These vectors are filled in incrementally as messages are
  // added in using ShuffleMessage().
  unsigned char *shuffled_messages_;

  // Maps storing relavent information from simulated shuffling.
  // message_machine_ids_index_[i] = machine-bucket of the ith message.
  uint32_t message_machine_ids_index_;
  std::vector<uint32_t> message_machine_ids_;
  // message_indices_[m][i] = index in shuffled_messages_ of the ith message
  //                          received from machine m (phase 2).
  std::unordered_map<uint32_t, std::pair<size_t, std::vector<uint32_t>>>
      message_indices_;
};

}  // namespace offline
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_OFFLINE_SHUFFLE_H_
