#include "google/protobuf/unknown_field_map.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/synchronization/notification.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/unknown_field_set.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

// Each test uses its own map so that tests don't interact with each other or
// with the global map.
class UnknownFieldMapTest : public testing::Test {
 protected:
  ~UnknownFieldMapTest() override { map_.Destroy(); }

  UnknownFieldMap map_;
};

TEST_F(UnknownFieldMapTest, GetOnEmptyMapReturnsNull) {
  InternalMetadata metadata(nullptr);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata), nullptr);
  EXPECT_EQ(map_.GetUnknownFieldSet<UnknownFieldSet>(&metadata), nullptr);
}

TEST_F(UnknownFieldMapTest, MutableStringCreatesEmptyEntry) {
  InternalMetadata metadata(nullptr);
  std::string* str = map_.MutableUnknownFieldSet<std::string>(&metadata);
  ASSERT_NE(str, nullptr);
  EXPECT_EQ(*str, "");
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata), str);

  map_.DeleteUnknownFieldSet<std::string>(&metadata);
}

TEST_F(UnknownFieldMapTest, MutableStringIsStableAcrossCalls) {
  InternalMetadata metadata(nullptr);
  std::string* str = map_.MutableUnknownFieldSet<std::string>(&metadata);
  str->append("unknown bytes");
  EXPECT_EQ(map_.MutableUnknownFieldSet<std::string>(&metadata), str);
  EXPECT_EQ(*map_.GetUnknownFieldSet<std::string>(&metadata), "unknown bytes");

  map_.DeleteUnknownFieldSet<std::string>(&metadata);
}

TEST_F(UnknownFieldMapTest, MutableUnknownFieldSetCreatesEmptyEntry) {
  InternalMetadata metadata(nullptr);
  UnknownFieldSet* ufs =
      map_.MutableUnknownFieldSet<UnknownFieldSet>(&metadata);
  ASSERT_NE(ufs, nullptr);
  EXPECT_TRUE(ufs->empty());
  EXPECT_EQ(map_.GetUnknownFieldSet<UnknownFieldSet>(&metadata), ufs);

  ufs->AddVarint(1, 42);
  EXPECT_EQ(map_.MutableUnknownFieldSet<UnknownFieldSet>(&metadata), ufs);
  EXPECT_EQ(map_.GetUnknownFieldSet<UnknownFieldSet>(&metadata)->field_count(),
            1);

  map_.DeleteUnknownFieldSet<UnknownFieldSet>(&metadata);
}

TEST_F(UnknownFieldMapTest, DeleteRemovesEntry) {
  InternalMetadata metadata(nullptr);
  (void)map_.MutableUnknownFieldSet<std::string>(&metadata);
  map_.DeleteUnknownFieldSet<std::string>(&metadata);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata), nullptr);

  // A new entry can be created for the same key after deletion.
  std::string* str2 = map_.MutableUnknownFieldSet<std::string>(&metadata);
  ASSERT_NE(str2, nullptr);
  EXPECT_EQ(*str2, "");
  map_.DeleteUnknownFieldSet<std::string>(&metadata);
}

TEST_F(UnknownFieldMapTest, EntriesAreKeyedByMetadataAddress) {
  InternalMetadata metadata1(nullptr);
  InternalMetadata metadata2(nullptr);
  std::string* str1 = map_.MutableUnknownFieldSet<std::string>(&metadata1);
  std::string* str2 = map_.MutableUnknownFieldSet<std::string>(&metadata2);
  EXPECT_NE(str1, str2);

  str1->assign("one");
  str2->assign("two");
  EXPECT_EQ(*map_.GetUnknownFieldSet<std::string>(&metadata1), "one");
  EXPECT_EQ(*map_.GetUnknownFieldSet<std::string>(&metadata2), "two");

  map_.DeleteUnknownFieldSet<std::string>(&metadata1);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata1), nullptr);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata2), str2);

  map_.DeleteUnknownFieldSet<std::string>(&metadata2);
}

