// Copyright 2020 multiparty.org

// Helpers for file and file protobuf IO.

#include "drivacy/io/file.h"

#include <cerrno>
#include <fstream>
#include <streambuf>
#include <string>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
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

util::OutputStatus<proto::Table> ParseTable(const std::string &json) {
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

  // Fill in protobuf one row at a time.
  proto::Table table;
  for (rapidjson::SizeType i = 0; i < rows.Size(); i++) {
    const rapidjson::Value &row = rows[i];
    if (!row.IsObject() || !row.HasMember("key") || !row.HasMember("value")) {
      return absl::InvalidArgumentError(
          absl::StrCat("Parsing: encountered bad row ", i));
    }

    const rapidjson::Value &key_val = row["key"];
    const rapidjson::Value &value_val = row["value"];
    if (!key_val.IsUint64() || !value_val.IsUint64()) {
      return absl::InvalidArgumentError("Parsing: row has bad key or value");
    }

    proto::Row *proto_row = table.add_rows();
    proto_row->set_key(key_val.GetUint64());
    proto_row->set_value(value_val.GetUint64());
  }

  return table;
}

}  // namespace file
}  // namespace io
}  // namespace drivacy
