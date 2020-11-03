/*
 *
 * Tests for upb_table.
 */

#include <limits.h>
#include <string.h>
#include <sys/resource.h>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "tests/upb_test.h"
#include "upb/table.int.h"

#include "upb/port_def.inc"

// Convenience interface for C++.  We don't put this in upb itself because
// the table is not exposed to users.

namespace upb {

template <class T> upb_value MakeUpbValue(T val);
template <class T> T GetUpbValue(upb_value val);
template <class T> upb_ctype_t GetUpbValueType();

#define FUNCS(name, type_t, enumval) \
  template<> upb_value MakeUpbValue<type_t>(type_t val) { return upb_value_ ## name(val); } \
  template<> type_t GetUpbValue<type_t>(upb_value val) { return upb_value_get ## name(val); } \
  template<> upb_ctype_t GetUpbValueType<type_t>() { return enumval; }

FUNCS(int32,    int32_t,      UPB_CTYPE_INT32)
FUNCS(int64,    int64_t,      UPB_CTYPE_INT64)
FUNCS(uint32,   uint32_t,     UPB_CTYPE_UINT32)
FUNCS(uint64,   uint64_t,     UPB_CTYPE_UINT64)
FUNCS(bool,     bool,         UPB_CTYPE_BOOL)
FUNCS(cstr,     char*,        UPB_CTYPE_CSTR)
FUNCS(ptr,      void*,        UPB_CTYPE_PTR)
FUNCS(constptr, const void*,  UPB_CTYPE_CONSTPTR)
FUNCS(fptr,     upb_func*,    UPB_CTYPE_FPTR)

#undef FUNCS

class IntTable {
 public:
  IntTable(upb_ctype_t value_type) { upb_inttable_init(&table_, value_type); }
  ~IntTable() { upb_inttable_uninit(&table_); }

  size_t count() { return upb_inttable_count(&table_); }

  bool Insert(uintptr_t key, upb_value val) {
    return upb_inttable_insert(&table_, key, val);
  }

  bool Replace(uintptr_t key, upb_value val) {
    return upb_inttable_replace(&table_, key, val);
  }

  std::pair<bool, upb_value> Remove(uintptr_t key) {
    std::pair<bool, upb_value> ret;
    ret.first = upb_inttable_remove(&table_, key, &ret.second);
    return ret;
  }

  std::pair<bool, upb_value> Lookup(uintptr_t key) const {
    std::pair<bool, upb_value> ret;
    ret.first = upb_inttable_lookup(&table_, key, &ret.second);
    return ret;
  }

  std::pair<bool, upb_value> Lookup32(uint32_t key) const {
    std::pair<bool, upb_value> ret;
    ret.first = upb_inttable_lookup32(&table_, key, &ret.second);
    return ret;
  }

  void Compact() { upb_inttable_compact(&table_); }

  class iterator : public std::iterator<std::forward_iterator_tag,
                                        std::pair<uintptr_t, upb_value> > {
   public:
    explicit iterator(IntTable* table) {
      upb_inttable_begin(&iter_, &table->table_);
    }

    static iterator end(IntTable* table) {
      iterator iter(table);
      upb_inttable_iter_setdone(&iter.iter_);
      return iter;
    }

    void operator++() {
      return upb_inttable_next(&iter_);
    }

    std::pair<uintptr_t, upb_value> operator*() const {
      std::pair<uintptr_t, upb_value> ret;
      ret.first = upb_inttable_iter_key(&iter_);
      ret.second = upb_inttable_iter_value(&iter_);
      return ret;
    }

    bool operator==(const iterator& other) const {
      return upb_inttable_iter_isequal(&iter_, &other.iter_);
    }

    bool operator!=(const iterator& other) const {
      return !(*this == other);
    }

   private:
    upb_inttable_iter iter_;
  };

  upb_inttable table_;
};

class StrTable {
 public:
  StrTable(upb_ctype_t value_type) { upb_strtable_init(&table_, value_type); }
  ~StrTable() { upb_strtable_uninit(&table_); }

  size_t count() { return upb_strtable_count(&table_); }

  bool Insert(const std::string& key, upb_value val) {
    return upb_strtable_insert2(&table_, key.c_str(), key.size(), val);
  }

  std::pair<bool, upb_value> Remove(const std::string& key) {
    std::pair<bool, upb_value> ret;
    ret.first =
        upb_strtable_remove2(&table_, key.c_str(), key.size(), &ret.second);
    return ret;
  }

  std::pair<bool, upb_value> Lookup(const std::string& key) const {
    std::pair<bool, upb_value> ret;
    ret.first =
        upb_strtable_lookup2(&table_, key.c_str(), key.size(), &ret.second);
    return ret;
  }

  void Resize(size_t size_lg2) {
    upb_strtable_resize(&table_, size_lg2, &upb_alloc_global);
  }

  class iterator : public std::iterator<std::forward_iterator_tag,
                                        std::pair<std::string, upb_value> > {
   public:
    explicit iterator(StrTable* table) {
      upb_strtable_begin(&iter_, &table->table_);
    }

    static iterator end(StrTable* table) {
      iterator iter(table);
      upb_strtable_iter_setdone(&iter.iter_);
      return iter;
    }

    void operator++() {
      return upb_strtable_next(&iter_);
    }

    std::pair<std::string, upb_value> operator*() const {
      std::pair<std::string, upb_value> ret;
      upb_strview view = upb_strtable_iter_key(&iter_);
      ret.first.assign(view.data, view.size);
      ret.second = upb_strtable_iter_value(&iter_);
      return ret;
    }

    bool operator==(const iterator& other) const {
      return upb_strtable_iter_isequal(&iter_, &other.iter_);
    }

    bool operator!=(const iterator& other) const {
      return !(*this == other);
    }

   private:
    upb_strtable_iter iter_;
  };

  upb_strtable table_;
};

template <class T> class TypedStrTable {
 public:
  TypedStrTable() : table_(GetUpbValueType<T>()) {}

  size_t count() { return table_.count(); }

  bool Insert(const std::string &key, T val) {
    return table_.Insert(key, MakeUpbValue<T>(val));
  }

  std::pair<bool, T> Remove(const std::string& key) {
    std::pair<bool, upb_value> found = table_.Remove(key);
    std::pair<bool, T> ret;
    ret.first = found.first;
    if (ret.first) {
      ret.second = GetUpbValue<T>(found.second);
    }
    return ret;
  }

  std::pair<bool, T> Lookup(const std::string& key) const {
    std::pair<bool, upb_value> found = table_.Lookup(key);
    std::pair<bool, T> ret;
    ret.first = found.first;
    if (ret.first) {
      ret.second = GetUpbValue<T>(found.second);
    }
    return ret;
  }

  void Resize(size_t size_lg2) {
    table_.Resize(size_lg2);
  }

  class iterator : public std::iterator<std::forward_iterator_tag, std::pair<std::string, T> > {
   public:
    explicit iterator(TypedStrTable* table) : iter_(&table->table_) {}
    static iterator end(TypedStrTable* table) {
      iterator iter(table);
      iter.iter_ = StrTable::iterator::end(&table->table_);
      return iter;
    }

    void operator++() { ++iter_; }

    std::pair<std::string, T> operator*() const {
      std::pair<std::string, upb_value> val = *iter_;
      std::pair<std::string, T> ret;
      ret.first = val.first;
      ret.second = GetUpbValue<T>(val.second);
      return ret;
    }

    bool operator==(const iterator& other) const {
      return iter_ == other.iter_;
    }

    bool operator!=(const iterator& other) const {
      return iter_ != other.iter_;
    }

   private:
    StrTable::iterator iter_;
  };

  iterator begin() { return iterator(this); }
  iterator end() { return iterator::end(this); }

  StrTable table_;
};

template <class T> class TypedIntTable {
 public:
  TypedIntTable() : table_(GetUpbValueType<T>()) {}

  size_t count() { return table_.count(); }

  bool Insert(uintptr_t key, T val) {
    return table_.Insert(key, MakeUpbValue<T>(val));
  }

  bool Replace(uintptr_t key, T val) {
    return table_.Replace(key, MakeUpbValue<T>(val));
  }

  std::pair<bool, T> Remove(uintptr_t key) {
    std::pair<bool, upb_value> found = table_.Remove(key);
    std::pair<bool, T> ret;
    ret.first = found.first;
    if (ret.first) {
      ret.second = GetUpbValue<T>(found.second);
    }
    return ret;
  }

  std::pair<bool, T> Lookup(uintptr_t key) const {
    std::pair<bool, upb_value> found = table_.Lookup(key);
    std::pair<bool, T> ret;
    ret.first = found.first;
    if (ret.first) {
      ret.second = GetUpbValue<T>(found.second);
    }
    return ret;
  }

  void Compact() { table_.Compact(); }

  class iterator : public std::iterator<std::forward_iterator_tag, std::pair<uintptr_t, T> > {
   public:
    explicit iterator(TypedIntTable* table) : iter_(&table->table_) {}
    static iterator end(TypedIntTable* table) {
      return IntTable::iterator::end(&table->table_);
    }

    void operator++() { ++iter_; }

    std::pair<uintptr_t, T> operator*() const {
      std::pair<uintptr_t, upb_value> val = *iter_;
      std::pair<uintptr_t, T> ret;
      ret.first = val.first;
      ret.second = GetUpbValue<T>(val.second);
      return ret;
    }

    bool operator==(const iterator& other) const {
      return iter_ == other.iter_;
    }

    bool operator!=(const iterator& other) const {
      return iter_ != other.iter_;
    }

   private:
    IntTable::iterator iter_;
  };

  iterator begin() { return iterator(this); }
  iterator end() { return iterator::end(this); }

  IntTable table_;
};

}

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
  std::map<std::string, int32_t> m;
  typedef upb::TypedStrTable<int32_t> Table;
  Table table;
  std::set<std::string> all;
  for(size_t i = 0; i < num_to_insert; i++) {
    const std::string& key = keys[i];
    all.insert(key);
    table.Insert(key, key[0]);
    m[key] = key[0];
  }

  /* Test correctness. */
  for(uint32_t i = 0; i < keys.size(); i++) {
    const std::string& key = keys[i];
    std::pair<bool, int32_t> found = table.Lookup(key);
    if(m.find(key) != m.end()) { /* Assume map implementation is correct. */
      ASSERT(found.first);
      ASSERT(found.second == key[0]);
      ASSERT(m[key] == key[0]);
    } else {
      ASSERT(!found.first);
    }
  }

  for (Table::iterator it = table.begin(); it != table.end(); ++it) {
    std::set<std::string>::iterator i = all.find((*it).first);
    ASSERT(i != all.end());
    all.erase(i);
  }
  ASSERT(all.empty());

  // Test iteration with resizes.

  for (int i = 0; i < 10; i++) {
    for (Table::iterator it = table.begin(); it != table.end(); ++it) {
      // Even if we invalidate the iterator it should only return real elements.
      ASSERT((*it).second == m[(*it).first]);

      // Force a resize even though the size isn't changing.
      // Also forces the table size to grow so some new buckets end up empty.
      int new_lg2 = table.table_.table_.t.size_lg2 + 1;
      // Don't use more than 64k tables, to avoid exhausting memory.
      new_lg2 = UPB_MIN(new_lg2, 16);
      table.Resize(new_lg2);
    }
  }

}

/* num_entries must be a power of 2. */
void test_inttable(int32_t *keys, uint16_t num_entries, const char *desc) {
  /* Initialize structures. */
  typedef upb::TypedIntTable<uint32_t> Table;
  Table table;
  uint32_t largest_key = 0;
  std::map<uint32_t, uint32_t> m;
  std::unordered_map<uint32_t, uint32_t> hm;
  for(size_t i = 0; i < num_entries; i++) {
    int32_t key = keys[i];
    largest_key = UPB_MAX((int32_t)largest_key, key);
    table.Insert(key, key * 2);
    m[key] = key*2;
    hm[key] = key*2;
  }

  /* Test correctness. */
  for(uint32_t i = 0; i <= largest_key; i++) {
    std::pair<bool, uint32_t> found = table.Lookup(i);
    if(m.find(i) != m.end()) { /* Assume map implementation is correct. */
      ASSERT(found.first);
      ASSERT(found.second == i*2);
      ASSERT(m[i] == i*2);
      ASSERT(hm[i] == i*2);
    } else {
      ASSERT(!found.first);
    }
  }

  for(uint16_t i = 0; i < num_entries; i += 2) {
    std::pair<bool, uint32_t> found = table.Remove(keys[i]);
    ASSERT(found.first == (m.erase(keys[i]) == 1));
    if (found.first) ASSERT(found.second == (uint32_t)keys[i] * 2);
    hm.erase(keys[i]);
    m.erase(keys[i]);
  }

  ASSERT(table.count() == hm.size());

  /* Test correctness. */
  for(uint32_t i = 0; i <= largest_key; i++) {
    std::pair<bool, uint32_t> found = table.Lookup(i);
    if(m.find(i) != m.end()) { /* Assume map implementation is correct. */
      ASSERT(found.first);
      ASSERT(found.second == i*2);
      ASSERT(m[i] == i*2);
      ASSERT(hm[i] == i*2);
    } else {
      ASSERT(!found.first);
    }
  }

  // Test replace.
  for(uint32_t i = 0; i <= largest_key; i++) {
    bool replaced = table.Replace(i, i*3);
    if(m.find(i) != m.end()) { /* Assume map implementation is correct. */
      ASSERT(replaced);
      m[i] = i * 3;
      hm[i] = i * 3;
    } else {
      ASSERT(!replaced);
    }
  }

  // Compact and test correctness again.
  table.Compact();
  for(uint32_t i = 0; i <= largest_key; i++) {
    std::pair<bool, uint32_t> found = table.Lookup(i);
    if(m.find(i) != m.end()) { /* Assume map implementation is correct. */
      ASSERT(found.first);
      ASSERT(found.second == i*3);
      ASSERT(m[i] == i*3);
      ASSERT(hm[i] == i*3);
    } else {
      ASSERT(!found.first);
    }
  }

  if(!benchmark) {
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
    bool ok = upb_inttable_lookup32(&table.table_.table_, key, &v);
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
    bool ok = upb_inttable_lookup32(&table.table_.table_, key, &v);
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

  printf("std::unordered_map<uint32_t, uint32_t>(seq): ");
  fflush(stdout);
  before = get_usertime();
  for(i = 0; true; i++) {
    MAYBE_BREAK;
    int32_t key = keys[rand_order[i & mask]];
    x += hm[key];
  }
  total = get_usertime() - before;
  printf("%ld/s (%0.1f%% of upb)\n", (long)(i/total), i / upb_seq_i);

  printf("std::unordered_map<uint32_t, uint32_t>(rand): ");
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
  delete[] rand_order;
}

/*
 * This test can't pass right now because the table can't store a value of
 * (uint64_t)-1.
 */
void test_int64_max_value() {
/*
  typedef upb::TypedIntTable<uint64_t> Table;
  Table table;
  uintptr_t uint64_max = (uint64_t)-1;
  table.Insert(1, uint64_max);
  std::pair<bool, uint64_t> found = table.Lookup(1);
  ASSERT(found.first);
  ASSERT(found.second == uint64_max);
*/
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

void test_init() {
  for (int i = 0; i < 2048; i++) {
    /* Tests that the size calculations in init() (lg2 size for target load)
     * work for all expected sizes. */
    upb_strtable t;
    upb_strtable_init2(&t, UPB_CTYPE_BOOL, i, &upb_alloc_global);
    upb_strtable_uninit(&t);
  }
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
  test_int64_max_value();

  return 0;
}

}
