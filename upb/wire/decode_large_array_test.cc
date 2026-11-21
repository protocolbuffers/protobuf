// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Decodes repeated fields whose backing storage crosses the 2 GiB and 4 GiB
// boundaries, to verify that array size/offset arithmetic does not overflow,
// through each of the decoder's paths for appending to a repeated scalar field.
// Also checks that a bool array stops growing cleanly at the maximum capacity.
//
// To avoid needing many GiB of physical memory, large allocations come from
// AliasingAlloc, which backs every block with just a few MiB: the first and
// last kAliasUnit bytes of each block are ordinary private memory, and every
// chunk in between maps the same shared kAliasUnit-sized region. This only
// works because every element the test writes into an array has the same
// value, so aliased writes are consistent.
//
// The aliasing would hide a write that lands a multiple of kAliasUnit away
// from where it belongs -- such as an offset that wrapped at 4 GiB. But such a
// write never reaches the unique tail of the block, so we fill each array
// exactly to capacity and check the tail.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <string>
#include <tuple>

#include <gtest/gtest.h>
#include "absl/container/flat_hash_map.h"
#include "upb/base/descriptor_constants.h"
#include "upb/base/status.h"
#include "upb/mem/alloc.h"
#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/array.h"
#include "upb/message/message.h"
#include "upb/mini_descriptor/build_enum.h"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_descriptor/internal/encode.hpp"
#include "upb/mini_descriptor/internal/modifiers.h"
#include "upb/mini_descriptor/link.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"

#if defined(__linux__) || defined(__APPLE__)
#define UPB_TEST_HAVE_ALIASING 1
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#endif

#if defined(__has_feature)
#if __has_feature(address_sanitizer) || __has_feature(memory_sanitizer) || \
    __has_feature(thread_sanitizer) || __has_feature(hwaddress_sanitizer)
#define UPB_TEST_SANITIZER 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define UPB_TEST_SANITIZER 1
#endif

