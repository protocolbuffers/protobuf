#include "google/protobuf/unknown_field_map.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/synchronization/notification.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/unittest.pb.h"
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

TEST_F(UnknownFieldMapTest, ResetOnUnusedMapIsNoOp) {
  // `impl_` has not been installed yet; Reset must not install or touch it.
  map_.Reset();

  InternalMetadata metadata(nullptr);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata), nullptr);
}

TEST_F(UnknownFieldMapTest, ResetRemovesAllEntries) {
  Arena arena;
  InternalMetadata metadata1(&arena);
  InternalMetadata metadata2(&arena);
  ASSERT_NE(map_.MutableUnknownFieldSet<std::string>(&metadata1), nullptr);
  ASSERT_NE(map_.MutableUnknownFieldSet<UnknownFieldSet>(&metadata2), nullptr);

  map_.Reset();

  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata1), nullptr);
  EXPECT_EQ(map_.GetUnknownFieldSet<UnknownFieldSet>(&metadata2), nullptr);
  // The arena still owns (and will free) the objects.
}

TEST_F(UnknownFieldMapTest, ResetDoesNotFreeEntries) {
  // Reset only drops the map entries; it does not destroy the pointed-to
  // objects, since for arena maps the arena owns them.
  InternalMetadata metadata(nullptr);
  std::string* str = map_.MutableUnknownFieldSet<std::string>(&metadata);
  str->assign(1024, 'x');

  map_.Reset();

  // Still valid (ASan would flag a use-after-free here otherwise).
  EXPECT_EQ(*str, std::string(1024, 'x'));
  delete str;
}

TEST_F(UnknownFieldMapTest, MapIsUsableAfterReset) {
  Arena arena;
  InternalMetadata metadata(&arena);
  std::string* str = map_.MutableUnknownFieldSet<std::string>(&metadata);
  str->assign("before reset");

  map_.Reset();

  // A new, empty entry is created for the same key rather than returning the
  // stale one.
  std::string* fresh = map_.MutableUnknownFieldSet<std::string>(&metadata);
  ASSERT_NE(fresh, nullptr);
  EXPECT_NE(fresh, str);
  EXPECT_EQ(*fresh, "");
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata), fresh);
}

TEST_F(UnknownFieldMapTest, ResetTwice) {
  Arena arena;
  InternalMetadata metadata(&arena);
  ASSERT_NE(map_.MutableUnknownFieldSet<std::string>(&metadata), nullptr);

  map_.Reset();
  map_.Reset();

  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&metadata), nullptr);
}

TEST_F(UnknownFieldMapTest, ResetThenSwapAndTransfer) {
  Arena arena;
  InternalMetadata a(&arena);
  InternalMetadata b(&arena);
  InternalMetadata c(&arena);
  (void)map_.MutableUnknownFieldSet<std::string>(&a);
  map_.Reset();

  // Swap and Transfer work on entries created after a Reset.
  std::string* str_a = map_.MutableUnknownFieldSet<std::string>(&a);
  std::string* str_b = map_.MutableUnknownFieldSet<std::string>(&b);
  map_.Swap(&a, &b);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&a), str_b);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&b), str_a);

  map_.Transfer(&c, &a);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&a), nullptr);
  EXPECT_EQ(map_.GetUnknownFieldSet<std::string>(&c), str_b);
}

// `Arena::Reset` must reset the arena's map, otherwise a key whose memory is
// reused after the reset would map to a freed object.
TEST(UnknownFieldMapArenaTest, ArenaResetClearsArenaMap) {
  Arena arena;
  UnknownFieldMap& map = UnknownFieldMap::ArenaUnknownFieldMap(arena);
  InternalMetadata metadata(&arena);
  std::string* str = map.MutableUnknownFieldSet<std::string>(&metadata);
  str->assign(1024, 'x');  // Force a heap buffer the arena must destroy.

  arena.Reset();

  EXPECT_EQ(map.GetUnknownFieldSet<std::string>(&metadata), nullptr);
  std::string* fresh = map.MutableUnknownFieldSet<std::string>(&metadata);
  ASSERT_NE(fresh, nullptr);
  EXPECT_EQ(*fresh, "");
}

