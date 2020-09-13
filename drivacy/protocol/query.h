// Copyright 2020 multiparty.org

// This file contains the query phase of the protocol.

#ifndef DRIVACY_PROTOCOL_QUERY_H_
#define DRIVACY_PROTOCOL_QUERY_H_

#include <cstdint>

#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {
namespace query {

types::OutgoingQuery ProcessQuery(uint32_t party_id,
                                  const types::IncomingQuery &query,
                                  const types::Configuration &config);

}  // namespace query
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_QUERY_H_
