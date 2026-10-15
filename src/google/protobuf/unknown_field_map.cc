#include "google/protobuf/unknown_field_map.h"

#include <atomic>
#include <string>
#include <utility>

#include "absl/base/optimization.h"
#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/debugging/leak_check.h"
#include "absl/log/absl_check.h"
#include "absl/synchronization/mutex.h"
#include "absl/types/optional.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/unknown_field_set.h"

namespace google {
namespace protobuf {
namespace internal {

UnknownFieldMap kGlobalUnknownFieldMap;

struct UseUnknownFieldSet {};
struct UseString {};

union UnknownFieldsEntry {
  UnknownFieldsEntry(UseUnknownFieldSet, Arena* arena)
      : unknown_field_set(google::protobuf::Arena::Create<UnknownFieldSet>(arena)) {}
  UnknownFieldsEntry(UseString, Arena* arena)
      : string(google::protobuf::Arena::Create<std::string>(arena)) {}

  std::string* string;
  UnknownFieldSet* unknown_field_set;
};

class UnknownFieldMap::Impl {
 public:
  absl::optional<UnknownFieldsEntry> GetUnknownFieldSet(
      const InternalMetadata* metadata) {
    absl::MutexLock lock(mutex_);
    auto it = map_.find(metadata);
    if (it == map_.end()) {
      return absl::nullopt;
    }
    return it->second;
  }

  template <typename... Args>
  UnknownFieldsEntry TryInsertUnknownFieldSet(const InternalMetadata* metadata,
                                              Args&&... args) {
    absl::MutexLock lock(mutex_);
    auto [it, inserted] =
        map_.try_emplace(metadata, std::forward<Args>(args)...);
    return it->second;
  }

  void Swap(const InternalMetadata* m1, const InternalMetadata* m2) {
    absl::MutexLock lock(mutex_);
    auto it1 = map_.find(m1);
    auto it2 = map_.find(m2);
    ABSL_DCHECK(it1 != map_.end());
    ABSL_DCHECK(it2 != map_.end());
    std::swap(it1->second, it2->second);
  }

  void Transfer(const InternalMetadata* dst, const InternalMetadata* src) {
    absl::MutexLock lock(mutex_);
    auto it = map_.find(src);
    ABSL_DCHECK(it != map_.end());
    UnknownFieldsEntry entry = it->second;
    map_.erase(it);
    map_.emplace(dst, entry);
  }

  [[nodiscard]] UnknownFieldsEntry RemoveEntry(
      const InternalMetadata* metadata) {
    absl::MutexLock lock(mutex_);
    auto it = map_.find(metadata);
    ABSL_DCHECK(it != map_.end());
    UnknownFieldsEntry entry = it->second;
    map_.erase(it);
    return entry;
  }

 private:
  absl::Mutex mutex_;
  absl::flat_hash_map<const InternalMetadata*, UnknownFieldsEntry> map_
      ABSL_GUARDED_BY(mutex_);
};

void UnknownFieldMap::Destroy() { delete impl_; }

template <>
UnknownFieldSet* UnknownFieldMap::GetUnknownFieldSet<UnknownFieldSet>(
    const InternalMetadata* metadata) {
  auto entry = impl().GetUnknownFieldSet(metadata);
  return entry.has_value() ? entry->unknown_field_set : nullptr;
}

template <>
std::string* UnknownFieldMap::GetUnknownFieldSet<std::string>(
    const InternalMetadata* metadata) {
  auto entry = impl().GetUnknownFieldSet(metadata);
  return entry.has_value() ? entry->string : nullptr;
}

template <>
UnknownFieldSet* UnknownFieldMap::MutableUnknownFieldSet<UnknownFieldSet>(
    const InternalMetadata* metadata) {
  UnknownFieldsEntry entry = impl().TryInsertUnknownFieldSet(
      metadata, UseUnknownFieldSet{}, metadata->arena());
  return entry.unknown_field_set;
}

template <>
std::string* UnknownFieldMap::MutableUnknownFieldSet<std::string>(
    const InternalMetadata* metadata) {
  UnknownFieldsEntry entry =
      impl().TryInsertUnknownFieldSet(metadata, UseString{}, metadata->arena());
  return entry.string;
}

void UnknownFieldMap::Swap(const InternalMetadata* m1,
                           const InternalMetadata* m2) {
  impl_assuming_present().Swap(m1, m2);
}

void UnknownFieldMap::Transfer(const InternalMetadata* dst,
                               const InternalMetadata* src) {
  impl_assuming_present().Transfer(dst, src);
}

template <>
void UnknownFieldMap::DeleteUnknownFieldSet<UnknownFieldSet>(
    const InternalMetadata* metadata) {
  UnknownFieldsEntry entry = impl_assuming_present().RemoveEntry(metadata);
  delete entry.unknown_field_set;
}

template <>
void UnknownFieldMap::DeleteUnknownFieldSet<std::string>(
    const InternalMetadata* metadata) {
  UnknownFieldsEntry entry = impl_assuming_present().RemoveEntry(metadata);
  delete entry.string;
}

inline UnknownFieldMap::Impl& UnknownFieldMap::impl() const {
  Impl* impl = impl_atomic().load(std::memory_order_acquire);
  if (ABSL_PREDICT_TRUE(impl != nullptr)) {
    return *impl;
  }

  Impl* new_impl = new Impl();
  Impl* old_impl = nullptr;
  if (!impl_atomic().compare_exchange_strong(old_impl, new_impl,
                                             std::memory_order_release,
                                             std::memory_order_acquire)) {
    delete new_impl;
    return *old_impl;
  }
  absl::IgnoreLeak(new_impl);
  return *new_impl;
}

inline UnknownFieldMap::Impl& UnknownFieldMap::impl_assuming_present() const {
  return *impl_atomic().load(std::memory_order_acquire);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
