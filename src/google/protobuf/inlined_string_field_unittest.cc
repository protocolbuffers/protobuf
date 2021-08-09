// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <google/protobuf/inlined_string_field.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/arenastring.h>
#include <gtest/gtest.h>
#include <google/protobuf/stubs/strutil.h>


namespace google {
namespace protobuf {

using internal::ArenaStringPtr;
using internal::InlinedStringField;

static std::string WrapString(const char* value) { return value; }

namespace {

const uint32 kMask = ~0x00000001u;
const uint32 kMask1 = ~0x00000004u;
const uint32 kMask2 = ~0x00000020u;

TEST(InlinedStringFieldTest, SetOnHeap) {
  InlinedStringField field;
  uint32 donating_states = 0;
  const std::string kDefaultValue = "default";
  field.Set(&kDefaultValue, WrapString("Test short"), nullptr, false,
            &donating_states, kMask);
  EXPECT_EQ(std::string("Test short"), field.Get());
  field.Set(&kDefaultValue, WrapString("Test long long long long value"),
            nullptr, false, &donating_states, kMask);
  EXPECT_EQ(std::string("Test long long long long value"), field.Get());
}

TEST(InlinedStringFieldTest, SetRvalueOnHeap) {
  InlinedStringField field;
  uint32 donating_states = 0;
  std::string string_moved = "Moved long long long long string 1";
  field.Set(nullptr, std::move(string_moved), nullptr, false, &donating_states,
            kMask);
  EXPECT_EQ("Moved long long long long string 1", field.Get());
  EXPECT_EQ(donating_states & ~kMask1, 0);
}

TEST(InlinedStringFieldTest, UnsafeMutablePointerThenRelease) {
  InlinedStringField field;
  const std::string kDefaultValue = "default";
  std::string* mut = field.UnsafeMutablePointer();
  // The address of inlined string doesn't change behind the scene.
  EXPECT_EQ(mut, field.UnsafeMutablePointer());
  EXPECT_EQ(mut, &field.Get());
  EXPECT_EQ(std::string(""), *mut);
  *mut = "Test long long long long value";  // ensure string allocates
  EXPECT_EQ(std::string("Test long long long long value"), field.Get());

  std::string* released = field.ReleaseNonDefaultNoArena(&kDefaultValue);
  EXPECT_EQ("Test long long long long value", *released);
  // Default value is ignored.
  EXPECT_EQ("", field.Get());
  delete released;
}

// When donating mechanism is enabled:
// - Initially, the string is donated.
// - After lvalue Set: the string is still donated.
// - After Mutable: the string is undonated. The data buffer of the string is a
// new buffer on the heap.
TEST(InlinedStringFieldTest, ArenaSetThenMutable) {
  Arena arena;
  auto* field_arena = Arena::CreateMessage<InlinedStringField>(&arena);
  uint32 donating_states = ~0u;
  const std::string kDefaultValue = "default";
  field_arena->Set(&kDefaultValue, WrapString("Test short"), &arena,
                   /*donated=*/true, &donating_states, kMask1);
  EXPECT_EQ(std::string("Test short"), field_arena->Get());
  field_arena->Set(&kDefaultValue, "Test long long long long value", &arena,
                   /*donated=*/(donating_states & ~kMask1) != 0,
                   &donating_states, kMask1);
  EXPECT_EQ(std::string("Test long long long long value"), field_arena->Get());
#if GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE
  EXPECT_NE(donating_states & ~kMask1, 0);  // donate.
#endif

  const std::string* old_string = &field_arena->Get();
  const char* old_data = old_string->data();
  (void)old_data;
  std::string* mut = field_arena->Mutable(
      ArenaStringPtr::EmptyDefault{}, &arena,
      /*donated=*/(donating_states & ~kMask1) != 0, &donating_states, kMask1);
  EXPECT_EQ(old_string, mut);
#if GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE
  EXPECT_EQ(donating_states & ~kMask1, 0);
  EXPECT_NE(old_data, mut->data());  // The data buffer of the mutated string is
                                     // a new buffer on the heap.
#endif
  *mut = "Test an even longer long long long long value";
  EXPECT_EQ(std::string("Test an even longer long long long long value"),
            field_arena->Get());
  EXPECT_EQ(&field_arena->Get(), mut);
}

// Release doesn't change the donating state.
// When donating mechanism is enabled:
// - Initially, the string is donated.
// - Then lvalue Set: the string is still donated.
// - Then Release: the string is cleared, and still donated.
// - Then Mutable: the string is undonated.
// - Then Release: the string is still undonated.
TEST(InlinedStringFieldTest, ArenaRelease) {
  Arena arena;
  auto* field_arena = Arena::CreateMessage<InlinedStringField>(&arena);
  uint32 donating_states = ~0u;
  const std::string kDefaultValue = "default";
  field_arena->Set(&kDefaultValue, WrapString("Test short"), &arena,
                   /*donated=*/true, &donating_states, kMask1);
  std::string* released = field_arena->Release(
      &kDefaultValue, &arena, /*donated=*/(donating_states & ~kMask1) != 0);
  EXPECT_EQ("Test short", *released);
  EXPECT_EQ("", field_arena->Get());
  EXPECT_NE(released, &field_arena->Get());
#if GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE
  EXPECT_NE(donating_states & ~kMask1, 0);  // still donated.
#endif
  delete released;

  std::string* mut = field_arena->Mutable(
      ArenaStringPtr::EmptyDefault{}, &arena,
      /*donated=*/(donating_states & ~kMask1) != 0u, &donating_states, kMask1);
  *mut = "Test long long long long value";
  std::string* released2 =
      field_arena->Release(&kDefaultValue, &arena,
                           /*donated=*/(donating_states & ~kMask1) != 0);
  EXPECT_EQ("Test long long long long value", *released2);
  EXPECT_EQ("", field_arena->Get());
#if GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE
  EXPECT_EQ(donating_states & ~kMask1, 0);  // undonated.
#endif
  delete released2;
}

// Rvalue Set always undoantes a donated string.
TEST(InlinedStringFieldTest, SetRvalueArena) {
  Arena arena;
  auto* field1_arena = Arena::CreateMessage<InlinedStringField>(&arena);
  auto* field2_arena = Arena::CreateMessage<InlinedStringField>(&arena);
  uint32 donating_states = ~0u;
  const std::string kDefaultValue = "default";

  std::string string_moved = "Moved long long long long string 1";
  field1_arena->Set(nullptr, std::move(string_moved), &arena, true,
                    &donating_states, kMask1);
  EXPECT_EQ("Moved long long long long string 1", field1_arena->Get());
#if GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE
  EXPECT_EQ(donating_states & ~kMask1, 0);  // Undonate.
#endif

  field2_arena->Set(nullptr, std::string("string 2 on heap"), &arena, true,
                    &donating_states, kMask);
  EXPECT_EQ("string 2 on heap", field2_arena->Get());
#if GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE
  EXPECT_EQ(donating_states & ~kMask, 0);  // Undonated.
#endif
}

// Tests SetAllocated for non-arena string fields and arena string fields.
TEST(InlinedStringFieldTest, SetAllocated) {
  InlinedStringField field;
  Arena arena;
  auto* field1_arena = Arena::CreateMessage<InlinedStringField>(&arena);
  auto* field2_arena = Arena::CreateMessage<InlinedStringField>(&arena);
  uint32 donating_states = ~0u;
  const std::string kDefaultValue = "default";

  // The string in field is on heap.
  field.Set(&kDefaultValue, WrapString("String on heap"), nullptr, false,
            &donating_states, kMask);
  auto* allocated = new std::string("Allocated string on heap");
  field.SetAllocatedNoArena(&kDefaultValue, allocated);
  EXPECT_EQ("Allocated string on heap", field.Get());

  // The string in field1_arena is on arena (aka. donated).
  field1_arena->Set(&kDefaultValue, WrapString("String 1 on arena"), &arena,
                    true, &donating_states, kMask1);
  *field1_arena->Mutable(ArenaStringPtr::EmptyDefault{}, &arena,
                         (donating_states & ~kMask1) != 0, &donating_states,
                         kMask1) = "Mutated string 1 is now on heap long long";
  // After Mutable, the string is undonated.
  allocated = new std::string("Allocated string on heap long long long");
  field1_arena->SetAllocated(&kDefaultValue, allocated, &arena,
                             (donating_states & ~kMask1) != 0, &donating_states,
                             kMask1);
  EXPECT_EQ("Allocated string on heap long long long", field1_arena->Get());
#if GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE
  EXPECT_EQ(donating_states & ~kMask1, 0);  // Still undonated.
#endif

  // The string in field2_arena is on arena (aka. donated).
  field2_arena->Set(&kDefaultValue, WrapString("String 2 on arena long long"),
                    &arena, true, &donating_states,
                    kMask2);  // Still donated.
  allocated = new std::string("Allocated string on heap long long long 2");
  field2_arena->SetAllocated(&kDefaultValue, allocated, &arena, true,
                             &donating_states, kMask2);
  EXPECT_EQ("Allocated string on heap long long long 2", field2_arena->Get());
#if GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE
  EXPECT_EQ(donating_states & ~kMask2, 0);  // Undonated.
#endif
}

// Tests Swap for non-arena string fields and arena string fields.
TEST(InlinedStringFieldTest, Swap) {
  // Swap should only be called when the from and to are on the same arena.
  InlinedStringField field1;
  InlinedStringField field2;
  uint32 donating_states = 0;
  Arena arena;
  auto* field1_arena = Arena::CreateMessage<InlinedStringField>(&arena);
  auto* field2_arena = Arena::CreateMessage<InlinedStringField>(&arena);
  uint32 donating_states_1 = ~0u;
  uint32 donating_states_2 = ~0u;
  const std::string kDefaultValue = "default";

  const std::string& string1_heap = "String 1 on heap";
  const std::string& string2_heap = "String 2 on heap long long long long";
  const std::string& string1_arena = "String 1 on arena";
  const std::string& string2_arena = "String 2 on arena long long long long";
  field1.SetNoArena(&kDefaultValue, string1_heap);
  field2.SetNoArena(&kDefaultValue, string2_heap);
  field1_arena->Set(&kDefaultValue, string1_arena, &arena, true,
                    &donating_states_1, kMask1);
  field2_arena->Set(&kDefaultValue, string2_arena, &arena, true,
                    &donating_states_2, kMask1);

  field1.Swap(&field2, &kDefaultValue, nullptr, false, false, &donating_states,
              &donating_states_2, kMask);
  field1_arena->Swap(field2_arena, &kDefaultValue, &arena, true, true,
                     &donating_states_1, &donating_states_2, kMask1);
  EXPECT_EQ(field1.Get(), string2_heap);
  EXPECT_EQ(field2.Get(), string1_heap);
  EXPECT_EQ(field1_arena->Get(), string2_arena);
  EXPECT_EQ(field2_arena->Get(), string1_arena);
}

}  // namespace
}  // namespace protobuf
}  // namespace google
