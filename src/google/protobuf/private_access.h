#ifndef GOOGLE_PROTOBUF_PRIVATE_ACCESS_H__
#define GOOGLE_PROTOBUF_PRIVATE_ACCESS_H__

#include <type_traits>

namespace google {
namespace protobuf {

class MessageLite;

namespace internal {

struct ClassData;
class ExtensionSet;

// The struct PrivateAccess is used to provide access to private members of
// message classes without making them public. This is useful for highly
// optimized code paths that need to access internals.
struct PrivateAccess {
  template <typename T, int number>
  static constexpr bool IsLazyField() {
    constexpr auto l =
        [](auto& msg) -> decltype(msg._lazy_internal_mutable(
                          std::integral_constant<int, number>{})) {};
    return std::is_invocable_v<decltype(l), T&>;
  }

  template <int number, typename T>
  static auto& MutableLazy(T& msg) {
    return msg._lazy_internal_mutable(std::integral_constant<int, number>{});
  }

  template <typename MessageT>
  static auto& GetInternalMetadata(MessageT& msg) {
    return msg._internal_metadata_;
  }

  template <typename T>
  static auto& GetExtensionSet(T& msg) {
    return msg._impl_._extensions_;
  }

  static ExtensionSet* GetExtensionSet(MessageLite& msg);
  static const ExtensionSet* GetExtensionSet(const MessageLite& msg);

  template <typename T>
  static void TrackerOnGetMetadata() {
    T::Impl_::TrackerOnGetMetadata();
  }

  template <typename T>
  static constexpr auto GenerateClassData() {
    return T::_Internal::GenerateClassData();
  }

  template <typename T>
  static constexpr auto GenerateParseTable(
      const ::google::protobuf::internal::ClassData* class_data) {
    return T::_Internal::GenerateParseTable(class_data);
  }

  template <typename T>
  static constexpr decltype(auto) FullMessageName() {
    return T::FullMessageName();
  }

  template <typename T>
  using ImplTForTesting = typename T::Impl_;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PRIVATE_ACCESS_H__