// End-to-end: unknown fields of a message allocated on an arena must not leak
// into a message allocated after `Arena::Reset`, even if it reuses the address.
TEST(UnknownFieldMapArenaTest, ArenaResetWithMessages) {
  Arena arena;
  auto* msg = Arena::Create<proto2_unittest::TestEmptyMessage>(&arena);
  // Field 1, varint 1.
  ASSERT_TRUE(msg->ParseFromString(std::string("\x08\x01", 2)));
  ASSERT_EQ(msg->unknown_fields().field_count(), 1);

  arena.Reset();

  auto* msg2 = Arena::Create<proto2_unittest::TestEmptyMessage>(&arena);
  EXPECT_TRUE(msg2->unknown_fields().empty());
  msg2->mutable_unknown_fields()->AddVarint(2, 2);
  ASSERT_EQ(msg2->unknown_fields().field_count(), 1);
  EXPECT_EQ(msg2->unknown_fields().field(0).number(), 2);
}

// ---------------------------------------------------------------------------
// InternalMetadata const read path (`unknown_fields()`): it must be a pure
// lookup that neither writes to the metadata nor inserts into the map.
// ---------------------------------------------------------------------------

// Returns the raw bits of `metadata` so tests can verify it is not modified.
intptr_t RawBits(const InternalMetadata& metadata) {
  static_assert(sizeof(InternalMetadata) == sizeof(intptr_t));
  intptr_t bits;
  std::memcpy(&bits, &metadata, sizeof(bits));
  return bits;
}

TEST(InternalMetadataConstReadTest, NoUnknownFieldsReturnsDefaultInstance) {
  InternalMetadata metadata(nullptr);
  const InternalMetadata& const_metadata = metadata;
  const intptr_t bits_before = RawBits(metadata);

  EXPECT_EQ(&const_metadata.unknown_fields<std::string>(&GetEmptyString),
            &GetEmptyString());
  EXPECT_EQ(&const_metadata.unknown_fields<UnknownFieldSet>(
                &UnknownFieldSet::default_instance),
            &UnknownFieldSet::default_instance());

  EXPECT_FALSE(metadata.have_unknown_fields());
  EXPECT_EQ(RawBits(metadata), bits_before);
  // The read must not have created a map entry.
  EXPECT_EQ(UnknownFieldMap::GlobalUnknownFieldMap().GetUnknownFieldSet<
                std::string>(&metadata),
            nullptr);
  EXPECT_EQ(UnknownFieldMap::GlobalUnknownFieldMap().GetUnknownFieldSet<
                UnknownFieldSet>(&metadata),
            nullptr);
}

TEST(InternalMetadataConstReadTest, HeapStringReadMatchesMutable) {
  InternalMetadata metadata(nullptr);
  std::string* mut = metadata.mutable_unknown_fields<std::string>();
  ASSERT_TRUE(metadata.have_unknown_fields());
  mut->assign("unknown bytes");

  const InternalMetadata& const_metadata = metadata;
  const intptr_t bits_before = RawBits(metadata);
  const std::string& read =
      const_metadata.unknown_fields<std::string>(&GetEmptyString);

  EXPECT_EQ(&read, mut);
  EXPECT_EQ(read, "unknown bytes");
  EXPECT_EQ(RawBits(metadata), bits_before);
  EXPECT_EQ(metadata.arena(), nullptr);

  metadata.Delete<std::string>();
  EXPECT_FALSE(metadata.have_unknown_fields());
}

TEST(InternalMetadataConstReadTest, HeapUnknownFieldSetReadMatchesMutable) {
  InternalMetadata metadata(nullptr);
  UnknownFieldSet* mut = metadata.mutable_unknown_fields<UnknownFieldSet>();
  mut->AddVarint(1, 42);

  const InternalMetadata& const_metadata = metadata;
  const intptr_t bits_before = RawBits(metadata);
  const UnknownFieldSet& read = const_metadata.unknown_fields<UnknownFieldSet>(
      &UnknownFieldSet::default_instance);

  EXPECT_EQ(&read, mut);
  ASSERT_EQ(read.field_count(), 1);
  EXPECT_EQ(read.field(0).varint(), 42);
  EXPECT_EQ(RawBits(metadata), bits_before);

  metadata.Delete<UnknownFieldSet>();
}

