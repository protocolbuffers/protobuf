/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Test of generated code, with a special focus on features that are not used in
 * descriptor.proto or conformance.proto (since these get some testing from
 * upb/def.c and tests/conformance_upb.c, respectively).
 */

#include "src/google/protobuf/test_messages_proto3.upb.h"
#include "tests/upb_test.h"
#include "tests/test.upb.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))

const char test_str[] = "abcdefg";
const char test_str2[] = "12345678910";
const char test_str3[] = "rstlnezxcvbnm";
const char test_str4[] = "just another test string";

const upb_strview test_str_view = {test_str, sizeof(test_str) - 1};
const upb_strview test_str_view2 = {test_str2, sizeof(test_str2) - 1};
const upb_strview test_str_view3 = {test_str3, sizeof(test_str3) - 1};
const upb_strview test_str_view4 = {test_str4, sizeof(test_str4) - 1};

const int32_t test_int32 = 10;
const int32_t test_int32_2 = -20;
const int32_t test_int32_3 = 30;
const int32_t test_int32_4 = -40;

static void test_scalars(void) {
  upb_arena *arena = upb_arena_new();
  protobuf_test_messages_proto3_TestAllTypesProto3 *msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena);
  protobuf_test_messages_proto3_TestAllTypesProto3 *msg2;
  upb_strview serialized;
  upb_strview val;

  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_int32(msg, 10);
  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_int64(msg, 20);
  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_uint32(msg, 30);
  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_uint64(msg, 40);
  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_float(msg, 50.5);
  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_double(msg, 60.6);
  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_bool(msg, 1);
  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_string(
      msg, test_str_view);

  serialized.data = protobuf_test_messages_proto3_TestAllTypesProto3_serialize(
      msg, arena, &serialized.size);

  msg2 = protobuf_test_messages_proto3_TestAllTypesProto3_parse(
      serialized.data, serialized.size, arena);

  ASSERT(protobuf_test_messages_proto3_TestAllTypesProto3_optional_int32(
             msg2) == 10);
  ASSERT(protobuf_test_messages_proto3_TestAllTypesProto3_optional_int64(
             msg2) == 20);
  ASSERT(protobuf_test_messages_proto3_TestAllTypesProto3_optional_uint32(
             msg2) == 30);
  ASSERT(protobuf_test_messages_proto3_TestAllTypesProto3_optional_uint64(
             msg2) == 40);
  ASSERT(protobuf_test_messages_proto3_TestAllTypesProto3_optional_float(
             msg2) - 50.5 < 0.01);
  ASSERT(protobuf_test_messages_proto3_TestAllTypesProto3_optional_double(
             msg2) - 60.6 < 0.01);
  ASSERT(protobuf_test_messages_proto3_TestAllTypesProto3_optional_bool(
             msg2) == 1);
  val = protobuf_test_messages_proto3_TestAllTypesProto3_optional_string(msg2);
  ASSERT(upb_strview_eql(val, test_str_view));

  upb_arena_free(arena);
}

static void test_utf8(void) {
  const char invalid_utf8[] = "\xff";
  const upb_strview invalid_utf8_view = upb_strview_make(invalid_utf8, 1);
  upb_arena *arena = upb_arena_new();
  upb_strview serialized;
  protobuf_test_messages_proto3_TestAllTypesProto3 *msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena);
  protobuf_test_messages_proto3_TestAllTypesProto3 *msg2;

  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_string(
      msg, invalid_utf8_view);

  serialized.data = protobuf_test_messages_proto3_TestAllTypesProto3_serialize(
      msg, arena, &serialized.size);

  msg2 = protobuf_test_messages_proto3_TestAllTypesProto3_parse(
      serialized.data, serialized.size, arena);
  ASSERT(msg2 == NULL);

  upb_arena_free(arena);
}

static void check_string_map_empty(
    protobuf_test_messages_proto3_TestAllTypesProto3 *msg) {
  size_t iter = UPB_MAP_BEGIN;

  ASSERT(
      protobuf_test_messages_proto3_TestAllTypesProto3_map_string_string_size(
          msg) == 0);
  ASSERT(
      !protobuf_test_messages_proto3_TestAllTypesProto3_map_string_string_next(
          msg, &iter));
}

