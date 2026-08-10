#include "google/protobuf/thread.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/base/config.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

using testing::AllOf;
using testing::Gt;
using testing::Lt;
using testing::Optional;

TEST(ThreadTest, TestStackInfo) {
#if !defined(_POSIX_THREADS) || defined(__wasm__)
  GTEST_SKIP() << "Not supported";
#endif
#if defined(ABSL_HAVE_ADDRESS_SANITIZER)
  GTEST_SKIP() << "ASan changes how the stack works.";
#endif  // ABSL_HAVE_ADDRESS_SANITIZER

  void* something_in_the_stack = &something_in_the_stack;
  auto stack_info = GetCurrentStackInfo();

  ASSERT_TRUE(stack_info.has_value());

  EXPECT_LT(stack_info->base_ptr, something_in_the_stack);
  EXPECT_LT(something_in_the_stack,
            static_cast<void*>(static_cast<char*>(stack_info->base_ptr) +
                               stack_info->size));
}

TEST(ThreadTest, GetEstimatedThreadStackRemaining) {
#if !defined(_POSIX_THREADS) || defined(__wasm__)
  GTEST_SKIP() << "Not supported";
#endif
#if defined(ABSL_HAVE_ADDRESS_SANITIZER)
  GTEST_SKIP() << "ASan changes how the stack works.";
#endif  // ABSL_HAVE_ADDRESS_SANITIZER

  auto estimated_stack = GetEstimatedThreadStackRemaining();
  // Just check that it is within some reasonable bounds.
  EXPECT_THAT(estimated_stack, Optional(AllOf(Gt(0), Lt(1'000'000'000))));
}

TEST(ThreadTest, RunSyncInSeparateThread) {
  if (!CanSpawnNewThreads()) {
    GTEST_SKIP() << "Platom can't spawn threads.";
  }
  static thread_local bool ran_in_other_thread;

  ran_in_other_thread = false;
  bool* main_ptr = &ran_in_other_thread;
  RunSyncInSeparateThread([&] {
    EXPECT_NE(main_ptr, &ran_in_other_thread);
    *main_ptr = true;
  });
  EXPECT_TRUE(ran_in_other_thread);
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
