// Copyright 2020 multiparty.org

// This file defines the "HeadParty" class. A specialized Party that interfaces
// with clients.
//
// All comments about deploying/running parties in simulation or deployment
// in drivacy/parties/party.h apply here too.

#ifndef DRIVACY_PARTIES_ONLINE_HEAD_PARTY_H_
#define DRIVACY_PARTIES_ONLINE_HEAD_PARTY_H_

#include <cstdint>
#include <memory>

#include "drivacy/io/websocket_server.h"
#include "drivacy/parties/online/party.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace parties {
namespace online {

// HeadParty is the special first party, which is responsible for communicating
// with other parties as well as clients!
class HeadParty : public Party, public io::socket::WebSocketServerListener {
 public:
  HeadParty(uint32_t party, uint32_t machine,
            const types::Configuration &config, const types::Table &table,
            double span, double cutoff, uint32_t batch_size)
      : Party(party, machine, config, table, span, cutoff),
        client_socket_(party, machine, true, config, this),
        processed_client_requests_(0),
        initial_batch_size_(batch_size) {}

  // Not copyable or movable!
  HeadParty(HeadParty &&other) = delete;
  HeadParty &operator=(HeadParty &&other) = delete;
  HeadParty(const HeadParty &) = delete;
  HeadParty &operator=(const HeadParty &) = delete;

  // Start listening on the sockets (blocking!)
  void Start() override;
  void Continue();

  void OnReceiveQuery(const types::Query &query) override;

  // Send responses via the client socket instead of the default socket!
  void SendResponses() override;

  // Useless.
  void OnReceiveMessage(const types::CipherText &message) { assert(false); }

 protected:
  // WebSocket server.
  io::socket::WebSocketServer client_socket_;
  // Various counters to keep track of how far we are in a batch.
  uint32_t processed_client_requests_;
  const uint32_t initial_batch_size_;
};

}  // namespace online
}  // namespace parties
}  // namespace drivacy

#endif  // DRIVACY_PARTIES_ONLINE_HEAD_PARTY_H_