static void check_string_map_one_entry(
    protobuf_test_messages_proto3_TestAllTypesProto3 *msg) {
  const protobuf_test_messages_proto3_TestAllTypesProto3_MapStringStringEntry
      *const_ent;
  size_t iter;
  upb_strview str;

  ASSERT(
      protobuf_test_messages_proto3_TestAllTypesProto3_map_string_string_size(
          msg) == 1);
  ASSERT(protobuf_test_messages_proto3_TestAllTypesProto3_map_string_string_get(
      msg, test_str_view, &str));
  ASSERT(upb_strview_eql(str, test_str_view2));

  ASSERT(
      !protobuf_test_messages_proto3_TestAllTypesProto3_map_string_string_get(
          msg, test_str_view3, &str));

  /* Test that iteration reveals a single k/v pair in the map. */
  iter = UPB_MAP_BEGIN;
  const_ent = protobuf_test_messages_proto3_TestAllTypesProto3_map_string_string_next(
      msg, &iter);
  ASSERT(const_ent);
  ASSERT(upb_strview_eql(
      test_str_view,
      protobuf_test_messages_proto3_TestAllTypesProto3_MapStringStringEntry_key(
          const_ent)));
  ASSERT(upb_strview_eql(
      test_str_view2,
      protobuf_test_messages_proto3_TestAllTypesProto3_MapStringStringEntry_value(
          const_ent)));

  const_ent = protobuf_test_messages_proto3_TestAllTypesProto3_map_string_string_next(
      msg, &iter);
  ASSERT(!const_ent);
}

static void test_string_double_map(void) {
  upb_arena *arena = upb_arena_new();
  upb_strview serialized;
  upb_test_MapTest *msg = upb_test_MapTest_new(arena);
  upb_test_MapTest *msg2;
  double val;

  upb_test_MapTest_map_string_double_set(msg, test_str_view, 1.5, arena);
  ASSERT(msg);
  ASSERT(upb_test_MapTest_map_string_double_get(msg, test_str_view, &val));
  ASSERT(val == 1.5);
  val = 0;

  serialized.data = upb_test_MapTest_serialize(msg, arena, &serialized.size);
  ASSERT(serialized.data);

  msg2 = upb_test_MapTest_parse(serialized.data, serialized.size, arena);
  ASSERT(msg2);
  ASSERT(upb_test_MapTest_map_string_double_get(msg2, test_str_view, &val));
  ASSERT(val == 1.5);

  upb_arena_free(arena);
}

static void test_string_map(void) {
  upb_arena *arena = upb_arena_new();
  protobuf_test_messages_proto3_TestAllTypesProto3 *msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena);
  const protobuf_test_messages_proto3_TestAllTypesProto3_MapStringStringEntry
      *const_ent;
  size_t iter, count;

  check_string_map_empty(msg);

  /* Set map[test_str_view] = test_str_view2 */
  protobuf_test_messages_proto3_TestAllTypesProto3_map_string_string_set(
      msg, test_str_view, test_str_view2, arena);
  check_string_map_one_entry(msg);

  /* Deleting a non-existent key does nothing. */
  ASSERT(
      !protobuf_test_messages_proto3_TestAllTypesProto3_map_string_string_delete(
          msg, test_str_view3));
  check_string_map_one_entry(msg);

  /* Deleting the key sets the map back to empty. */
  ASSERT(
      protobuf_test_messages_proto3_TestAllTypesProto3_map_string_string_delete(
          msg, test_str_view));
  check_string_map_empty(msg);

  /* Set two keys this time:
   *   map[test_str_view] = test_str_view2
   *   map[test_str_view3] = test_str_view4
   */
  protobuf_test_messages_proto3_TestAllTypesProto3_map_string_string_set(
      msg, test_str_view, test_str_view2, arena);
  protobuf_test_messages_proto3_TestAllTypesProto3_map_string_string_set(
      msg, test_str_view3, test_str_view4, arena);

  /* Test iteration */
  iter = UPB_MAP_BEGIN;
  count = 0;

  while (
      (const_ent =
           protobuf_test_messages_proto3_TestAllTypesProto3_map_string_string_next(
               msg, &iter)) != NULL) {
    upb_strview key =
        protobuf_test_messages_proto3_TestAllTypesProto3_MapStringStringEntry_key(
            const_ent);
    upb_strview val =
        protobuf_test_messages_proto3_TestAllTypesProto3_MapStringStringEntry_value(
            const_ent);

    count++;
    if (upb_strview_eql(key, test_str_view)) {
      ASSERT(upb_strview_eql(val, test_str_view2));
    } else {
      ASSERT(upb_strview_eql(key, test_str_view3));
      ASSERT(upb_strview_eql(val, test_str_view4));
    }
  }

  ASSERT(count == 2);

  /* Clearing the map goes back to empty. */
  protobuf_test_messages_proto3_TestAllTypesProto3_map_string_string_clear(msg);
  check_string_map_empty(msg);

  upb_arena_free(arena);
}

static void check_int32_map_empty(
    protobuf_test_messages_proto3_TestAllTypesProto3 *msg) {
  size_t iter = UPB_MAP_BEGIN;

  ASSERT(
      protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_size(
          msg) == 0);
  ASSERT(
      !protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_next(
          msg, &iter));
}