TEST(InternalMetadataConstReadTest, ArenaReadUsesArenaMap) {
  Arena arena;
  InternalMetadata metadata(&arena);
  std::string* mut = metadata.mutable_unknown_fields<std::string>();
  mut->assign("on arena");

  const InternalMetadata& const_metadata = metadata;
  const intptr_t bits_before = RawBits(metadata);
  const std::string& read =
      const_metadata.unknown_fields<std::string>(&GetEmptyString);

  EXPECT_EQ(&read, mut);
  EXPECT_EQ(RawBits(metadata), bits_before);
  EXPECT_EQ(metadata.arena(), &arena);
  // The entry lives in the arena's map, not the global one.
  EXPECT_EQ(UnknownFieldMap::ArenaUnknownFieldMap(arena)
                .GetUnknownFieldSet<std::string>(&metadata),
            mut);
  EXPECT_EQ(UnknownFieldMap::GlobalUnknownFieldMap().GetUnknownFieldSet<
                std::string>(&metadata),
            nullptr);
  // The arena owns the string; no Delete for arena metadata.
}

TEST(InternalMetadataConstReadTest, RepeatedReadsReturnSameObject) {
  InternalMetadata metadata(nullptr);
  UnknownFieldSet* mut = metadata.mutable_unknown_fields<UnknownFieldSet>();
  const InternalMetadata& const_metadata = metadata;

  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(&const_metadata.unknown_fields<UnknownFieldSet>(
                  &UnknownFieldSet::default_instance),
              mut);
  }
  // The mutable accessor keeps returning the same object as well.
  EXPECT_EQ(metadata.mutable_unknown_fields<UnknownFieldSet>(), mut);

  metadata.Delete<UnknownFieldSet>();
}

// Many threads reading the unknown fields of the same const metadata must not
// race. Run under --config=tsan to detect writes on the read path.
TEST(InternalMetadataConstReadTest, ConcurrentConstReads) {
  constexpr int kThreads = 8;
  constexpr int kReadsPerThread = 1000;

  InternalMetadata metadata(nullptr);
  metadata.mutable_unknown_fields<std::string>()->assign("shared");
  const InternalMetadata& const_metadata = metadata;
  const std::string* expected =
      &const_metadata.unknown_fields<std::string>(&GetEmptyString);

  absl::Notification start;
  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t) {
    threads.emplace_back([&] {
      start.WaitForNotification();
      for (int i = 0; i < kReadsPerThread; ++i) {
        const std::string& read =
            const_metadata.unknown_fields<std::string>(&GetEmptyString);
        EXPECT_EQ(&read, expected);
        EXPECT_EQ(read, "shared");
        EXPECT_TRUE(const_metadata.have_unknown_fields());
      }
    });
  }
  start.Notify();
  for (std::thread& t : threads) t.join();

  metadata.Delete<std::string>();
}

// Same as above, through the public message API.
TEST(InternalMetadataConstReadTest, ConcurrentConstReadsOnMessage) {
  constexpr int kThreads = 8;
  constexpr int kReadsPerThread = 1000;

  proto2_unittest::TestEmptyMessage msg;
  // Field 1 varint 1, field 2 varint 2.
  ASSERT_TRUE(msg.ParseFromString(std::string("\x08\x01\x10\x02", 4)));
  const proto2_unittest::TestEmptyMessage& const_msg = msg;

  absl::Notification start;
  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t) {
    threads.emplace_back([&] {
      start.WaitForNotification();
      for (int i = 0; i < kReadsPerThread; ++i) {
        EXPECT_EQ(const_msg.unknown_fields().field_count(), 2);
      }
    });
  }
  start.Notify();
  for (std::thread& t : threads) t.join();
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
