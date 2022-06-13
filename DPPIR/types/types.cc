#include "DPPIR/types/types.h"

namespace DPPIR {

// Debugging/Printing.
std::ostream& operator<<(std::ostream& o, const Query& q) {
  return o << "{tag: " << q.tag << ", tally: " << q.tally << "}";
}

std::ostream& operator<<(std::ostream& o, const Response& r) {
  o << "{value: " << r.value << ", sig: [" << static_cast<int>(r.sig[0]);
  for (size_t i = 1; i < r.sig.size(); i++) {
    o << ", " << static_cast<int>(r.sig[i]);
  }
  o << "]}";
  return o;
}

// Equality of responses.
bool operator==(const Response& l, const Response& r) {
  if (l.value != r.value) {
    return false;
  }
  for (size_t i = 0; i < l.sig.size(); i++) {
    if (l.sig[i] != r.sig[i]) {
      return false;
    }
  }
  return true;
}
bool operator!=(const Response& l, const Response& r) { return !(l == r); }

}  // namespace DPPIR
