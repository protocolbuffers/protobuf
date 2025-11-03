// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_table/generated_registry.h"

#include <array>
#include <thread>  // NOLINT
#include <vector>

#include "google/protobuf/descriptor.upb_minitable.h"
#include <gtest/gtest.h>
#include "absl/synchronization/barrier.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/test/custom_options.upb_minitable.h"
#include "upb/test/editions_test.upb_minitable.h"
#include "upb/test/test_multiple_files.upb_minitable.h"
#include "upb/test/test_multiple_files2.upb_minitable.h"

namespace {

class GeneratedRegistryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ref_ = upb_GeneratedRegistry_Load();
    ASSERT_NE(ref_, nullptr);
    reg_ = upb_GeneratedRegistry_Get(ref_);
    ASSERT_NE(reg_, nullptr);
  }

  void TearDown() override { upb_GeneratedRegistry_Release(ref_); }

  const upb_GeneratedRegistryRef* ref_ = nullptr;
  const upb_ExtensionRegistry* reg_ = nullptr;
};

TEST_F(GeneratedRegistryTest, LocalExtensionExists) {
  // Test an arbitrary extension defined in the same .proto file that we expect
  // to be linked in.

  // Force linkage and sanity check extension numbers.
  ASSERT_EQ(upb_MiniTableExtension_Number(&upb_test_2023_ext_ext), 100);

  EXPECT_NE(upb_ExtensionRegistry_Lookup(
                reg_, &upb__test_02023__EditionsMessage_msg_init, 100),
            nullptr);
}

TEST_F(GeneratedRegistryTest, ForeignExtensionExists) {
  // Test an arbitrary extension defined by an import that we expect to be
  // linked in.

  // Force linkage and sanity check extension numbers.
  ASSERT_EQ(upb_MiniTableExtension_Number(&upb_message_opt_ext), 7739036);

  EXPECT_NE(upb_ExtensionRegistry_Lookup(reg_, &google__protobuf__MessageOptions_msg_init,
                                         7739036),
            nullptr);
}

TEST_F(GeneratedRegistryTest, UnlinkedExtensionDoesNotExist) {
  // Test an arbitrary extension defined by an option import that we do not
  // expect to be linked in.

  // Corresponds to upb.message_opt_unlinked in custom_options_unlinked.proto.
  EXPECT_EQ(upb_ExtensionRegistry_Lookup(reg_, &google__protobuf__MessageOptions_msg_init,
                                         7739037),
            nullptr);
}

TEST_F(GeneratedRegistryTest, MultipleFilesExtensionExists) {
  // Test that multiple .proto files from the same proto_library can be linked
  // in and registered.

  // Force linkage and sanity check extension numbers.
  ASSERT_EQ(upb_MiniTableExtension_Number(&upb_multiple_files_ext1_ext), 100);
  ASSERT_EQ(upb_MiniTableExtension_Number(&upb_multiple_files_ext2_ext), 100);

  EXPECT_NE(upb_ExtensionRegistry_Lookup(
                reg_, &upb__MultipleFilesMessage1_msg_init, 100),
            nullptr);
  EXPECT_NE(upb_ExtensionRegistry_Lookup(
                reg_, &upb__MultipleFilesMessage2_msg_init, 100),
            nullptr);
}

TEST_F(GeneratedRegistryTest, ReleaseOnError) {
  upb_GeneratedRegistry_Release(nullptr);
}

TEST_F(GeneratedRegistryTest, GetOnError) {
  EXPECT_EQ(upb_GeneratedRegistry_Get(nullptr), nullptr);
}

// On 32-bit systems, the stack size is smaller, so we can't run too many
// concurrent threads without overflowing the stack.
constexpr int kRaceIterations = sizeof(void*) < 8 ? 100 : 2000;

TEST(GeneratedRegistryRaceTest, Load) {
  absl::Barrier barrier(kRaceIterations);
  std::vector<std::thread> threads;
  std::array<const upb_GeneratedRegistryRef*, kRaceIterations> refs;
  for (int i = 0; i < kRaceIterations; ++i) {
    threads.push_back(std::thread([&, i]() {
      barrier.Block();
      const upb_GeneratedRegistryRef* ref = upb_GeneratedRegistry_Load();
      EXPECT_NE(upb_GeneratedRegistry_Get(ref), nullptr);
      refs[i] = ref;
    }));
  }

  for (auto& t : threads) t.join();

  for (int i = 0; i < kRaceIterations; ++i) {
    EXPECT_NE(refs[i], nullptr);
    upb_GeneratedRegistry_Release(refs[i]);
  }
}

TEST(GeneratedRegistryRaceTest, Release) {
  absl::Barrier barrier(kRaceIterations);
  std::vector<std::thread> threads;
  std::array<const upb_GeneratedRegistryRef*, kRaceIterations> refs;

  for (int i = 0; i < kRaceIterations; ++i) {
    refs[i] = upb_GeneratedRegistry_Load();
    ASSERT_NE(refs[i], nullptr);
  }

  for (int i = 0; i < kRaceIterations; ++i) {
    threads.push_back(std::thread([&, i]() {
      EXPECT_NE(upb_GeneratedRegistry_Get(refs[i]), nullptr);
      barrier.Block();
      upb_GeneratedRegistry_Release(refs[i]);
    }));
  }

  for (auto& t : threads) t.join();
}

TEST(GeneratedRegistryRaceTest, LoadRelease) {
  absl::Barrier barrier(kRaceIterations);
  std::vector<std::thread> threads;

  for (int i = 0; i < kRaceIterations; ++i) {
    threads.push_back(std::thread([&]() {
      barrier.Block();
      const upb_GeneratedRegistryRef* ref = upb_GeneratedRegistry_Load();
      EXPECT_NE(upb_GeneratedRegistry_Get(ref), nullptr);
      upb_GeneratedRegistry_Release(ref);
    }));
  }

  for (auto& t : threads) t.join();
}

TEST(GeneratedRegistryRaceTest, ReleaseLastAndLoadMultiple) {
  for (int i = 0; i < kRaceIterations; ++i) {
    // Load one reference. The registry is now alive with ref_count == 1.
    const upb_GeneratedRegistryRef* ref = upb_GeneratedRegistry_Load();
    ASSERT_NE(ref, nullptr);

    constexpr int kNumLoaders = 4;
    absl::Barrier barrier(kNumLoaders + 1);
    std::vector<std::thread> threads;

    threads.push_back(std::thread([&]() {
      barrier.Block();
      // This release should bring the ref_count to 0 and trigger cleanup.
      upb_GeneratedRegistry_Release(ref);
    }));

    for (int j = 0; j < kNumLoaders; ++j) {
      threads.push_back(std::thread([&]() {
        barrier.Block();
        const upb_GeneratedRegistryRef* ref2 = upb_GeneratedRegistry_Load();
        EXPECT_NE(upb_GeneratedRegistry_Get(ref2), nullptr);
        upb_GeneratedRegistry_Release(ref2);
      }));
    }

    for (auto& t : threads) t.join();
  }
}

}  // namespace
