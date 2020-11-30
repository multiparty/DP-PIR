// Copyright 2020 multiparty.org

// Socket Interface responsible for intra-party communication over TCP.
//
// Machine with id i acts a server for all sockets with other machines with
// IDs less than i.

#ifndef DRIVACY_IO_INTRAPARTY_SOCKET_H_
#define DRIVACY_IO_INTRAPARTY_SOCKET_H_

#include <poll.h>

#include <cstdint>
#include <vector>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace io {
namespace socket {

class IntraPartySocketListener {
 public:
  virtual bool OnReceiveBatchSize(uint32_t machine_id, uint32_t batch_size) = 0;
  virtual void OnCollectedBatchSizes() = 0;  // ensures FIFO msg delivery.
  virtual void OnReceiveMessage(uint32_t machine_id,
                                const types::CipherText &message) = 0;
  virtual void OnReceiveQuery(uint32_t machine_id,
                              const types::Query &query) = 0;
  virtual void OnReceiveResponse(uint32_t machine_id,
                                 const types::Response &response) = 0;
};

class IntraPartyTCPSocket : public AbstractSocket {
 public:
  IntraPartyTCPSocket(uint32_t party_id, uint32_t machine_id,
                      const types::Configuration &config,
                      IntraPartySocketListener *listener);

  ~IntraPartyTCPSocket();

  // AbstractSocket
  uint32_t FdCount() override;
  bool PollNoiseMessages(pollfd *fds) override;
  bool PollMessages(pollfd *fds) override;
  bool PollNoiseQueries(pollfd *fds) override;
  bool PollQueries(pollfd *fds) override;
  bool PollResponses(pollfd *fds) override;

  bool ReadNoiseMessage(uint32_t fd_index, pollfd *fds) override;
  bool ReadMessage(uint32_t fd_index, pollfd *fds) override;
  bool ReadNoiseQuery(uint32_t fd_index, pollfd *fds) override;
  bool ReadQuery(uint32_t fd_index, pollfd *fds) override;
  bool ReadResponse(uint32_t fd_index, pollfd *fds) override;

  // New functionality.
  void CollectBatchSizes();
  void CollectMessagesReady();
  void CollectQueriesReady();
  void CollectResponsesReady();

  void BroadcastBatchSize(uint32_t batch_size);
  void BroadcastNoiseMessageCounts(std::vector<uint32_t> &&noise_msg_counts);
  void BroadcastNoiseQueryCounts(std::vector<uint32_t> &&noise_query_counts);
  void BroadcastMessagesReady();
  void BroadcastQueriesReady();
  void BroadcastResponsesReady();

  void SendMessage(uint32_t machine_id, const types::CipherText &message);
  void SendQuery(uint32_t machine_id, const types::Query &query);
  void SendResponse(uint32_t machine_id, const types::Response &response);

  // Setting incoming queries counts.
  void SetQueryCounts(std::vector<uint32_t> &&incoming_query_counts);
  void SetMessageCounts(std::vector<uint32_t> &&incoming_message_counts);

 private:
  // Configurations.
  uint32_t party_id_;
  uint32_t machine_id_;
  uint32_t party_count_;
  uint32_t parallelism_;
  types::Configuration config_;
  IntraPartySocketListener *listener_;
  // The size of the message cipher text (both incoming and outgoing).
  uint32_t message_size_;
  // Maps a machine_id to socket with that machine.
  std::vector<int> sockets_;
  // Expected number of incoming queries and responses.
  std::vector<uint32_t> noise_message_counts_;
  std::vector<uint32_t> incoming_message_counts;
  std::vector<uint32_t> noise_query_counts_;
  std::vector<uint32_t> incoming_query_counts_;
  std::vector<uint32_t> incoming_response_counts_;
  // How many sockets are still waiting for queries or responses.
  uint32_t sockets_with_noise_messages_;
  uint32_t sockets_with_messages_;
  uint32_t sockets_with_noise_queries_;
  uint32_t sockets_with_queries_;
  uint32_t sockets_with_responses_;
  // Read buffers for storing a message that was just read from a socket.
  unsigned char *read_message_buffer_;
  types::Query read_query_buffer_;
  types::Response read_response_buffer_;
  // Synchronization points.
  void BroadcastSync();
  void CollectSync();
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_INTRAPARTY_SOCKET_H_
