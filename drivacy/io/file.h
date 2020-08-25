// Copyright 2020 multiparty.org

// Helpers for file and file protobuf IO.

#ifndef DRIVACY_IO_FILE_H_
#define DRIVACY_IO_FILE_H_

#include <string>

#include "absl/status/status.h"
#include "drivacy/proto/table.pb.h"
#include "drivacy/util/status.h"

namespace drivacy {
namespace io {
namespace file {

util::OutputStatus<std::string> ReadFile(const std::string &path);

util::OutputStatus<proto::Table> ParseTable(const std::string &json);

// Read content of the json file at the given path, and uses protobuf to parse
// it and store it inside protobuf.
// Returns an absl error if file reading or json parsing had issues.
template <typename T>
absl::Status ReadProtobufFromJson(const std::string &path, T *protobuf);

}  // namespace file
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_FILE_H_
