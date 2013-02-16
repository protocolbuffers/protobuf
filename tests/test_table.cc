/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2012 Google Inc.  See LICENSE for details.
 *
 * Tests for upb_table.
 */

#include <string.h>
#include <sys/resource.h>
#include <ext/hash_map>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "tests/test_util.h"
#include "tests/upb_test.h"
#include "upb/table.h"

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
    const upb_value *v = upb_strtable_lookup(&table, key.c_str());
    if(m.find(key) != m.end()) { /* Assume map implementation is correct. */
      ASSERT(v);
      ASSERT(upb_value_getint32(*v) == key[0]);
      ASSERT(m[key] == key[0]);
    } else {
      ASSERT(v == NULL);
    }
  }

  upb_strtable_iter iter;
  for(upb_strtable_begin(&iter, &table); !upb_strtable_done(&iter);
      upb_strtable_next(&iter)) {
    const char *key = upb_strtable_iter_key(&iter);
    std::string tmp(key, strlen(key));
    std::set<std::string>::iterator i = all.find(tmp);
    ASSERT(i != all.end());
    all.erase(i);
  }
  ASSERT(all.empty());

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
    const upb_value *v = upb_inttable_lookup(&table, i);
    if(m.find(i) != m.end()) { /* Assume map implementation is correct. */
      ASSERT(v);
      ASSERT(upb_value_getuint32(*v) == i*2);
      ASSERT(m[i] == i*2);
      ASSERT(hm[i] == i*2);
    } else {
      ASSERT(v == NULL);
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
    const upb_value *v = upb_inttable_lookup(&table, i);
    if(m.find(i) != m.end()) { /* Assume map implementation is correct. */
      ASSERT(v);
      ASSERT(upb_value_getuint32(*v) == i*2);
      ASSERT(m[i] == i*2);
      ASSERT(hm[i] == i*2);
    } else {
      ASSERT(v == NULL);
    }
  }

  // Compact and test correctness again.
  upb_inttable_compact(&table);
  for(uint32_t i = 0; i <= largest_key; i++) {
    const upb_value *v = upb_inttable_lookup(&table, i);
    if(m.find(i) != m.end()) { /* Assume map implementation is correct. */
      ASSERT(v);
      ASSERT(upb_value_getuint32(*v) == i*2);
      ASSERT(m[i] == i*2);
      ASSERT(hm[i] == i*2);
    } else {
      ASSERT(v == NULL);
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
    const upb_value *v = upb_inttable_lookup32(&table, key);
    x += (uintptr_t)v;
  }
  double total = get_usertime() - before;
  printf("%s/s\n", eng(i/total, 3, false));

  printf("upb_inttable(rand): ");
  fflush(stdout);
  before = get_usertime();
  for(i = 0; true; i++) {
    MAYBE_BREAK;
    int32_t key = keys[rand_order[i & mask]];
    const upb_value *v = upb_inttable_lookup32(&table, key);
    x += (uintptr_t)v;
  }
  total = get_usertime() - before;
  printf("%s/s\n", eng(i/total, 3, false));

  printf("std::map<int32_t, int32_t>(seq): ");
  fflush(stdout);
  before = get_usertime();
  for(i = 0; true; i++) {
    MAYBE_BREAK;
    int32_t key = keys[i & mask];
    x += m[key];
  }
  total = get_usertime() - before;
  printf("%s/s\n", eng(i/total, 3, false));

  printf("std::map<int32_t, int32_t>(rand): ");
  fflush(stdout);
  before = get_usertime();
  for(i = 0; true; i++) {
    MAYBE_BREAK;
    int32_t key = keys[rand_order[i & mask]];
    x += m[key];
  }
  total = get_usertime() - before;
  printf("%s/s\n", eng(i/total, 3, false));

  printf("__gnu_cxx::hash_map<uint32_t, uint32_t>(seq): ");
  fflush(stdout);
  before = get_usertime();
  for(i = 0; true; i++) {
    MAYBE_BREAK;
    int32_t key = keys[rand_order[i & mask]];
    x += hm[key];
  }
  total = get_usertime() - before;
  printf("%s/s\n", eng(i/total, 3, false));

  printf("__gnu_cxx::hash_map<uint32_t, uint32_t>(rand): ");
  fflush(stdout);
  before = get_usertime();
  for(i = 0; true; i++) {
    MAYBE_BREAK;
    int32_t key = keys[rand_order[i & mask]];
    x += hm[key];
  }
  total = get_usertime() - before;
  printf("%s/s\n\n", eng(i/total, 3, false));
  upb_inttable_uninit(&table);
  delete rand_order;
}

int32_t *get_contiguous_keys(int32_t num) {
  int32_t *buf = new int32_t[num];
  for(int32_t i = 0; i < num; i++)
    buf[i] = i;
  return buf;
}

extern "C" {

int run_tests(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--benchmark") == 0) benchmark = true;
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

  test_strtable(keys, 18);

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
  return 0;
}

}
