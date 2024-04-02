#ifndef GOOGLE_PROTOBUF_GTEST_VISIBILITY_H__
#define GOOGLE_PROTOBUF_GTEST_VISIBILITY_H__

namespace testing {
namespace internal {
class ProtobufPrinter;
}  // namespace internal
}  // namespace testing

namespace google {
namespace protobuf {
namespace internal {

// Empty class to use as a mandatory 'internal token' for functions that have to
// be public, but that are for gtest use only.
class GTestVisibility {
 private:
  // Note: we don't use `InternalVisibility() = default` here, but default the
  // ctor outside of the class to force a private ctor instance.
  explicit constexpr GTestVisibility();

  friend class ::testing::internal::ProtobufPrinter;
};

inline constexpr GTestVisibility::GTestVisibility() = default;

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_GTEST_VISIBILITY_H__
