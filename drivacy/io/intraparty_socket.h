// Copyright 2020 multiparty.org

// Socket Interface responsible for intra-party communication over TCP.
//
// Machine with id i acts a server for all sockets with other machines with
// IDs less than i.

#ifndef DRIVACY_IO_INTRAPARTY_SOCKET_H_
#define DRIVACY_IO_INTRAPARTY_SOCKET_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace io {
namespace socket {

class IntraPartyTCPSocket : public AbstractIntraPartySocket {
 public:
  IntraPartyTCPSocket(uint32_t party_id, uint32_t machine_id,
                      const types::Configuration &config,
                      IntraPartySocketListener *listener);

  // Free internal write buffers.
  ~IntraPartyTCPSocket() {
    delete[] this->read_query_buffer_;
    delete[] this->read_response_buffer_;
  }

  static std::unique_ptr<AbstractIntraPartySocket> Factory(
      uint32_t party_id, uint32_t machine_id,
      const types::Configuration &config, IntraPartySocketListener *listener) {
    return std::make_unique<IntraPartyTCPSocket>(party_id, machine_id, config,
                                                 listener);
  }

  void ListenBatchSizes() override;
  void ListenQueries(std::vector<uint32_t> counts) override;
  void ListenResponses() override;

  void BroadcastBatchSize(uint32_t batch_size) override;
  void BroadcastQueriesReady() override;
  void BroadcastResponsesReady() override;

  void SendQuery(uint32_t machine_id,
                 const types::OutgoingQuery &query) override;
  void SendResponse(uint32_t machine_id,
                    const types::ForwardResponse &response) override;

 private:
  // Maps a machine_id to socket with that machine.
  std::vector<int> sockets_;
  // Read buffers for storing a message that was just read from a socket.
  unsigned char *read_query_buffer_;
  unsigned char *read_response_buffer_;
  // counts of queries sent within a batch mapped by the target machine id.
  std::vector<uint32_t> outgoing_counts_;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_INTRAPARTY_SOCKET_H_
