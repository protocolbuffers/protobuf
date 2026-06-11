#include "upb/mini_table/minitable_registry.h"

#include <stdint.h>

#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "upb/mini_table/minitable_registry_test1.upb_minitable.h"
#include "upb/mini_table/minitable_registry_test2.upb_minitable.h"
#include "upb/mini_table/minitable_registry_test3.upb_minitable.h"
#include "util/hash/farmhash_fingerprint.h"

namespace upb {
namespace mini_table {
namespace {

TEST(MinitableRegistryTest, Lookup) {
  // Compute hashes of full names
  uint64_t hash1 =
      farmhash::Fingerprint64(absl::string_view("upb.mini_table.TestMessage1"));
  uint64_t hash2 =
      farmhash::Fingerprint64(absl::string_view("upb.mini_table.TestMessage2"));
  uint64_t hash3 =
      farmhash::Fingerprint64(absl::string_view("upb.mini_table.TestMessage3"));

  // Lookup
  const upb_MiniTable* mt1 = upb_MinitableRegistry_Lookup(hash1);
  const upb_MiniTable* mt2 = upb_MinitableRegistry_Lookup(hash2);
  const upb_MiniTable* mt3 = upb_MinitableRegistry_Lookup(hash3);

  // Verify
  ASSERT_NE(mt1, nullptr);
  ASSERT_NE(mt2, nullptr);
  ASSERT_NE(mt3, nullptr);

  // The generated symbols should match what we found
  EXPECT_EQ(mt1, &upb__mini_0table__TestMessage1_msg_init);
  EXPECT_EQ(mt2, &upb__mini_0table__TestMessage2_msg_init);
  EXPECT_EQ(mt3, &upb__mini_0table__TestMessage3_msg_init);

  // Lookup non-existent
  uint64_t hash_bad =
      farmhash::Fingerprint64(absl::string_view("upb.mini_table.NonExistent"));
  EXPECT_EQ(upb_MinitableRegistry_Lookup(hash_bad), nullptr);
}

}  // namespace
}  // namespace mini_table
}  // namespace upb
