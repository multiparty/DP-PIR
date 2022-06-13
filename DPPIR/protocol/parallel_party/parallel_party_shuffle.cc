#include <iostream>

#include "DPPIR/protocol/parallel_party/parallel_party.h"

namespace DPPIR {
namespace protocol {

void ParallelParty::FromSibling(server_id_t source, const char* cipher) {
  // Find the index of this query in this server queue.
  index_t source_start = this->pshuffler_.PrefixSumCountFromServer(source);
  index_t source_count = this->received_from_sibling_counts_[source]++;
  // Shuffle the cipher based on its index.
  index_t target = this->lshuffler_.Shuffle(source_start + source_count);
  this->out_ciphers_.SetShort(target, cipher);
}

void ParallelParty::FromSibling(server_id_t source, const Query& query) {
  // Find the index of this query in this server queue.
  index_t source_start = this->pshuffler_.PrefixSumCountFromServer(source);
  index_t source_count = this->received_from_sibling_counts_[source]++;
  // Shuffle the query based on its index.
  index_t target = this->lshuffler_.Shuffle(source_start + source_count);
  this->out_queries_[target] = query;
}

void ParallelParty::FromSibling(server_id_t source, const Response& response) {
  this->received_from_sibling_counts_[source]++;
  // Find the index of this response ignoring noise responses which are dropped
  // by sibling.
  index_t idx = 0;
  do {
    idx = this->pshuffler_.DeshuffleOne(source);
  } while (idx < this->noise_count_);
  // Handle the response and store it.
  idx = idx - this->noise_count_;
  tag_t& tag = this->in_tags_[idx];
  this->HandleResponse(tag, response, &this->out_responses_[idx]);
}

void ParallelParty::ShuffleCiphers() {
  std::cout << "Shuffling ciphers with siblings..." << std::endl;

  // Allocate memory.
  this->out_ciphers_.Initialize(this->shuffled_count_, 0);

  // Figure out how many ciphers to read from each server.
  index_t total_read = this->shuffled_count_;
  ServersMap<index_t> from_counts(this->server_id_, this->server_count_);
  for (server_id_t id = 0; id < this->server_count_; id++) {
    index_t count = this->pshuffler_.CountFromServer(id);
    if (id != this->server_id_) {
      from_counts[id] = count;
    } else {
      total_read -= count;
    }
  }

  // Shuffle in_ciphers_ with siblings while reading the ciphers they send us.
  size_t poll_rate = POLL_RATE / this->output_cipher_size_;
  this->SendAndPoll<char*>(
      &this->in_ciphers_, poll_rate, this->noise_count_ + this->input_count_,
      total_read, from_counts,
      std::function<void(const char*)>([&, this](const char* cipher) {
        server_id_t target = this->pshuffler_.ShuffleOne();
        if (target == this->server_id_) {
          this->FromSibling(target, cipher);
        } else {
          this->siblings_.SendCipher(target, cipher);
        }
      }),
      std::function<index_t(server_id_t, index_t)>(
          [this](server_id_t source, index_t remaining) {
            CipherLogicalBuffer& buffer =
                this->siblings_.ReadCiphers(source, remaining);
            for (char* cipher : buffer) {
              this->FromSibling(source, cipher);
            }
            index_t size = buffer.Size();
            buffer.Clear();
            return size;
          }));

  // Free consumed memory.
  this->in_ciphers_.Free();
  this->pshuffler_.FinishForward();
  this->lshuffler_.FinishForward();

  // Wait until silbings are done shuffling.
  std::cout << "Waiting for siblings..." << std::endl;
  this->siblings_.BroadcastReady();
  this->siblings_.WaitForReady();
}

void ParallelParty::ShuffleQueries() {
  std::cout << "Shuffling queries with siblings..." << std::endl;

  // Allocate memory.
  this->out_queries_.Initialize(this->shuffled_count_);

  // Figure out how many queries to read from each server.
  index_t total_read = this->shuffled_count_;
  ServersMap<index_t> from_counts(this->server_id_, this->server_count_);
  for (server_id_t id = 0; id < this->server_count_; id++) {
    index_t count = this->pshuffler_.CountFromServer(id);
    if (id != this->server_id_) {
      from_counts[id] = count;
    } else {
      total_read -= count;
    }
  }

  // Shuffle in_queries_ with siblings while reading the ciphers they send us.
  size_t poll_rate = POLL_RATE / sizeof(Query);
  this->SendAndPoll<Query>(
      &this->in_queries_, poll_rate, this->noise_count_ + this->input_count_,
      total_read, from_counts,
      std::function<void(const Query&)>([this](const Query& query) {
        server_id_t target = this->pshuffler_.ShuffleOne();
        if (target == this->server_id_) {
          this->FromSibling(target, query);
        } else {
          this->siblings_.SendQuery(target, query);
        }
      }),
      std::function<index_t(server_id_t, index_t)>(
          [this](server_id_t source, index_t remaining) {
            LogicalBuffer<Query>& buffer =
                this->siblings_.ReadQueries(source, remaining);
            for (Query& query : buffer) {
              this->FromSibling(source, query);
            }
            index_t size = buffer.Size();
            buffer.Clear();
            return size;
          }));

  // Free consumed memory.
  this->in_queries_.Free();
  this->pshuffler_.FinishForward();
  this->lshuffler_.FinishForward();

  // Wait until silbings are done shuffling.
  std::cout << "Waiting for siblings..." << std::endl;
  this->siblings_.BroadcastReady();
  this->siblings_.WaitForReady();
}

void ParallelParty::DeshuffleResponses() {
  std::cout << "Deshuffling responses with siblings..." << std::endl;

  // Allocate memory.
  this->out_responses_.Initialize(this->input_count_);

  // Figure out how many responses to read from each server.
  index_t total_read = this->input_count_;
  ServersMap<index_t> from_counts(this->server_id_, this->server_count_);
  for (server_id_t id = 0; id < this->server_count_; id++) {
    index_t non_noise_count = this->pshuffler_.CountToServer(id) -
                              this->pshuffler_.CountNoiseToServer(id);
    if (id != this->server_id_) {
      from_counts[id] = non_noise_count;
    } else {
      total_read -= non_noise_count;
    }
  }

  // Shuffle in_responses_ with siblings while reading the ciphers they send us.
  // Note that in_responses_ already has all responses corresponding by noise
  // added by any server from this party filtered out (in CollectResponses).
  index_t index = 0;
  server_id_t target = 0;
  index_t target_end = this->pshuffler_.PrefixSumCountFromServer(target + 1);
  target_end -= this->noise_from_sibling_prefixsum_[target];
  size_t poll_rate = POLL_RATE / sizeof(Response);
  this->SendAndPoll<Response>(
      &this->in_responses_, poll_rate, this->input_count_, total_read,
      from_counts,
      std::function<void(const Response&)>([&, this](const Response& response) {
        while (index >= target_end && target < this->server_count_ - 1) {
          target++;
          if (target < this->server_count_ - 1) {
            target_end = this->pshuffler_.PrefixSumCountFromServer(target + 1);
            target_end -= this->noise_from_sibling_prefixsum_[target];
          }
        }
        if (target == this->server_id_) {
          this->FromSibling(target, response);
        } else {
          this->siblings_.SendResponse(target, response);
        }
        index++;
      }),
      std::function<index_t(server_id_t, index_t)>(
          [this](server_id_t source, index_t remaining) {
            LogicalBuffer<Response>& buffer =
                this->siblings_.ReadResponses(source, remaining);
            for (Response& response : buffer) {
              this->FromSibling(source, response);
            }
            index_t size = buffer.Size();
            buffer.Clear();
            return size;
          }));

  // Free consumed memory.
  this->in_responses_.Free();
  this->pshuffler_.FinishBackward();

  // Wait until silbings are done shuffling.
  std::cout << "Waiting for siblings..." << std::endl;
  this->siblings_.BroadcastReady();
  this->siblings_.WaitForReady();
}

}  // namespace protocol
}  // namespace DPPIR
