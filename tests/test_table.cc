/*
 *
 * Tests for upb_table.
 */

#include <limits.h>
#include <string.h>
#include <sys/resource.h>
#include <ext/hash_map>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "tests/upb_test.h"
#include "upb/table.int.h"

bool benchmark = false;
#define CPU_TIME_PER_TEST 0.5

using std::vector;

double get_usertime() {
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  return usage.ru_utime.tv_sec + (usage.ru_utime.tv_usec/1000000.0);
}

/* num_entries must be a power of 2. */
void test_strtable(const vector<std::string>& keys, uint32_t num_to_insert) {
  /* Initialize structures. */
  upb_strtable table;
  std::map<std::string, int32_t> m;
  upb_strtable_init(&table, UPB_CTYPE_INT32);
  std::set<std::string> all;
  for(size_t i = 0; i < num_to_insert; i++) {
    const std::string& key = keys[i];
    all.insert(key);
    upb_strtable_insert(&table, key.c_str(), upb_value_int32(key[0]));
    m[key] = key[0];
  }

  /* Test correctness. */
  for(uint32_t i = 0; i < keys.size(); i++) {
    const std::string& key = keys[i];
    upb_value v;
    bool found = upb_strtable_lookup(&table, key.c_str(), &v);
    if(m.find(key) != m.end()) { /* Assume map implementation is correct. */
      ASSERT(found);
      ASSERT(upb_value_getint32(v) == key[0]);
      ASSERT(m[key] == key[0]);
    } else {
      ASSERT(!found);
    }
  }

  upb_strtable_iter iter;
  for(upb_strtable_begin(&iter, &table); !upb_strtable_done(&iter);
      upb_strtable_next(&iter)) {
    const char *key = upb_strtable_iter_key(&iter);
    std::string tmp(key, strlen(key));
    ASSERT(strlen(key) == upb_strtable_iter_keylength(&iter));
    std::set<std::string>::iterator i = all.find(tmp);
    ASSERT(i != all.end());
    all.erase(i);
  }
  ASSERT(all.empty());

  // Test iteration with resizes.

  for (int i = 0; i < 10; i++) {
    for(upb_strtable_begin(&iter, &table); !upb_strtable_done(&iter);
        upb_strtable_next(&iter)) {
      // Even if we invalidate the iterator it should only return real elements.
      const char *key = upb_strtable_iter_key(&iter);
      std::string tmp(key, strlen(key));
      ASSERT(upb_value_getint32(upb_strtable_iter_value(&iter)) == m[tmp]);

      // Force a resize even though the size isn't changing.
      // Also forces the table size to grow so some new buckets end up empty.
      int new_lg2 = table.t.size_lg2 + 1;
      // Don't use more than 64k tables, to avoid exhausting memory.
      new_lg2 = UPB_MIN(new_lg2, 16);
      upb_strtable_resize(&table, new_lg2);
    }
  }

  upb_strtable_uninit(&table);
}

