// Copyright 2020 multiparty.org

// This file contains the query phase of the protocol.

#ifndef DRIVACY_PROTOCOL_QUERY_H_
#define DRIVACY_PROTOCOL_QUERY_H_

#include "drivacy/types/messages.pb.h"

namespace drivacy {
namespace protocol {
namespace query {

types::Query ProcessQuery(const types::Query &query);

}  // namespace query
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_QUERY_H_
