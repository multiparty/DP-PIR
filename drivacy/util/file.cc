// Copyright 2020 multiparty.org

// Helpers for file and file protobuf IO.

#include "drivacy/util/file.h"

#include <cerrno>
#include <fstream>
#include <streambuf>
#include <string>

#include "absl/strings/str_cat.h"
#include "drivacy/types/config.pb.h"
#include "google/protobuf/util/json_util.h"

namespace drivacy {
namespace util {
namespace file {

util::OutputStatus<std::string> ReadFile(const std::string &path) {
  std::ifstream f;
  f.open(path.c_str());
  if (f.fail()) {
    return absl::NotFoundError(absl::StrCat("Cannot open file at path ", path));
  }
  std::string result(std::istreambuf_iterator<char>(f),
                     (std::istreambuf_iterator<char>()));
  if (f.bad()) {
    return absl::UnknownError(
        absl::StrCat("Cannot read file ", path, " errno = ", errno));
  }
  f.close();
  return result;
}

types::Table ParseTable(uint32_t table_size) {
  types::Table table;
  table.reserve(table_size);
  for (uint32_t i = 0; i < table_size; i++) {
    table.push_back(i + 10);
  }
  return table;
}

template <typename T>
absl::Status ReadProtobufFromJson(const std::string &path, T *protobuf) {
  ASSIGN_OR_RETURN(std::string json, ReadFile(path));
  google::protobuf::util::Status parse_status =
      google::protobuf::util::JsonStringToMessage(json, protobuf);
  if (!parse_status.ok()) {
    return absl::InvalidArgumentError(
        "Cannot parse configuration protobuf from JSON!");
  }
  return absl::OkStatus();
}

// Specialization of template to concrete types.
// Need this work around since the function is declared in the header file
// but defined here.
template absl::Status ReadProtobufFromJson<>(const std::string &,
                                             drivacy::types::Configuration *);

}  // namespace file
}  // namespace util
}  // namespace drivacy