/* num_entries must be a power of 2. */
void test_inttable(int32_t *keys, uint16_t num_entries, const char *desc) {
  /* Initialize structures. */
  upb_inttable table;
  uint32_t largest_key = 0;
  std::map<uint32_t, uint32_t> m;
  __gnu_cxx::hash_map<uint32_t, uint32_t> hm;
  upb_inttable_init(&table, UPB_CTYPE_UINT32);
  for(size_t i = 0; i < num_entries; i++) {
    int32_t key = keys[i];
    largest_key = UPB_MAX((int32_t)largest_key, key);
    upb_inttable_insert(&table, key, upb_value_uint32(key * 2));
    m[key] = key*2;
    hm[key] = key*2;
  }

  /* Test correctness. */
  for(uint32_t i = 0; i <= largest_key; i++) {
    upb_value v;
    bool found = upb_inttable_lookup(&table, i, &v);
    if(m.find(i) != m.end()) { /* Assume map implementation is correct. */
      ASSERT(found);
      ASSERT(upb_value_getuint32(v) == i*2);
      ASSERT(m[i] == i*2);
      ASSERT(hm[i] == i*2);
    } else {
      ASSERT(!found);
    }
  }

  for(uint16_t i = 0; i < num_entries; i += 2) {
    upb_value val;
    bool ret = upb_inttable_remove(&table, keys[i], &val);
    ASSERT(ret == (m.erase(keys[i]) == 1));
    if (ret) ASSERT(upb_value_getuint32(val) == (uint32_t)keys[i] * 2);
    hm.erase(keys[i]);
    m.erase(keys[i]);
  }

  ASSERT(upb_inttable_count(&table) == hm.size());

  /* Test correctness. */
  for(uint32_t i = 0; i <= largest_key; i++) {
    upb_value v;
    bool found = upb_inttable_lookup(&table, i, &v);
    if(m.find(i) != m.end()) { /* Assume map implementation is correct. */
      ASSERT(found);
      ASSERT(upb_value_getuint32(v) == i*2);
      ASSERT(m[i] == i*2);
      ASSERT(hm[i] == i*2);
    } else {
      ASSERT(!found);
    }
  }

  // Test replace.
  for(uint32_t i = 0; i <= largest_key; i++) {
    upb_value v = upb_value_uint32(i*3);
    bool replaced = upb_inttable_replace(&table, i, v);
    if(m.find(i) != m.end()) { /* Assume map implementation is correct. */
      ASSERT(replaced);
      m[i] = i * 3;
      hm[i] = i * 3;
    } else {
      ASSERT(!replaced);
    }
  }

  // Compact and test correctness again.
  upb_inttable_compact(&table);
  for(uint32_t i = 0; i <= largest_key; i++) {
    upb_value v;
    bool found = upb_inttable_lookup(&table, i, &v);
    if(m.find(i) != m.end()) { /* Assume map implementation is correct. */
      ASSERT(found);
      ASSERT(upb_value_getuint32(v) == i*3);
      ASSERT(m[i] == i*3);
      ASSERT(hm[i] == i*3);
    } else {
      ASSERT(!found);
    }
  }

  if(!benchmark) {
    upb_inttable_uninit(&table);
    return;
  }

  printf("%s\n", desc);

  /* Test performance. We only test lookups for keys that are known to exist. */
  uint16_t *rand_order = new uint16_t[num_entries];
  for(uint16_t i = 0; i < num_entries; i++) {
    rand_order[i] = i;
  }
  for(uint16_t i = num_entries - 1; i >= 1; i--) {
    uint16_t rand_i = (random() / (double)RAND_MAX) * i;
    ASSERT(rand_i <= i);
    uint16_t tmp = rand_order[rand_i];
    rand_order[rand_i] = rand_order[i];
    rand_order[i] = tmp;
  }

  uintptr_t x = 0;
  const int mask = num_entries - 1;
  int time_mask = 0xffff;

  printf("upb_inttable(seq): ");
  fflush(stdout);
  double before = get_usertime();
  unsigned int i;

#define MAYBE_BREAK \
    if ((i & time_mask) == 0 && (get_usertime() - before) > CPU_TIME_PER_TEST) \
      break;
  for(i = 0; true; i++) {
    MAYBE_BREAK;
    int32_t key = keys[i & mask];
    upb_value v;
    bool ok = upb_inttable_lookup32(&table, key, &v);
    x += (uintptr_t)ok;
  }
  double total = get_usertime() - before;
  printf("%ld/s\n", (long)(i/total));
  double upb_seq_i = i / 100;  // For later percentage calcuation.

  printf("upb_inttable(rand): ");
  fflush(stdout);
  before = get_usertime();
  for(i = 0; true; i++) {
    MAYBE_BREAK;
    int32_t key = keys[rand_order[i & mask]];
    upb_value v;
    bool ok = upb_inttable_lookup32(&table, key, &v);
    x += (uintptr_t)ok;
  }
  total = get_usertime() - before;
  printf("%ld/s\n", (long)(i/total));
  double upb_rand_i = i / 100;  // For later percentage calculation.

  printf("std::map<int32_t, int32_t>(seq): ");
  fflush(stdout);
  before = get_usertime();
  for(i = 0; true; i++) {
    MAYBE_BREAK;
    int32_t key = keys[i & mask];
    x += m[key];
  }
  total = get_usertime() - before;
  printf("%ld/s (%0.1f%% of upb)\n", (long)(i/total), i / upb_seq_i);

  printf("std::map<int32_t, int32_t>(rand): ");
  fflush(stdout);
  before = get_usertime();
  for(i = 0; true; i++) {
    MAYBE_BREAK;
    int32_t key = keys[rand_order[i & mask]];
    x += m[key];
  }
  total = get_usertime() - before;
  printf("%ld/s (%0.1f%% of upb)\n", (long)(i/total), i / upb_rand_i);

  printf("__gnu_cxx::hash_map<uint32_t, uint32_t>(seq): ");
  fflush(stdout);
  before = get_usertime();
  for(i = 0; true; i++) {
    MAYBE_BREAK;
    int32_t key = keys[rand_order[i & mask]];
    x += hm[key];
  }
  total = get_usertime() - before;
  printf("%ld/s (%0.1f%% of upb)\n", (long)(i/total), i / upb_seq_i);

  printf("__gnu_cxx::hash_map<uint32_t, uint32_t>(rand): ");
  fflush(stdout);
  before = get_usertime();
  for(i = 0; true; i++) {
    MAYBE_BREAK;
    int32_t key = keys[rand_order[i & mask]];
    x += hm[key];
  }
  total = get_usertime() - before;
  if (x == INT_MAX) abort();
  printf("%ld/s (%0.1f%% of upb)\n\n", (long)(i/total), i / upb_rand_i);
  upb_inttable_uninit(&table);
  delete[] rand_order;
}

