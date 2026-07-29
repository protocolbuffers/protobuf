
#include <gtest/gtest.h>
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/generated_registry.h"
#include "upb/mini_table/generated_registry_empty_test.upb_minitable.h"
#include "upb/mini_table/message.h"

namespace {

// Tests that we can instantiate the registry even if no extensions are linked.
// This ensures that the sentinel object in the linker array properly ensures
// that the encapsulation symbols are defined (eg. __start_linkarr_exts,
// __stop_linkarr_exts) even when no extensions were linked.
TEST(GeneratedRegistryEmptyTest, Instantiate) {
  // Strongly reference the generated MiniTable to ensure that the TU for
  // generated_registry_empty_test.upb_minitable.c is pulled in.
  const upb_MiniTable* volatile table =
      &upb_0test_0empty_0registry__EmptyRegistryTestMessage_msg_init;
  (void)table;

  // Test that the registry can be loaded, even if no extensions are linked.
  // If we did not have a sentinel in the linker array, we would get a linker
  // error here like:
  //
  // ld: error: undefined symbol: __start_linkarr_upb_AllExts
  // >>> referenced by generated_registry_empty_test.upb_minitable.c
  // >>>
  // generated_registry_empty_test.upb_minitable.pic.o:(upb_GeneratedRegistry_Constructor_dont_copy_me__upb_internal_use_only.entry)
  // >>> the encapsulation symbol needs to be retained under --gc-sections
  // properly; consider -z nostart-stop-gc (see
  // https://lld.llvm.org/ELF/start-stop-gc)
  //
  // ld: error: undefined symbol: __stop_linkarr_upb_AllExts
  // >>> referenced by generated_registry_empty_test.upb_minitable.c
  // >>>
  // generated_registry_empty_test.upb_minitable.pic.o:(upb_GeneratedRegistry_Constructor_dont_copy_me__upb_internal_use_only.entry)
  const upb_GeneratedRegistryRef* ref = upb_GeneratedRegistry_Load();
  EXPECT_NE(ref, nullptr);
  const upb_ExtensionRegistry* reg = upb_GeneratedRegistry_Get(ref);
  EXPECT_NE(reg, nullptr);

  upb_GeneratedRegistry_Release(ref);
}

}  // namespace
