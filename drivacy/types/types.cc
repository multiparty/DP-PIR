// Copyright 2020 multiparty.org

// C++ types used in the protocol.

#include "drivacy/types/types.h"

namespace drivacy {
namespace types {

const Tag &OnionMessage::tag() const {
  return *reinterpret_cast<Tag *>(this->buffer_.get());
}

const CommonReference &OnionMessage::common_reference() const {
  return *reinterpret_cast<CommonReference *>(this->buffer_.get() + sizeof(Tag));
}

CipherText OnionMessage::onion_cipher() const {
  return this->buffer_.get() + sizeof(Message);
}

}  // namespace types
}  // namespace drivacy