int32_t *get_contiguous_keys(int32_t num) {
  int32_t *buf = new int32_t[num];
  for(int32_t i = 0; i < num; i++)
    buf[i] = i;
  return buf;
}

void test_delete() {
  upb_inttable t;
  upb_inttable_init(&t, UPB_CTYPE_BOOL);
  upb_inttable_insert(&t, 0, upb_value_bool(true));
  upb_inttable_insert(&t, 2, upb_value_bool(true));
  upb_inttable_insert(&t, 4, upb_value_bool(true));
  upb_inttable_compact(&t);
  upb_inttable_remove(&t, 0, NULL);
  upb_inttable_remove(&t, 2, NULL);
  upb_inttable_remove(&t, 4, NULL);

  upb_inttable_iter iter;
  for (upb_inttable_begin(&iter, &t); !upb_inttable_done(&iter);
       upb_inttable_next(&iter)) {
    ASSERT(false);
  }

  upb_inttable_uninit(&t);
}

extern "C" {

int run_tests(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "benchmark") == 0) benchmark = true;
  }

  vector<std::string> keys;
  keys.push_back("google.protobuf.FileDescriptorSet");
  keys.push_back("google.protobuf.FileDescriptorProto");
  keys.push_back("google.protobuf.DescriptorProto");
  keys.push_back("google.protobuf.DescriptorProto.ExtensionRange");
  keys.push_back("google.protobuf.FieldDescriptorProto");
  keys.push_back("google.protobuf.EnumDescriptorProto");
  keys.push_back("google.protobuf.EnumValueDescriptorProto");
  keys.push_back("google.protobuf.ServiceDescriptorProto");
  keys.push_back("google.protobuf.MethodDescriptorProto");
  keys.push_back("google.protobuf.FileOptions");
  keys.push_back("google.protobuf.MessageOptions");
  keys.push_back("google.protobuf.FieldOptions");
  keys.push_back("google.protobuf.EnumOptions");
  keys.push_back("google.protobuf.EnumValueOptions");
  keys.push_back("google.protobuf.ServiceOptions");
  keys.push_back("google.protobuf.MethodOptions");
  keys.push_back("google.protobuf.UninterpretedOption");
  keys.push_back("google.protobuf.UninterpretedOption.NamePart");

  for (int i = 0; i < 10; i++) {
    test_strtable(keys, 18);
  }

  int32_t *keys1 = get_contiguous_keys(8);
  test_inttable(keys1, 8, "Table size: 8, keys: 1-8 ====");
  delete[] keys1;

  int32_t *keys2 = get_contiguous_keys(64);
  test_inttable(keys2, 64, "Table size: 64, keys: 1-64 ====\n");
  delete[] keys2;

  int32_t *keys3 = get_contiguous_keys(512);
  test_inttable(keys3, 512, "Table size: 512, keys: 1-512 ====\n");
  delete[] keys3;

  int32_t *keys4 = new int32_t[64];
  for(int32_t i = 0; i < 64; i++) {
    if(i < 32)
      keys4[i] = i+1;
    else
      keys4[i] = 10101+i;
  }
  test_inttable(keys4, 64, "Table size: 64, keys: 1-32 and 10133-10164 ====\n");
  delete[] keys4;

  test_delete();

  return 0;
}

}
