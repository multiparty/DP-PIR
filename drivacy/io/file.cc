// Copyright 2020 multiparty.org

// Helpers for file and file protobuf IO.

#include "drivacy/io/file.h"

#include <cerrno>
#include <fstream>
#include <streambuf>
#include <string>

#include "absl/strings/str_cat.h"
#include "drivacy/types/config.pb.h"
#include "google/protobuf/util/json_util.h"
#include "rapidjson/document.h"

namespace drivacy {
namespace io {
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

util::OutputStatus<types::Table> ParseTable(const std::string &json) {
  // Parse string.
  rapidjson::Document document;
  if (document.Parse(json.c_str()).HasParseError()) {
    return absl::InvalidArgumentError("Parsing: badly formatted JSON");
  }

  // Make sure format is ok!
  if (!document.IsObject() || !document.HasMember("table")) {
    return absl::InvalidArgumentError("Parsing: <json>#table is invalid");
  }
  const rapidjson::Value &rows = document["table"];
  if (!rows.IsArray()) {
    return absl::InvalidArgumentError("Parsing: <json>#table is not an array");
  }

  // Fill in map one row at a time.
  types::Table table;
  for (rapidjson::SizeType i = 0; i < rows.Size(); i++) {
    const rapidjson::Value &row = rows[i];
    if (!row.IsObject() || !row.HasMember("key") || !row.HasMember("value")) {
      return absl::InvalidArgumentError(
          absl::StrCat("Parsing: encountered bad row ", i));
    }

    const rapidjson::Value &key = row["key"];
    const rapidjson::Value &value = row["value"];
    if (!key.IsUint64() || !value.IsUint64()) {
      return absl::InvalidArgumentError("Parsing: row has bad key or value");
    }

    table[key.GetUint64()] = value.GetUint64();
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
}  // namespace io
}  // namespace drivacy
