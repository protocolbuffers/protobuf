#ifndef GOOGLE_PROTOBUF_STATIC_MESSAGE_FACTORY_H__
#define GOOGLE_PROTOBUF_STATIC_MESSAGE_FACTORY_H__

#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/message_lite.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

namespace internal {

class ByPrototype {
 public:
  explicit ByPrototype(const MessageLite* prototype) : prototype_(prototype) {}

  MessageLite* New(Arena* arena) const { return prototype_->New(arena); }

  const MessageLite& Default() const { return *prototype_; }

 private:
  const MessageLite* prototype_;
};

template <typename MessageType>
class ByTemplate {
 public:
  // In some cases, MessageType may be incomplete, and we don't need the default
  // instance (only `Get()` needs the default instance). Rather than pass the
  // default instance down just for it to not be used, we allow omitting the
  // default instance.
  explicit ByTemplate() : ByTemplate(nullptr) {}
  explicit ByTemplate(const MessageType* default_instance)
      : default_instance_(default_instance) {}

  MessageLite* New(Arena* arena) const {
    return reinterpret_cast<MessageLite*>(
        Arena::DefaultConstruct<MessageType>(arena));
  }

  const MessageLite& Default() const {
    ABSL_DCHECK(default_instance_ != nullptr);
    return *reinterpret_cast<const MessageLite*>(default_instance_);
  }

 private:
  const MessageType* default_instance_;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_STATIC_MESSAGE_FACTORY_H__
