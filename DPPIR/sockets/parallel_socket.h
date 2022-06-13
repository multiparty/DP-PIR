#ifndef DPPIR_SOCKETS_PARALLEL_SOCKET_H_
#define DPPIR_SOCKETS_PARALLEL_SOCKET_H_

#include <poll.h>

#include "DPPIR/config/config.h"
#include "DPPIR/sockets/consts.h"
#include "DPPIR/types/containers.h"
#include "DPPIR/types/types.h"

namespace DPPIR {
namespace sockets {

// A collection of server_count - 1 sockets.
class ParallelSocket {
 public:
  ParallelSocket(server_id_t server_id, server_id_t server_count,
                 size_t cipher_size);

  // Connect to all other servers.
  void Initialize(const config::PartyConfig& config);

  // Logistics.
  void SendCount(server_id_t target, index_t count);
  void BroadcastCount(index_t count);
  index_t ReadCount(server_id_t id);
  void BroadcastReady();
  void WaitForReady();

  // Poll (but do not read).
  server_id_t Poll(ServersMap<bool>* outs, int timeout);
  void IgnoreServer(server_id_t id);
  void ResetServers();

  // Read from a specific server (poll tells you who to read from).
  CipherLogicalBuffer& ReadCiphers(server_id_t source, index_t read_count);
  LogicalBuffer<OfflineSecret>& ReadSecrets(server_id_t source,
                                            index_t read_count);
  LogicalBuffer<Query>& ReadQueries(server_id_t source, index_t read_count);
  LogicalBuffer<Response>& ReadResponses(server_id_t source,
                                         index_t read_count);

  // Writing API: same as singular socket but takes the target server_id as arg.
  void SendCipher(server_id_t target, const char* onion_cipher);
  void SendQuery(server_id_t target, const Query& query);
  void SendResponse(server_id_t target, const Response& response);
  void BroadcastSecret(const OfflineSecret& secret);  // to all servers.

  // Flush API: either for a specific parallel server or for all.
  void FlushCiphers();
  void FlushSecrets();
  void FlushQueries();
  void FlushResponses();
  void FlushCiphers(server_id_t id);
  void FlushSecrets(server_id_t id);
  void FlushQueries(server_id_t id);
  void FlushResponses(server_id_t id);

 private:
  // Count of parallel servers.
  server_id_t server_id_;
  server_id_t server_count_;
  // Sockets to the parallel servers.
  // We act as a server to all servers < server_id_ which connect to as clients.
  // We connect as a client to all servers > server_id_.
  ServersMap<int> sockfds_;
  ServersMap<pollfd> pollfds_;
  // Physical buffer: we use a single read, but write buffers are per sibling.
  ServersMap<PhysicalBuffer<BUFFER_SIZE>> rbuffers_;
  ServersMap<PhysicalBuffer<BUFFER_SIZE>> wbuffers_;
  // Logical buffers that wrap the physical buffers with message types.
  // Offline onion cipher buffers.
  ServersMap<CipherLogicalBuffer> cipher_rbufs_;
  ServersMap<CipherLogicalBuffer> cipher_wbufs_;
  // Offline secrets buffers (so that all siblings have all the secrets).
  ServersMap<LogicalBuffer<OfflineSecret>> secret_rbufs_;
  ServersMap<LogicalBuffer<OfflineSecret>> secret_wbufs_;
  // Online query buffers.
  ServersMap<LogicalBuffer<Query>> query_rbufs_;
  ServersMap<LogicalBuffer<Query>> query_wbufs_;
  // Online response buffers.
  ServersMap<LogicalBuffer<Response>> response_rbufs_;
  ServersMap<LogicalBuffer<Response>> response_wbufs_;
};

}  // namespace sockets
}  // namespace DPPIR

#endif  // DPPIR_SOCKETS_PARALLEL_SOCKET_H_