TEST_F(UnknownFieldMapTest, ArenaStringIsAllocatedOnArena) {
  Arena arena;
  InternalMetadata metadata(&arena);
  const uint64_t space_used_before = arena.SpaceUsed();
  std::string* str = map_.MutableUnknownFieldSet<std::string>(&metadata);
  ASSERT_NE(str, nullptr);
  EXPECT_GT(arena.SpaceUsed(), space_used_before);
  // Make the string long enough to require a heap buffer, which the arena
  // must free via the registered destructor.
  str->assign(1024, 'x');
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata), str);
}

TEST_F(UnknownFieldMapTest, ArenaUnknownFieldSetIsAllocatedOnArena) {
  Arena arena;
  InternalMetadata metadata(&arena);
  const uint64_t space_used_before = arena.SpaceUsed();
  UnknownFieldSet* ufs =
      map_.MutableUnknownFieldSet<UnknownFieldSet>(&metadata);
  ASSERT_NE(ufs, nullptr);
  EXPECT_GT(arena.SpaceUsed(), space_used_before);
  ufs->AddVarint(1, 42);
  EXPECT_EQ(map_.GetUnknownFieldSet<UnknownFieldSet>(&metadata), ufs);
}

TEST_F(UnknownFieldMapTest, SwapExchangesStringEntries) {
  InternalMetadata metadata1(nullptr);
  InternalMetadata metadata2(nullptr);
  std::string* str1 = map_.MutableUnknownFieldSet<std::string>(&metadata1);
  std::string* str2 = map_.MutableUnknownFieldSet<std::string>(&metadata2);
  str1->assign("one");
  str2->assign("two");

  map_.Swap(&metadata1, &metadata2);

  // The underlying objects are exchanged, not their contents.
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata1), str2);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata2), str1);
  EXPECT_EQ(*map_.GetUnknownFieldSet<std::string>(&metadata1), "two");
  EXPECT_EQ(*map_.GetUnknownFieldSet<std::string>(&metadata2), "one");

  // Swapping back restores the original mapping.
  map_.Swap(&metadata2, &metadata1);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata1), str1);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata2), str2);

  map_.DeleteUnknownFieldSet<std::string>(&metadata1);
  map_.DeleteUnknownFieldSet<std::string>(&metadata2);
}

TEST_F(UnknownFieldMapTest, SwapExchangesUnknownFieldSetEntries) {
  InternalMetadata metadata1(nullptr);
  InternalMetadata metadata2(nullptr);
  UnknownFieldSet* ufs1 =
      map_.MutableUnknownFieldSet<UnknownFieldSet>(&metadata1);
  UnknownFieldSet* ufs2 =
      map_.MutableUnknownFieldSet<UnknownFieldSet>(&metadata2);
  ufs1->AddVarint(1, 100);
  ufs2->AddVarint(2, 200);
  ufs2->AddVarint(3, 300);

  map_.Swap(&metadata1, &metadata2);

  UnknownFieldSet* got1 = map_.GetUnknownFieldSet<UnknownFieldSet>(&metadata1);
  UnknownFieldSet* got2 = map_.GetUnknownFieldSet<UnknownFieldSet>(&metadata2);
  EXPECT_EQ(got1, ufs2);
  EXPECT_EQ(got2, ufs1);
  ASSERT_EQ(got1->field_count(), 2);
  EXPECT_EQ(got1->field(0).number(), 2);
  EXPECT_EQ(got1->field(1).number(), 3);
  ASSERT_EQ(got2->field_count(), 1);
  EXPECT_EQ(got2->field(0).number(), 1);
  EXPECT_EQ(got2->field(0).varint(), 100);

  map_.DeleteUnknownFieldSet<UnknownFieldSet>(&metadata1);
  map_.DeleteUnknownFieldSet<UnknownFieldSet>(&metadata2);
}