namespace {

constexpr int kFieldNumber = 1;
constexpr size_t kGiB = size_t{1} << 30;

// We reach multi-GiB arrays by repeatedly merging the same (ordinary, small)
// input into one message. Arrays grow by powers of two and each input holds a
// power of two number of elements, so after a power of two number of decodes
// the array is exactly full.
constexpr size_t kInputBytes = size_t{64} << 20;  // 64 MiB

// A repeated scalar field, and the path the decoder takes to append to it.
struct FieldCase {
  const char* name;
  upb_FieldType type;
  bool packed;
  size_t elem_size;     // Size of an element in the array.
  uint64_t elem_value;  // Value of every element.
};

// Every case encodes each element with 0x01 bytes, so the input is trivial to
// build and every element has the same value.
const FieldCase kFieldCases[] = {
    // _upb_Decoder_DecodeFixedPacked(): appends all elements with memcpy().
    {"PackedFixed64", kUpb_FieldType_Fixed64, true, 8, 0x0101010101010101},
    // _upb_Decoder_DecodeVarintPacked(): appends one element at a time.
    {"PackedUInt64", kUpb_FieldType_UInt64, true, 8, 1},
    // _upb_Decoder_DecodeEnumPacked(): appends one (valid) element at a time.
    {"PackedClosedEnum", kUpb_FieldType_Enum, true, 4, 1},
    // _upb_Decoder_DecodeToArray(): appends one tag/value pair at a time.
    {"UnpackedUInt64", kUpb_FieldType_UInt64, false, 8, 1},
};

const FieldCase kBoolCase = {"PackedBool", kUpb_FieldType_Bool, true, 1, 1};

void PrintTo(const FieldCase& c, std::ostream* os) { *os << c.name; }

size_t InputBytesPerElem(const FieldCase& c) {
  if (!c.packed) return 2;  // Tag and value.
  return c.type == kUpb_FieldType_Fixed64 ? 8 : 1;
}

size_t InputElems(const FieldCase& c) {
  return kInputBytes / InputBytesPerElem(c);
}

// Returns the serialized form of a message with `elems` elements of field `c`.
std::string MakeInput(const FieldCase& c, size_t elems) {
  std::string input;
  if (c.packed) {
    input.push_back(static_cast<char>((kFieldNumber << 3) | 2));  // LEN
    const size_t len = elems * InputBytesPerElem(c);
    for (size_t v = len; v != 0; v >>= 7) {
      input.push_back(static_cast<char>((v & 0x7f) | (v >= 0x80 ? 0x80 : 0)));
    }
    input.append(len, '\x01');
  } else {
    input.reserve(elems * 2);
    for (size_t i = 0; i < elems; i++) {
      input.push_back(static_cast<char>(kFieldNumber << 3));  // VARINT
      input.push_back('\x01');
    }
  }
  return input;
}

std::string CaseName(const FieldCase& c, int options) {
  return std::string(c.name) +
         (options & kUpb_DecodeOption_DisableFastTable ? "_NoFastTable" : "");
}

constexpr int kDecodeOptions[] = {0, kUpb_DecodeOption_DisableFastTable};

#ifdef UPB_TEST_HAVE_ALIASING

// Size of the shared region, and of the unique head and tail of each block.
constexpr size_t kAliasUnit = size_t{2} << 20;  // 2 MiB
// Allocations at least this large are aliased; smaller ones are forwarded to
// the global allocator.
constexpr size_t kAliasThreshold = size_t{64} << 20;  // 64 MiB

class AliasingAlloc {
 public:
  AliasingAlloc() {
#if defined(__ANDROID__)
    // Bionic only provides memfd_create() as of Android API 30. Otherwise, use
    // the temporary file fallback below.
#if __ANDROID_MIN_SDK_VERSION__ >= 30
    fd_ = memfd_create("upb_decode_large_array_test", MFD_CLOEXEC);
#elif defined(__ANDROID_UNAVAILABLE_SYMBOLS_ARE_WEAK__)
    if (__builtin_available(android 30, *)) {
      fd_ = memfd_create("upb_decode_large_array_test", MFD_CLOEXEC);
    }
#endif
#elif defined(__GLIBC__) && \
    (__GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 27))
    // glibc only provides memfd_create() as of version 2.27, so use the
    // temporary file fallback below.
#elif defined(__linux__)
    fd_ = memfd_create("upb_decode_large_array_test", MFD_CLOEXEC);
#else
    char name[64];
    // macOS limits shared memory object names to 31 characters.
    snprintf(name, sizeof(name), "/upb_test_%d", static_cast<int>(getpid()));
    fd_ = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd_ >= 0) shm_unlink(name);
#endif
    if (fd_ < 0) {
      // Fall back to an unlinked temporary file.
      std::string path = testing::TempDir() + "upb_alias_XXXXXX";
      fd_ = mkstemp(path.data());
      if (fd_ >= 0) unlink(path.c_str());
    }
    if (fd_ >= 0 && ftruncate(fd_, kAliasUnit) != 0) {
      close(fd_);
      fd_ = -1;
    }
  }

  ~AliasingAlloc() {
    for (const auto& [ptr, len] : blocks_) munmap(ptr, len);
    if (fd_ >= 0) close(fd_);
  }

  bool ok() const { return fd_ >= 0; }
  upb_alloc* alloc() { return &alloc_.alloc; }

  // Size of the last allocation request that failed, or 0 if none failed.
  size_t failed_request() const { return failed_request_; }

  // Allocates memory that is freed when this object is destroyed.
  void* Malloc(size_t size) {
    return Func(&alloc_.alloc, nullptr, 0, size, nullptr);
  }

 private:
  static void* Func(upb_alloc* alloc, void* ptr, size_t oldsize, size_t size,
                    size_t* actual_size) {
    AliasingAlloc* self = reinterpret_cast<AllocWithOwner*>(alloc)->owner;
    auto it = ptr ? self->blocks_.find(ptr) : self->blocks_.end();
    if (it != self->blocks_.end()) {
      // upb arenas never resize blocks, so the only operation on one of our
      // blocks is freeing it.
      if (size != 0) {
        self->failed_request_ = size;
        return nullptr;
      }
      munmap(it->first, it->second);
      self->blocks_.erase(it);
      return nullptr;
    }
    if (ptr != nullptr || size < kAliasThreshold) {
      void* ret = upb_alloc_global.func(&upb_alloc_global, ptr, oldsize, size,
                                        actual_size);
      if (size != 0 && ret == nullptr) self->failed_request_ = size;
      return ret;
    }
    void* ret = self->MapAliased(size);
    if (ret == nullptr) self->failed_request_ = size;
    return ret;
  }

  void* MapAliased(size_t size) {
    const size_t len = (size + kAliasUnit - 1) / kAliasUnit * kAliasUnit;
    char* base = static_cast<char*>(mmap(nullptr, len, PROT_READ | PROT_WRITE,
                                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (base == MAP_FAILED) return nullptr;
    // Replace everything but the first and last units with the shared region.
    // Prefaulting the page tables is much faster than taking millions of page
    // faults as the test touches each page.
#ifdef MAP_POPULATE
    const int populate = MAP_POPULATE;
#else
    const int populate = 0;
#endif
    for (size_t ofs = kAliasUnit; ofs + kAliasUnit < len; ofs += kAliasUnit) {
      void* p = mmap(base + ofs, kAliasUnit, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_FIXED | populate, fd_, 0);
      if (p == MAP_FAILED) {
        munmap(base, len);
        return nullptr;
      }
    }
    blocks_[base] = len;
    return base;
  }

  struct AllocWithOwner {
    upb_alloc alloc;  // Must be first.
    AliasingAlloc* owner;
  };

  AllocWithOwner alloc_ = {{&AliasingAlloc::Func, nullptr}, this};
  int fd_ = -1;
  size_t failed_request_ = 0;
  absl::flat_hash_map<void*, size_t> blocks_;
};

#define SKIP_UNLESS_64_BIT()                              \
  do {                                                    \
    if (sizeof(size_t) < 8) {                             \
      GTEST_SKIP() << "Requires a 64-bit address space."; \
    }                                                     \
  } while (0)

#define SKIP_IF_ALLOC_FAILED(alias)                                     \
  do {                                                                  \
    if ((alias).failed_request() != 0) {                                \
      GTEST_SKIP() << "Unable to allocate " << (alias).failed_request() \
                   << " bytes.";                                        \
    }                                                                   \
  } while (0)

class DecodeLargeArrayTestBase : public testing::Test {
 protected:
  void SetUp() override {
#ifdef UPB_TEST_SANITIZER
    GTEST_SKIP() << "Sanitizer shadow memory would cover many GiB.";
#endif
    if (!alias_.ok()) GTEST_SKIP() << "Unable to create shared memory.";
    arena_ = upb_Arena_Init(nullptr, 0, alias_.alloc());
    SKIP_IF_ALLOC_FAILED(alias_);
    ASSERT_TRUE(arena_);
  }

  void TearDown() override {
    if (arena_) upb_Arena_Free(arena_);
  }

  // Returns a mini table with a single repeated field described by `c`.
  const upb_MiniTable* BuildMiniTable(const FieldCase& c) {
    const bool is_enum = c.type == kUpb_FieldType_Enum;
    upb::MtDataEncoder e;
    e.StartMessage(0);
    e.PutField(c.type, kFieldNumber,
               kUpb_FieldModifier_IsRepeated |
                   (c.packed ? kUpb_FieldModifier_IsPacked : 0) |
                   (is_enum ? kUpb_FieldModifier_IsClosedEnum : 0));
    upb_Status status;
    upb_Status_Clear(&status);
    upb_MiniTable* mt =
        upb_MiniTable_Build(e.data().data(), e.data().size(), arena_, &status);
    EXPECT_TRUE(mt) << upb_Status_ErrorMessage(&status);
    if (!mt || !is_enum) return mt;

    upb::MtDataEncoder enum_e;
    enum_e.StartEnum();
    enum_e.PutEnumValue(1);
    enum_e.EndEnum();
    upb_MiniTableEnum* enum_mt = upb_MiniTableEnum_Build(
        enum_e.data().data(), enum_e.data().size(), arena_, &status);
    EXPECT_TRUE(enum_mt) << upb_Status_ErrorMessage(&status);
    upb_MiniTableField* field = const_cast<upb_MiniTableField*>(
        upb_MiniTable_FindFieldByNumber(mt, kFieldNumber));
    if (!enum_mt || !upb_MiniTable_SetSubEnum(mt, field, enum_mt)) {
      ADD_FAILURE() << "Failed to link enum.";
      return nullptr;
    }
    return mt;
  }

  // Checks that `arr` has exactly `elems` elements of field `c` and is exactly
  // full, so that its last elements are in the unique tail of its block.
  void CheckFull(const upb_Array* arr, const FieldCase& c, size_t elems) {
    SCOPED_TRACE(testing::Message() << "elems=" << elems);
    ASSERT_TRUE(arr);
    ASSERT_EQ(upb_Array_Size(arr), elems);
    ASSERT_EQ(upb_Array_Capacity(arr), elems);
    const char* data = static_cast<const char*>(upb_Array_DataPtr(arr));
    const size_t check_elems = kAliasUnit / c.elem_size;
    for (size_t n = 0; n < check_elems; n++) {
      for (size_t i : {n, elems - 1 - n}) {
        uint64_t val = 0;
        switch (c.elem_size) {
          case 1:
            val = static_cast<unsigned char>(data[i]);
            break;
          case 4: {
            uint32_t v;
            memcpy(&v, data + i * 4, 4);
            val = v;
            break;
          }
          default:
            memcpy(&val, data + i * 8, 8);
            break;
        }
        ASSERT_EQ(val, c.elem_value) << "element " << i;
      }
    }
  }

  // Declared before the arena so that it outlives it.
  AliasingAlloc alias_;
  upb_Arena* arena_ = nullptr;
};

class DecodeLargeArrayTest
    : public DecodeLargeArrayTestBase,
      public testing::WithParamInterface<std::tuple<FieldCase, int>> {};

TEST_P(DecodeLargeArrayTest, Crosses2GiBAnd4GiB) {
  SKIP_UNLESS_64_BIT();
  const auto& [c, options] = GetParam();
  const upb_MiniTable* mt = BuildMiniTable(c);
  ASSERT_TRUE(mt);
  const upb_MiniTableField* field =
      upb_MiniTable_FindFieldByNumber(mt, kFieldNumber);
  const size_t input_elems = InputElems(c);
  const std::string input = MakeInput(c, input_elems);
  upb_Message* msg = upb_Message_New(mt, arena_);
  SKIP_IF_ALLOC_FAILED(alias_);
  ASSERT_TRUE(msg);

  // Decode until the array holds 8 GiB of elements, so that elements are
  // written beyond a byte offset of 4 GiB. Check at 2, 4, and 8 GiB.
  const size_t decodes_per_2gib = 2 * kGiB / (input_elems * c.elem_size);
  for (size_t i = 1; i <= 4 * decodes_per_2gib; i++) {
    const upb_DecodeStatus status = upb_Decode(input.data(), input.size(), msg,
                                               mt, nullptr, options, arena_);
    SKIP_IF_ALLOC_FAILED(alias_);
    ASSERT_EQ(status, kUpb_DecodeStatus_Ok) << "decode " << i;
    if (i == decodes_per_2gib || i == 2 * decodes_per_2gib ||
        i == 4 * decodes_per_2gib) {
      CheckFull(upb_Message_GetArray(msg, field), c, i * input_elems);
      if (HasFatalFailure()) return;
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    Cases, DecodeLargeArrayTest,
    testing::Combine(testing::ValuesIn(kFieldCases),
                     testing::ValuesIn(kDecodeOptions)),
    [](const testing::TestParamInfo<std::tuple<FieldCase, int>>& info) {
      return CaseName(std::get<0>(info.param), std::get<1>(info.param));
    });

class DecodeLargeBoolArrayTest : public DecodeLargeArrayTestBase,
                                 public testing::WithParamInterface<int> {};

// A bool array's byte offsets equal its element counts, so it cannot reach a
// 4 GiB offset. Instead, check that it can grow to 2^31 elements and beyond.
TEST_P(DecodeLargeBoolArrayTest, GrowsTo2GiBElements) {
  SKIP_UNLESS_64_BIT();
  const FieldCase& c = kBoolCase;
  const int options = GetParam();
  const upb_MiniTable* mt = BuildMiniTable(c);
  ASSERT_TRUE(mt);
  const upb_MiniTableField* field =
      upb_MiniTable_FindFieldByNumber(mt, kFieldNumber);
  const size_t input_elems = InputElems(c);
  const std::string input = MakeInput(c, input_elems);
  upb_Message* msg = upb_Message_New(mt, arena_);
  SKIP_IF_ALLOC_FAILED(alias_);
  ASSERT_TRUE(msg);

  const size_t max_elems = size_t{1} << 31;
  for (size_t i = 1; i <= max_elems / input_elems; i++) {
    const upb_DecodeStatus status = upb_Decode(input.data(), input.size(), msg,
                                               mt, nullptr, options, arena_);
    SKIP_IF_ALLOC_FAILED(alias_);
    ASSERT_EQ(status, kUpb_DecodeStatus_Ok) << "decode " << i;
  }
  CheckFull(upb_Message_GetArray(msg, field), c, max_elems);
  if (HasFatalFailure()) return;

  // One more element.
  const char one_more[] = {static_cast<char>(kFieldNumber << 3), 1};
  const upb_DecodeStatus status =
      upb_Decode(one_more, sizeof(one_more), msg, mt, nullptr, options, arena_);
  SKIP_IF_ALLOC_FAILED(alias_);
  EXPECT_EQ(status, kUpb_DecodeStatus_Ok);
  EXPECT_EQ(upb_Array_Size(upb_Message_GetArray(msg, field)), max_elems + 1);
}

INSTANTIATE_TEST_SUITE_P(Options, DecodeLargeBoolArrayTest,
                         testing::ValuesIn(kDecodeOptions),
                         [](const testing::TestParamInfo<int>& info) {
                           return CaseName(kBoolCase, info.param);
                         });

class DecodeCapacityOverflowTest : public DecodeLargeArrayTestBase,
                                   public testing::WithParamInterface<int> {};

// Grows an array of 8-byte elements from just under 2^27 elements (1 GiB) to
// just over 2^28 elements (2 GiB) in a single packed field. The array's
// capacity would round up to 2^29 elements, which is 4 GiB. With a 32-bit
// size_t, that byte size does not fit, so upb must refuse rather than wrap
// around to a tiny allocation. This needs only about 2 GiB of address space, so
// it also runs on 32-bit platforms.
TEST_P(DecodeCapacityOverflowTest, PackedFixed64GrowsPast2GiB) {
  const FieldCase& c = kFieldCases[0];
  ASSERT_EQ(c.type, kUpb_FieldType_Fixed64);
  const int options = GetParam();
  const upb_MiniTable* mt = BuildMiniTable(c);
  ASSERT_TRUE(mt);
  const upb_MiniTableField* field =
      upb_MiniTable_FindFieldByNumber(mt, kFieldNumber);
  upb_Message* msg = upb_Message_New(mt, arena_);
  SKIP_IF_ALLOC_FAILED(alias_);
  ASSERT_TRUE(msg);

  // Start with an array with capacity for 2^27 elements and room for one more
  // element: the decoder reserves room for one element before decoding a
  // packed field, and if the array were full, that would double it to 2 GiB
  // before the packed field's size is taken into account. (The array's
  // contents are not checked: they would not be consistent in aliased memory.)
  const size_t start_capacity = size_t{1} << 27;
  const size_t start_elems = start_capacity - 1;
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, field, arena_);
  SKIP_IF_ALLOC_FAILED(alias_);
  ASSERT_TRUE(arr);
  const bool resized = upb_Array_Resize(arr, start_capacity, arena_);
  SKIP_IF_ALLOC_FAILED(alias_);
  ASSERT_TRUE(resized);
  ASSERT_TRUE(upb_Array_Resize(arr, start_elems, arena_));
  ASSERT_EQ(upb_Array_Capacity(arr), start_capacity);

  // An input with enough elements to bring the array to 2^28 + 1 elements. It
  // is too big for an ordinary allocation on a 32-bit platform, so it comes
  // from its own aliased memory (the decoder will not copy it into the array
  // there).
  const size_t append_elems = (size_t{1} << 28) + 1 - start_elems;
  const size_t len = append_elems * c.elem_size;
  AliasingAlloc input_alias;
  if (!input_alias.ok()) GTEST_SKIP() << "Unable to create shared memory.";
  const size_t input_size = 1 + 5 + len;  // Tag, length, and data.
  char* input = static_cast<char*>(input_alias.Malloc(input_size));
  SKIP_IF_ALLOC_FAILED(input_alias);
  ASSERT_TRUE(input);
  memset(input, 0x01, input_size);
  input[0] = static_cast<char>((kFieldNumber << 3) | 2);  // LEN
  for (int i = 0; i < 5; i++) {
    input[1 + i] =
        static_cast<char>(((len >> (7 * i)) & 0x7f) | (i < 4 ? 0x80 : 0));
  }

  const upb_DecodeStatus status =
      upb_Decode(input, input_size, msg, mt, nullptr, options, arena_);
  if (sizeof(size_t) < 8) {
    // upb refuses to grow the array, without even trying to allocate memory.
    EXPECT_EQ(alias_.failed_request(), 0);
    EXPECT_EQ(status, kUpb_DecodeStatus_OutOfMemory);
    EXPECT_EQ(upb_Array_Size(arr), start_elems);
    EXPECT_EQ(upb_Array_Capacity(arr), start_capacity);
  } else {
    SKIP_IF_ALLOC_FAILED(alias_);
    EXPECT_EQ(status, kUpb_DecodeStatus_Ok);
    arr = upb_Message_GetMutableArray(msg, field);
    EXPECT_EQ(upb_Array_Size(arr), start_elems + append_elems);
    EXPECT_EQ(upb_Array_Capacity(arr), size_t{1} << 29);
  }
}

INSTANTIATE_TEST_SUITE_P(Options, DecodeCapacityOverflowTest,
                         testing::ValuesIn(kDecodeOptions),
                         [](const testing::TestParamInfo<int>& info) {
                           return CaseName(kFieldCases[0], info.param);
                         });

#else  // UPB_TEST_HAVE_ALIASING

TEST(DecodeLargeArrayTest, Unsupported) {
  GTEST_SKIP() << "Requires mmap() and a shared memory object.";
}

#endif  // UPB_TEST_HAVE_ALIASING

}  // namespace
