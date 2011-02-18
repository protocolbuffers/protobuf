
#undef NDEBUG  /* ensure tests always assert. */
#include "upb_table.h"
#include "upb_string.h"
#include "test_util.h"
#include <assert.h>
#include <map>
#include <string>
#include <vector>
#include <set>
#include <ext/hash_map>
#include <sys/resource.h>
#include <iostream>

bool benchmark = false;
#define CPU_TIME_PER_TEST 0.5

using std::string;
using std::vector;

typedef struct {
  uint32_t value;  /* key*2 */
} inttable_entry;

typedef struct {
  upb_strtable_entry e;
  int32_t value;  /* ASCII Value of first letter */
} strtable_entry;

double get_usertime()
{
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  return usage.ru_utime.tv_sec + (usage.ru_utime.tv_usec/1000000.0);
}

/* num_entries must be a power of 2. */
void test_strtable(const vector<string>& keys, uint32_t num_to_insert)
{
  /* Initialize structures. */
  upb_strtable table;
  std::map<string, int32_t> m;
  upb_strtable_init(&table, 0, sizeof(strtable_entry));
  std::set<string> all;
  for(size_t i = 0; i < num_to_insert; i++) {
    const string& key = keys[i];
    all.insert(key);
    strtable_entry e;
    e.value = key[0];
    upb_string *str = upb_strduplen(key.c_str(), key.size());
    e.e.key = str;
    upb_strtable_insert(&table, &e.e);
    upb_string_unref(str);  // The table still owns a ref.
    m[key] = key[0];
  }

  /* Test correctness. */
  for(uint32_t i = 0; i < keys.size(); i++) {
    const string& key = keys[i];
    upb_string *str = upb_strduplen(key.c_str(), key.size());
    strtable_entry *e = (strtable_entry*)upb_strtable_lookup(&table, str);
    printf("Looking up " UPB_STRFMT "...\n", UPB_STRARG(str));
    if(m.find(key) != m.end()) { /* Assume map implementation is correct. */
      assert(e);
      assert(upb_streql(e->e.key, str));
      assert(e->value == key[0]);
      assert(m[key] == key[0]);
    } else {
      assert(e == NULL);
    }
    upb_string_unref(str);
  }

  strtable_entry *e;
  for(e = (strtable_entry*)upb_strtable_begin(&table); e;
      e = (strtable_entry*)upb_strtable_next(&table, &e->e)) {
    string tmp(upb_string_getrobuf(e->e.key), upb_string_len(e->e.key));
    std::set<string>::iterator i = all.find(tmp);
    assert(i != all.end());
    all.erase(i);
  }
  assert(all.empty());

  upb_strtable_free(&table);
}

