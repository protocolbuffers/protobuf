#ifndef GOOGLE_PROTOBUF_UNKNOWN_FIELD_MAP_H__
#define GOOGLE_PROTOBUF_UNKNOWN_FIELD_MAP_H__

#include <atomic>
#include <string>

namespace google {
namespace protobuf {

class Arena;
class UnknownFieldSet;

namespace internal {

class InternalMetadata;

class UnknownFieldMap {
 public:
  UnknownFieldMap() = default;

  UnknownFieldMap(const UnknownFieldMap&) = delete;
  UnknownFieldMap& operator=(const UnknownFieldMap&) = delete;

  void Destroy();

  static UnknownFieldMap& GlobalUnknownFieldMap();

  // Defined in arena.h
  static UnknownFieldMap& ArenaUnknownFieldMap(Arena& arena);

  template <typename T>
  T* GetUnknownFieldSet(const InternalMetadata* metadata) = delete;

  template <>
  UnknownFieldSet* GetUnknownFieldSet<UnknownFieldSet>(
      const InternalMetadata* metadata);

  template <>
  std::string* GetUnknownFieldSet<std::string>(
      const InternalMetadata* metadata);

  template <typename T>
  T* MutableUnknownFieldSet(const InternalMetadata* metadata) = delete;

  template <>
  UnknownFieldSet* MutableUnknownFieldSet<UnknownFieldSet>(
      const InternalMetadata* metadata);

  template <>
  std::string* MutableUnknownFieldSet<std::string>(
      const InternalMetadata* metadata);

  // Swaps the unknown fields of m1 and m2, assuming both have unknown fields.
  void Swap(const InternalMetadata* m1, const InternalMetadata* m2);

  // Transfers the unknown fields from src to dst, assuming dst does not have
  // unknown fields and src does.
  void Transfer(const InternalMetadata* dst, const InternalMetadata* src);

  template <typename T>
  void DeleteUnknownFieldSet(const InternalMetadata* metadata) = delete;

  template <>
  void DeleteUnknownFieldSet<UnknownFieldSet>(const InternalMetadata* metadata);

  template <>
  void DeleteUnknownFieldSet<std::string>(const InternalMetadata* metadata);

 private:
  class Impl;

#if defined(__cpp_lib_atomic_ref)
  std::atomic_ref<Impl*> impl_atomic() const {
    return std::atomic_ref<Impl*>(impl_);
  }
#else
  std::atomic<Impl*>& impl_atomic() const { return impl_; }
#endif

  Impl& impl() const;
  Impl& impl_assuming_present() const;

#if defined(__cpp_lib_atomic_ref)
  mutable Impl* impl_ = nullptr;
#else
  mutable std::atomic<Impl*> impl_ = nullptr;
#endif
};

extern UnknownFieldMap kGlobalUnknownFieldMap;

inline UnknownFieldMap& UnknownFieldMap::GlobalUnknownFieldMap() {
  return kGlobalUnknownFieldMap;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_UNKNOWN_FIELD_MAP_H__
