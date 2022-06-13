#include <cassert>
#include <iostream>
#include <vector>

#include "DPPIR/protocol/client/client.h"
#include "DPPIR/types/containers.h"

namespace DPPIR {
namespace protocol {

void Client::StartOnline(index_t count) {
  this->queries_count_ = count;

  // Store queried keys in order (for checking response correctness).
  std::vector<key_t> queries;
  queries.reserve(count);

  // Make count queries.
  std::cout << "Queries: " << count << std::endl;
  for (index_t i = 0; i < count; i++) {
    key_t key = this->db_.RandomRow();
    queries.push_back(key);

    // Make and send query.
    this->next_.SendQuery(this->MakeQuery(key));
  }
  this->next_.FlushQueries();
  this->state_.FinishSharing();

  // Read count responses.
  std::cout << "Responses: " << count << std::endl;
  index_t read = 0;
  while (read < count) {
    LogicalBuffer<Response>& buffer = this->next_.ReadResponses(count - read);
    for (index_t i = 0; i < buffer.Size(); i++) {
      Response& response = buffer[i];

      // Reconstruct.
      this->ReconstructResponse(&response);

      // Verify signature.
      // TODO(babman): signature.

      // Ensure response is correct (testing: can't do this in reality!).
      const Response& expected = this->db_.Lookup(queries[read++]);
      assert(response == expected);
    }
    buffer.Clear();
  }
}

}  // namespace protocol
}  // namespace DPPIR