/* num_entries must be a power of 2. */
void test_inttable(int32_t *keys, uint16_t num_entries)
{
  /* Initialize structures. */
  upb_inttable table;
  uint32_t largest_key = 0;
  std::map<uint32_t, uint32_t> m;
  __gnu_cxx::hash_map<uint32_t, uint32_t> hm;
  upb_inttable_init(&table, num_entries, sizeof(inttable_entry));
  for(size_t i = 0; i < num_entries; i++) {
    int32_t key = keys[i];
    largest_key = UPB_MAX((int32_t)largest_key, key);
    inttable_entry e;
    e.value = (key*2) << 1;
    upb_inttable_insert(&table, key, &e);
    m[key] = key*2;
    hm[key] = key*2;
  }

  /* Test correctness. */
  for(uint32_t i = 0; i <= largest_key; i++) {
    inttable_entry *e = (inttable_entry*)upb_inttable_lookup(
        &table, i);
    if(m.find(i) != m.end()) { /* Assume map implementation is correct. */
      assert(e);
      //printf("addr: %p, expected: %d, actual: %d\n", e, i*2, e->value);
      assert(((e->value) >> 1) == i*2);
      assert(m[i] == i*2);
      assert(hm[i] == i*2);
    } else {
      assert(e == NULL);
    }
  }

  // Compact and test correctness again.
  upb_inttable_compact(&table);
  for(uint32_t i = 0; i <= largest_key; i++) {
    inttable_entry *e = (inttable_entry*)upb_inttable_lookup(
        &table, i);
    if(m.find(i) != m.end()) { /* Assume map implementation is correct. */
      assert(e);
      //printf("addr: %p, expected: %d, actual: %d\n", e, i*2, e->value);
      assert(((e->value) >> 1) == i*2);
      assert(m[i] == i*2);
      assert(hm[i] == i*2);
    } else {
      assert(e == NULL);
    }
  }

  if(!benchmark) {
    upb_inttable_free(&table);
    return;
  }

  /* Test performance. We only test lookups for keys that are known to exist. */
  uint16_t rand_order[num_entries];
  for(uint16_t i = 0; i < num_entries; i++) {
    rand_order[i] = i;
  }
  for(uint16_t i = num_entries - 1; i >= 1; i--) {
    uint16_t rand_i = (random() / (double)RAND_MAX) * i;
    assert(rand_i <= i);
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
  for(i = 0; true; i++) {
    if ((i & time_mask) == 0 && (get_usertime() - before) > CPU_TIME_PER_TEST) break;
    int32_t key = keys[i & mask];
    inttable_entry *e = (inttable_entry*)upb_inttable_lookup(&table, key);
    x += (uintptr_t)e;
  }
  double total = get_usertime() - before;
  printf("%s/s\n", eng(i/total, 3, false));

  printf("upb_inttable(rand): ");
  fflush(stdout);
  before = get_usertime();
  for(i = 0; true; i++) {
    if ((i & time_mask) == 0 && (get_usertime() - before) > CPU_TIME_PER_TEST) break;
    int32_t key = keys[rand_order[i & mask]];
    inttable_entry *e = (inttable_entry*)upb_inttable_lookup(&table, key);
    x += (uintptr_t)e;
  }
  total = get_usertime() - before;
  printf("%s/s\n", eng(i/total, 3, false));

  printf("std::map<int32_t, int32_t>(seq): ");
  fflush(stdout);
  before = get_usertime();
  for(i = 0; true; i++) {
    if ((i & time_mask) == 0 && (get_usertime() - before) > CPU_TIME_PER_TEST) break;
    int32_t key = keys[i & mask];
    x += m[key];
  }
  total = get_usertime() - before;
  printf("%s/s\n", eng(i/total, 3, false));

  printf("std::map<int32_t, int32_t>(rand): ");
  fflush(stdout);
  before = get_usertime();
  for(i = 0; true; i++) {
    if ((i & time_mask) == 0 && (get_usertime() - before) > CPU_TIME_PER_TEST) break;
    int32_t key = keys[rand_order[i & mask]];
    x += m[key];
  }
  total = get_usertime() - before;
  printf("%s/s\n", eng(i/total, 3, false));

  printf("__gnu_cxx::hash_map<uint32_t, uint32_t>(seq): ");
  fflush(stdout);
  before = get_usertime();
  for(i = 0; true; i++) {
    if ((i & time_mask) == 0 && (get_usertime() - before) > CPU_TIME_PER_TEST) break;
    int32_t key = keys[rand_order[i & mask]];
    x += hm[key];
  }
  total = get_usertime() - before;
  printf("%s/s\n", eng(i/total, 3, false));

  printf("__gnu_cxx::hash_map<uint32_t, uint32_t>(rand): ");
  fflush(stdout);
  before = get_usertime();
  for(i = 0; true; i++) {
    if ((i & time_mask) == 0 && (get_usertime() - before) > CPU_TIME_PER_TEST) break;
    int32_t key = keys[rand_order[i & mask]];
    x += hm[key];
  }
  total = get_usertime() - before;
  printf("%s/s\n\n", eng(i/total, 3, false));
  upb_inttable_free(&table);
}

int32_t *get_contiguous_keys(int32_t num)
{
  int32_t *buf = new int32_t[num];
  for(int32_t i = 0; i < num; i++)
    buf[i] = i+1;
  return buf;
}

int main(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--benchmark") == 0) benchmark = true;
  }

  vector<string> keys;
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

  printf("Benchmarking hash lookups in an integer-keyed hash table.\n");
  printf("\n");
  int32_t *keys1 = get_contiguous_keys(8);
  printf("Table size: 8, keys: 1-8 ====\n");
  test_inttable(keys1, 8);
  delete[] keys1;

  int32_t *keys2 = get_contiguous_keys(64);
  printf("Table size: 64, keys: 1-64 ====\n");
  test_inttable(keys2, 64);
  delete[] keys2;

  int32_t *keys3 = get_contiguous_keys(512);
  printf("Table size: 512, keys: 1-512 ====\n");
  test_inttable(keys3, 512);
  delete[] keys3;

  int32_t *keys4 = new int32_t[64];
  for(int32_t i = 0; i < 64; i++) {
    if(i < 32)
      keys4[i] = i+1;
    else
      keys4[i] = 10101+i;
  }
  printf("Table size: 64, keys: 1-32 and 10133-10164 ====\n");
  test_inttable(keys4, 64);
  delete[] keys4;
}
