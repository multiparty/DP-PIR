// Copyright 2020 multiparty.org

// This file contains an implementation of OutputStatus class, which
// represents the output of a function.
//
// OutputStatus either contains a value or an absl error, depending
// on whether the function was successful or not.
//
// This is inspired by absl::Status and by google's gutil::StatusOr
// https://github.com/google/p4-pdpi/blob/master/gutil/status.h

#ifndef DRIVACY_UTIL_STATUS_H_
#define DRIVACY_UTIL_STATUS_H_

#include <utility>

#include "absl/status/status.h"
#include "absl/types/optional.h"

namespace drivacy {
namespace util {

template <typename T>
class ABSL_MUST_USE_RESULT OutputStatus {
 public:
  // NOLINTNEXTLINE
  OutputStatus(T &&value)
      : status_(absl::OkStatus()), value_(std::move(value)) {}
  // NOLINTNEXTLINE
  OutputStatus(const absl::Status &status) : status_(status) {
    assert(!status.ok());
  }
  // NOLINTNEXTLINE
  OutputStatus(absl::Status &&status) : status_(std::move(status)) {
    assert(!status.ok());
  }

  bool ok() const { return this->status_.ok(); }
  const absl::Status &status() const { return this->status_; }
  const T &value() const & { return this->value_.value(); }
  T &&value() const && { return this->value_.value(); }

 private:
  absl::Status status_;
  absl::optional<T> value_;
};

#define CHECK_STATUS(status) \
  if (!status.ok()) return status

#define __ASSIGN_OR_RETURN_VAR_NAME(arg) __ASSIGN_OR_RETURN_RESULT_##arg
#define __ASSIGN_OR_RETURN_VAL(arg) __ASSIGN_OR_RETURN_VAR_NAME(arg)
#define ASSIGN_OR_RETURN(lexpr, rexpr)                \
  auto __ASSIGN_OR_RETURN_VAL(__LINE__) = rexpr;      \
  if (!__ASSIGN_OR_RETURN_VAL(__LINE__).ok())         \
    return __ASSIGN_OR_RETURN_VAL(__LINE__).status(); \
  lexpr = __ASSIGN_OR_RETURN_VAL(__LINE__).value()

}  // namespace util
}  // namespace drivacy

#endif  // DRIVACY_UTIL_STATUS_H_
