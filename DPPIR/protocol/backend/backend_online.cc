#include <iostream>

#include "DPPIR/protocol/backend/backend.h"

namespace DPPIR {
namespace protocol {

void BackendParty::CollectQueries() {
  // Listen to all incoming queries.
  std::cout << "Listening to " << this->queries_.Capacity() << " queries..."
            << std::endl;
  index_t count = this->queries_.Capacity();
  while (count > 0) {
    LogicalBuffer<Query>& buffer = this->back_.ReadQueries(count);
    for (Query& query : buffer) {
      this->queries_.PushBack(query);
    }
    count -= buffer.Size();
    buffer.Clear();
  }
}

void BackendParty::SendResponses() {
  // Handle queries and send responses.
  std::cout << "Handling responses..." << std::endl;
  for (Query& query : this->queries_) {
    Response response = this->HandleQuery(query);
    this->back_.SendResponse(response);
  }
  this->back_.FlushResponses();
}

void BackendParty::StartOnline() {
  this->CollectQueries();
  this->SendResponses();
}

}  // namespace protocol
}  // namespace DPPIR
