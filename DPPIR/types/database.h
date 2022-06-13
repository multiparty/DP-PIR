#ifndef DPPIR_TYPES_DATABASE_H_
#define DPPIR_TYPES_DATABASE_H_

#include <memory>

#include "DPPIR/types/types.h"

namespace DPPIR {

class Database {
 public:
  // Constructs a database with keys in [0, rows).
  explicit Database(index_t rows);

  // Get a random key/row.
  key_t RandomRow() const;

  // Lookup by key.
  const Response& Lookup(key_t key) const;

  // Number of rows.
  index_t Size() const { return this->size_; }

 private:
  index_t size_;
  std::unique_ptr<Response[]> db_;
};

}  // namespace DPPIR

#endif  // DPPIR_TYPES_DATABASE_H_