static void check_int32_map_one_entry(
    protobuf_test_messages_proto3_TestAllTypesProto3 *msg) {
  const protobuf_test_messages_proto3_TestAllTypesProto3_MapInt32Int32Entry
      *const_ent;
  size_t iter;
  int32_t val;

  ASSERT(
      protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_size(
          msg) == 1);
  ASSERT(protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_get(
      msg, test_int32, &val));
  ASSERT(val == test_int32_2);

  ASSERT(
      !protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_get(
          msg, test_int32_3, &val));

  /* Test that iteration reveals a single k/v pair in the map. */
  iter = UPB_MAP_BEGIN;
  const_ent = protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_next(
      msg, &iter);
  ASSERT(const_ent);
  ASSERT(
      test_int32 ==
      protobuf_test_messages_proto3_TestAllTypesProto3_MapInt32Int32Entry_key(
          const_ent));
  ASSERT(
      test_int32_2 ==
      protobuf_test_messages_proto3_TestAllTypesProto3_MapInt32Int32Entry_value(
          const_ent));

  const_ent = protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_next(
      msg, &iter);
  ASSERT(!const_ent);
}

static void test_int32_map(void) {
  upb_arena *arena = upb_arena_new();
  protobuf_test_messages_proto3_TestAllTypesProto3 *msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena);
  const protobuf_test_messages_proto3_TestAllTypesProto3_MapInt32Int32Entry
      *const_ent;
  size_t iter, count;

  check_int32_map_empty(msg);

  /* Set map[test_int32] = test_int32_2 */
  protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_set(
      msg, test_int32, test_int32_2, arena);
  check_int32_map_one_entry(msg);

  /* Deleting a non-existent key does nothing. */
  ASSERT(
      !protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_delete(
          msg, test_int32_3));
  check_int32_map_one_entry(msg);

  /* Deleting the key sets the map back to empty. */
  ASSERT(
      protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_delete(
          msg, test_int32));
  check_int32_map_empty(msg);

  /* Set two keys this time:
   *   map[test_int32] = test_int32_2
   *   map[test_int32_3] = test_int32_4
   */
  protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_set(
      msg, test_int32, test_int32_2, arena);
  protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_set(
      msg, test_int32_3, test_int32_4, arena);

  /* Test iteration */
  iter = UPB_MAP_BEGIN;
  count = 0;

  while (
      (const_ent =
           protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_next(
               msg, &iter)) != NULL) {
    int32_t key =
        protobuf_test_messages_proto3_TestAllTypesProto3_MapInt32Int32Entry_key(
            const_ent);
    int32_t val =
        protobuf_test_messages_proto3_TestAllTypesProto3_MapInt32Int32Entry_value(
            const_ent);

    count++;
    if (key == test_int32) {
      ASSERT(val == test_int32_2);
    } else {
      ASSERT(key == test_int32_3);
      ASSERT(val == test_int32_4);
    }
  }

  ASSERT(count == 2);

  /* Clearing the map goes back to empty. */
  protobuf_test_messages_proto3_TestAllTypesProto3_map_int32_int32_clear(msg);
  check_int32_map_empty(msg);

  upb_arena_free(arena);
}

void test_repeated(void) {
  upb_arena *arena = upb_arena_new();
  protobuf_test_messages_proto3_TestAllTypesProto3 *msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(arena);
  size_t size;
  const int *elems;

  protobuf_test_messages_proto3_TestAllTypesProto3_add_repeated_int32(
      msg, 5, arena);

  elems = protobuf_test_messages_proto3_TestAllTypesProto3_repeated_int32(
      msg, &size);

  ASSERT(size == 1);
  ASSERT(elems[0] == 5);

  upb_arena_free(arena);
}

void test_null_decode_buf(void) {
  upb_arena *arena = upb_arena_new();
  protobuf_test_messages_proto3_TestAllTypesProto3 *msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_parse(NULL, 0, arena);
  size_t size;

  ASSERT(msg);
  protobuf_test_messages_proto3_TestAllTypesProto3_serialize(msg, arena, &size);
  ASSERT(size == 0);
  upb_arena_free(arena);
}

void test_status_truncation(void) {
  int i, j;
  upb_status status;
  upb_status status2;
  for (i = 0; i < UPB_STATUS_MAX_MESSAGE + 20; i++) {
    char *msg = malloc(i + 1);
    int end;
    char ch = (i % 96) + 33;  /* Cycle through printable chars. */

    for (j = 0; j < i; j++) {
      msg[j] = ch;
    }
    msg[i] = '\0';

    upb_status_seterrmsg(&status, msg);
    upb_status_seterrf(&status2, "%s", msg);
    end = MIN(i, UPB_STATUS_MAX_MESSAGE - 1);
    ASSERT(strlen(status.msg) == end);
    ASSERT(strlen(status2.msg) == end);

    for (j = 0; j < end; j++) {
      ASSERT(status.msg[j] == ch);
      ASSERT(status2.msg[j] == ch);
    }

    free(msg);
  }
}

