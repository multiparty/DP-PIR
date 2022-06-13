#include "DPPIR/types/database.h"

// NOLINTNEXTLINE
#include "sodium.h"

namespace DPPIR {

Database::Database(index_t rows) : size_(rows) {
  this->db_ = std::make_unique<Response[]>(rows);
  for (index_t i = 0; i < rows; i++) {
    this->db_[i].value = 2 * i;
    this->db_[i].sig.fill(i % 128);
  }
}

key_t Database::RandomRow() const { return randombytes_uniform(this->size_); }

const Response& Database::Lookup(key_t key) const { return this->db_[key]; }

}  // namespace DPPIR