TEST_F(UnknownFieldMapTest, SwapWithSelfIsNoOp) {
  InternalMetadata metadata(nullptr);
  std::string* str = map_.MutableUnknownFieldSet<std::string>(&metadata);
  str->assign("self");

  map_.Swap(&metadata, &metadata);

  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata), str);
  EXPECT_EQ(*str, "self");
  map_.DeleteUnknownFieldSet<std::string>(&metadata);
}

TEST_F(UnknownFieldMapTest, SwapDoesNotAffectOtherEntries) {
  InternalMetadata metadata1(nullptr);
  InternalMetadata metadata2(nullptr);
  InternalMetadata bystander(nullptr);
  std::string* str1 = map_.MutableUnknownFieldSet<std::string>(&metadata1);
  std::string* str2 = map_.MutableUnknownFieldSet<std::string>(&metadata2);
  std::string* str3 = map_.MutableUnknownFieldSet<std::string>(&bystander);

  map_.Swap(&metadata1, &metadata2);

  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata1), str2);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata2), str1);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&bystander), str3);

  map_.DeleteUnknownFieldSet<std::string>(&metadata1);
  map_.DeleteUnknownFieldSet<std::string>(&metadata2);
  map_.DeleteUnknownFieldSet<std::string>(&bystander);
}

TEST_F(UnknownFieldMapTest, SwapArenaEntriesOnSameArena) {
  Arena arena;
  InternalMetadata metadata1(&arena);
  InternalMetadata metadata2(&arena);
  std::string* str1 = map_.MutableUnknownFieldSet<std::string>(&metadata1);
  std::string* str2 = map_.MutableUnknownFieldSet<std::string>(&metadata2);
  str1->assign(1024, 'a');
  str2->assign(1024, 'b');

  map_.Swap(&metadata1, &metadata2);

  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata1), str2);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata2), str1);
  // The arena owns both strings and frees them on destruction.
}

TEST_F(UnknownFieldMapTest, TransferMovesStringEntry) {
  InternalMetadata src(nullptr);
  InternalMetadata dst(nullptr);
  std::string* str = map_.MutableUnknownFieldSet<std::string>(&src);
  str->assign("payload");

  map_.Transfer(&dst, &src);

  // The same object is now owned by `dst`, and `src` has no entry.
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&src), nullptr);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&dst), str);
  EXPECT_EQ(*str, "payload");

  map_.DeleteUnknownFieldSet<std::string>(&dst);
}

TEST_F(UnknownFieldMapTest, TransferMovesUnknownFieldSetEntry) {
  InternalMetadata src(nullptr);
  InternalMetadata dst(nullptr);
  UnknownFieldSet* ufs = map_.MutableUnknownFieldSet<UnknownFieldSet>(&src);
  ufs->AddVarint(7, 77);

  map_.Transfer(&dst, &src);

  EXPECT_EQ(map_.GetUnknownFieldSet<UnknownFieldSet>(&src), nullptr);
  UnknownFieldSet* got = map_.GetUnknownFieldSet<UnknownFieldSet>(&dst);
  EXPECT_EQ(got, ufs);
  ASSERT_EQ(got->field_count(), 1);
  EXPECT_EQ(got->field(0).number(), 7);
  EXPECT_EQ(got->field(0).varint(), 77);

  map_.DeleteUnknownFieldSet<UnknownFieldSet>(&dst);
}

TEST_F(UnknownFieldMapTest, TransferThenMutableOnSrcCreatesFreshEntry) {
  InternalMetadata src(nullptr);
  InternalMetadata dst(nullptr);
  std::string* str = map_.MutableUnknownFieldSet<std::string>(&src);
  str->assign("moved");

  map_.Transfer(&dst, &src);

  std::string* fresh = map_.MutableUnknownFieldSet<std::string>(&src);
  ASSERT_NE(fresh, nullptr);
  EXPECT_NE(fresh, str);
  EXPECT_EQ(*fresh, "");
  EXPECT_EQ(*map_.GetUnknownFieldSet<std::string>(&dst), "moved");

  map_.DeleteUnknownFieldSet<std::string>(&src);
  map_.DeleteUnknownFieldSet<std::string>(&dst);
}

