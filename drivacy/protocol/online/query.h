// Copyright 2020 multiparty.org

// This file contains the query phase of the protocol.

#ifndef DRIVACY_PROTOCOL_ONLINE_QUERY_H_
#define DRIVACY_PROTOCOL_ONLINE_QUERY_H_

#include <cstdint>
#include <utility>

#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {
namespace online {
namespace query {

std::pair<types::Query, types::QueryState> ProcessQuery(
    const types::Query &query, const types::CommonReferenceMap &common_map);

}  // namespace query
}  // namespace online
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_ONLINE_QUERY_H_