void decrement_int(void *ptr) {
  int* iptr = ptr;
  (*iptr)--;
}

void test_arena_fuse(void) {
  int i1 = 5;
  int i2 = 5;
  int i3 = 5;
  int i4 = 5;

  upb_arena *arena1 = upb_arena_new();
  upb_arena *arena2 = upb_arena_new();

  upb_arena_addcleanup(arena1, &i1, decrement_int);
  upb_arena_addcleanup(arena2, &i2, decrement_int);

  ASSERT(upb_arena_fuse(arena1, arena2));

  upb_arena_addcleanup(arena1, &i3, decrement_int);
  upb_arena_addcleanup(arena2, &i4, decrement_int);

  upb_arena_free(arena1);
  ASSERT(i1 == 5);
  ASSERT(i2 == 5);
  ASSERT(i3 == 5);
  ASSERT(i4 == 5);
  upb_arena_free(arena2);
  ASSERT(i1 == 4);
  ASSERT(i2 == 4);
  ASSERT(i3 == 4);
  ASSERT(i4 == 4);
}

/* Do nothing allocator for testing */
static void *test_allocfunc(upb_alloc *alloc, void *ptr, size_t oldsize,
                            size_t size) {
  return upb_alloc_global.func(alloc, ptr, oldsize, size);
}
upb_alloc test_alloc = {&test_allocfunc};

void test_arena_fuse_with_initial_block(void) {
  char buf1[1024];
  char buf2[1024];
  upb_arena *arenas[] = {upb_arena_init(buf1, 1024, &upb_alloc_global),
                         upb_arena_init(buf2, 1024, &upb_alloc_global),
                         upb_arena_init(NULL, 0, &test_alloc),
                         upb_arena_init(NULL, 0, &upb_alloc_global)};
  int size = sizeof(arenas)/sizeof(arenas[0]);
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) {
      if (i == j) {
        ASSERT(upb_arena_fuse(arenas[i], arenas[j]));
      } else {
        ASSERT(!upb_arena_fuse(arenas[i], arenas[j]));
      }
    }
  }

  for (int i = 0; i < size; ++i) upb_arena_free(arenas[i]);
}

void test_arena_decode(void) {
  // Tests against a bug that previously existed when passing an arena to
  // upb_decode().
  char large_string[1024] = {0};
  upb_strview large_string_view = {large_string, sizeof(large_string)};
  upb_arena *tmp = upb_arena_new();

  protobuf_test_messages_proto3_TestAllTypesProto3 *msg =
      protobuf_test_messages_proto3_TestAllTypesProto3_new(tmp);

  protobuf_test_messages_proto3_TestAllTypesProto3_set_optional_bytes(
      msg, large_string_view);

  upb_strview serialized;
  serialized.data = protobuf_test_messages_proto3_TestAllTypesProto3_serialize(
      msg, tmp, &serialized.size);

  upb_arena *arena = upb_arena_new();
  // Parse the large payload, forcing an arena block to be allocated. This used
  // to corrupt the cleanup list, preventing subsequent upb_arena_addcleanup()
  // calls from working properly.
  protobuf_test_messages_proto3_TestAllTypesProto3_parse(
      serialized.data, serialized.size, arena);

  int i1 = 5;
  upb_arena_addcleanup(arena, &i1, decrement_int);
  ASSERT(i1 == 5);
  upb_arena_free(arena);
  ASSERT(i1 == 4);

  upb_arena_free(tmp);
}

void test_arena_unaligned(void) {
  char buf1[1024];
  // Force the pointer to be unaligned.
  char *unaligned_buf_ptr = (char*)((uintptr_t)buf1 | 7);
  upb_arena *arena = upb_arena_init(
      unaligned_buf_ptr, &buf1[sizeof(buf1)] - unaligned_buf_ptr, NULL);
  char *mem = upb_arena_malloc(arena, 5);
  ASSERT(((uintptr_t)mem & 15) == 0);
  upb_arena_free(arena);

  // Try the same, but with a size so small that aligning up will overflow.
  arena = upb_arena_init(unaligned_buf_ptr, 5, &upb_alloc_global);
  mem = upb_arena_malloc(arena, 5);
  ASSERT(((uintptr_t)mem & 15) == 0);
  upb_arena_free(arena);
}

int run_tests(int argc, char *argv[]) {
  test_scalars();
  test_utf8();
  test_string_map();
  test_string_double_map();
  test_int32_map();
  test_repeated();
  test_null_decode_buf();
  test_status_truncation();
  test_arena_fuse();
  test_arena_fuse_with_initial_block();
  test_arena_decode();
  test_arena_unaligned();
  return 0;
}
