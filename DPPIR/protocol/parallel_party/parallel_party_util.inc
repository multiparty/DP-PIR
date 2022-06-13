#define IS_CIPHER std::is_same<T, char*>::value

template <typename T,  // type of what's being written (Query, Response, ...).
          typename R = std::conditional<IS_CIPHER, const char*, const T&>,
          typename C = std::conditional<IS_CIPHER, CipherBatch, Batch<T>>>
void SendAndPoll(
    C* collection, index_t poll_rate, index_t total_progress,
    // # things to read from all servers.
    index_t read_count,
    // # things to read from each server.
    const ServersMap<index_t>& servers_read_count,
    const std::function<void(R)>& send_handler,
    const std::function<index_t(server_id_t, index_t)>& read_handler) {
  // Maps to keep track of each sibling status.
  ServersMap<bool> polled(this->server_id_, this->server_count_, false);
  this->received_from_sibling_counts_ =
      ServersMap(this->server_count_, this->server_count_ + 1, 0u);

  // Set the polling servers.
  this->siblings_.ResetServers();
  for (server_id_t id = 0; id < this->server_count_; id++) {
    if (id != this->server_id_) {
      if (servers_read_count[id] == 0) {
        this->siblings_.IgnoreServer(id);
      }
    }
  }

  index_t sent = 0;
  // Broadcast the secret while also polling periodically and reading whatever
  // is received to keep buffers from filling up.
  auto it = collection->begin();
  auto end = collection->end();
  while (it != end) {
    // Send until poll rate is reach or we have sent everything.
    for (index_t i = 0; it != end && (i < poll_rate || read_count == 0); i++) {
      if (++sent % PROGRESS_RATE == 0) {
        std::cout << "Progress " << (sent - 1) << " / " << total_progress
                  << std::endl;
      }
      send_handler(*(it++));
    }

    // No more to read; continue to send more if any remain.
    if (read_count == 0) {
      continue;
    }

    // Keep polling until no more data is available to read, or no more things
    // are expected.
    bool found_data;
    do {
      found_data = false;
      server_id_t npoll = this->siblings_.Poll(&polled, 0);
      for (server_id_t source = 0; npoll > 0; source++) {
        // Check if we have something to read from source.
        if (source != this->server_id_ && polled[source]) {
          npoll--;
          polled[source] = false;
          index_t remaining = servers_read_count[source] -
                              this->received_from_sibling_counts_[source];
          if (remaining > 0) {
            found_data = true;
            // Read at most remaining many things.
            index_t count = read_handler(source, remaining);
            // Update counts.
            read_count -= count;
            index_t recevied = this->received_from_sibling_counts_[source];
            if (recevied == servers_read_count[source]) {
              this->siblings_.IgnoreServer(source);
            }
          }
        }
      }
    } while (found_data && read_count > 0);
  }

  // Flush socket after sending is done.
  if constexpr (std::is_same<T, char*>::value) {
    this->siblings_.FlushCiphers();
  } else if constexpr (std::is_same<T, OfflineSecret>::value) {
    this->siblings_.FlushSecrets();
  } else if constexpr (std::is_same<T, Query>::value) {
    this->siblings_.FlushQueries();
  } else if constexpr (std::is_same<T, Response>::value) {
    this->siblings_.FlushResponses();
  }

  // Continue reading any remaining messages from siblings until all are
  // consumed.
  while (read_count > 0) {
    server_id_t npoll = this->siblings_.Poll(&polled, -1);
    assert(npoll > 0);
    for (server_id_t source = 0; npoll > 0; source++) {
      // Check if we have something to read from source.
      if (source != this->server_id_ && polled[source]) {
        npoll--;
        polled[source] = false;
        index_t remaining = servers_read_count[source] -
                            this->received_from_sibling_counts_[source];
        if (remaining > 0) {
          // Read at most remaining many things.
          index_t count = read_handler(source, remaining);
          // Update counts.
          read_count -= count;
          index_t server_count = this->received_from_sibling_counts_[source];
          if (server_count == servers_read_count[source]) {
            this->siblings_.IgnoreServer(source);
          }
        }
      }
    }
  }

  // Reset all servers so future polls ignores no one.
  this->siblings_.ResetServers();
}
