// Helps for file and file protobuf IO.

#include <string>

#include "drivacy/proto/table.pb.h"
#include "drivacy/util/status.h"

#ifndef IO_FILE_H_
#define IO_FILE_H_

namespace drivacy {
namespace io {
namespace file {

util::OutputStatus<std::string> ReadFile(const std::string &path);

util::OutputStatus<proto::Table> ParseTable(const std::string &json);

}  // namespace file
}  // namespace io
}  // namespace drivacy

#endif  // IO_FILE_H_