TEST_F(UnknownFieldMapTest, TransferRoundTrip) {
  InternalMetadata a(nullptr);
  InternalMetadata b(nullptr);
  std::string* str = map_.MutableUnknownFieldSet<std::string>(&a);

  map_.Transfer(&b, &a);
  map_.Transfer(&a, &b);

  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&a), str);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&b), nullptr);

  map_.DeleteUnknownFieldSet<std::string>(&a);
}

TEST_F(UnknownFieldMapTest, TransferDoesNotAffectOtherEntries) {
  InternalMetadata src(nullptr);
  InternalMetadata dst(nullptr);
  InternalMetadata bystander(nullptr);
  std::string* str = map_.MutableUnknownFieldSet<std::string>(&src);
  std::string* other = map_.MutableUnknownFieldSet<std::string>(&bystander);

  map_.Transfer(&dst, &src);

  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&dst), str);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&bystander), other);

  map_.DeleteUnknownFieldSet<std::string>(&dst);
  map_.DeleteUnknownFieldSet<std::string>(&bystander);
}

TEST_F(UnknownFieldMapTest, TransferArenaEntryOnSameArena) {
  Arena arena;
  InternalMetadata src(&arena);
  InternalMetadata dst(&arena);
  std::string* str = map_.MutableUnknownFieldSet<std::string>(&src);
  str->assign(1024, 'x');

  map_.Transfer(&dst, &src);

  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&src), nullptr);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&dst), str);
  // The arena owns the string and frees it on destruction.
}

TEST(UnknownFieldMapGlobalTest, GlobalMapIsSingleton) {
  EXPECT_EQ(&UnknownFieldMap::GlobalUnknownFieldMap(),
            &UnknownFieldMap::GlobalUnknownFieldMap());
}

// Races many threads to be the first to touch a fresh map, which lazily
// installs its `Impl` via compare-and-swap. If installation were not
// thread-safe, threads could end up inserting into different `Impl`s and some
// entries would be lost (or TSan/ASan would report a race/leak).
TEST(UnknownFieldMapConcurrencyTest, ConcurrentImplInstallation) {
  constexpr size_t kThreads = 16;
  constexpr size_t kIterations = 50;

  for (size_t iter = 0; iter < kIterations; ++iter) {
    UnknownFieldMap map;
    std::vector<InternalMetadata> metadatas(kThreads,
                                            InternalMetadata(nullptr));
    std::vector<std::string*> strings(kThreads, nullptr);
    absl::Notification start;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (size_t i = 0; i < kThreads; ++i) {
      threads.emplace_back([&, i] {
        start.WaitForNotification();
        // Alternate between the read and write paths, both of which may be
        // the first call to install the `Impl`.
        if (i % 2 == 0) {
          EXPECT_EQ(map.GetUnknownFieldSet<std::string>(&metadatas[i]),
                    nullptr);
        }
        strings[i] = map.MutableUnknownFieldSet<std::string>(&metadatas[i]);
        strings[i]->assign(absl::StrCat("thread ", i));
      });
    }
    start.Notify();
    for (std::thread& t : threads) t.join();

    // Every thread's entry must be visible through the single installed impl.
    for (size_t i = 0; i < kThreads; ++i) {
      ASSERT_NE(strings[i], nullptr);
      std::string* found = map.GetUnknownFieldSet<std::string>(&metadatas[i]);
      ASSERT_EQ(found, strings[i]) << "iteration " << iter << ", thread " << i;
      EXPECT_EQ(*found, absl::StrCat("thread ", i));
    }

    for (size_t i = 0; i < kThreads; ++i) {
      map.DeleteUnknownFieldSet<std::string>(&metadatas[i]);
    }
    map.Destroy();
  }
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
