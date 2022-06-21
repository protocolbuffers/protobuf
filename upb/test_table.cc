// Copyright (c) 2009-2021, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Google LLC nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/*
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

#include "gtest/gtest.h"
#include "upb/internal/table.h"
#include "upb/upb.hpp"

// Must be last.
#include "upb/port_def.inc"

// Convenience interface for C++.  We don't put this in upb itself because
// the table is not exposed to users.

namespace upb {

template <class T>
upb_value MakeUpbValue(T val);
template <class T>
T GetUpbValue(upb_value val);

#define FUNCS(name, type_t, enumval)           \
  template <>                                  \
  upb_value MakeUpbValue<type_t>(type_t val) { \
    return upb_value_##name(val);              \
  }                                            \
  template <>                                  \
  type_t GetUpbValue<type_t>(upb_value val) {  \
    return upb_value_get##name(val);           \
  }

FUNCS(int32, int32_t, UPB_CTYPE_INT32)
FUNCS(int64, int64_t, UPB_CTYPE_INT64)
FUNCS(uint32, uint32_t, UPB_CTYPE_UINT32)
FUNCS(uint64, uint64_t, UPB_CTYPE_UINT64)
FUNCS(bool, bool, UPB_CTYPE_BOOL)
FUNCS(cstr, char*, UPB_CTYPE_CSTR)
FUNCS(ptr, void*, UPB_CTYPE_PTR)
FUNCS(constptr, const void*, UPB_CTYPE_CONSTPTR)

#undef FUNCS

class IntTable {
 public:
  IntTable() { upb_inttable_init(&table_, arena_.ptr()); }

  size_t count() { return upb_inttable_count(&table_); }

  bool Insert(uintptr_t key, upb_value val) {
    return upb_inttable_insert(&table_, key, val, arena_.ptr());
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
    ret.first = upb_inttable_lookup(&table_, key, &ret.second);
    return ret;
  }

  void Compact() { upb_inttable_compact(&table_, arena_.ptr()); }

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

    void operator++() { return upb_inttable_next(&iter_); }

    std::pair<uintptr_t, upb_value> operator*() const {
      std::pair<uintptr_t, upb_value> ret;
      ret.first = upb_inttable_iter_key(&iter_);
      ret.second = upb_inttable_iter_value(&iter_);
      return ret;
    }

    bool operator==(const iterator& other) const {
      return upb_inttable_iter_isequal(&iter_, &other.iter_);
    }

    bool operator!=(const iterator& other) const { return !(*this == other); }

   private:
    upb_inttable_iter iter_;
  };

  upb::Arena arena_;
  upb_inttable table_;
};

class StrTable {
 public:
  StrTable() { upb_strtable_init(&table_, 4, arena_.ptr()); }

  size_t count() { return upb_strtable_count(&table_); }

  bool Insert(const std::string& key, upb_value val) {
    return upb_strtable_insert(&table_, key.c_str(), key.size(), val,
                               arena_.ptr());
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
    upb_strtable_resize(&table_, size_lg2, arena_.ptr());
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

    void operator++() { return upb_strtable_next(&iter_); }

    std::pair<std::string, upb_value> operator*() const {
      std::pair<std::string, upb_value> ret;
      upb_StringView view = upb_strtable_iter_key(&iter_);
      ret.first.assign(view.data, view.size);
      ret.second = upb_strtable_iter_value(&iter_);
      return ret;
    }

    bool operator==(const iterator& other) const {
      return upb_strtable_iter_isequal(&iter_, &other.iter_);
    }

    bool operator!=(const iterator& other) const { return !(*this == other); }

   private:
    upb_strtable_iter iter_;
  };

  upb::Arena arena_;
  upb_strtable table_;
};

template <class T>
class TypedStrTable {
 public:
  size_t count() { return table_.count(); }

  bool Insert(const std::string& key, T val) {
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

  void Resize(size_t size_lg2) { table_.Resize(size_lg2); }

  class iterator : public std::iterator<std::forward_iterator_tag,
                                        std::pair<std::string, T> > {
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

template <class T>
class TypedIntTable {
 public:
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

  class iterator : public std::iterator<std::forward_iterator_tag,
                                        std::pair<uintptr_t, T> > {
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

}  // namespace upb

#define CPU_TIME_PER_TEST 0.5

using std::vector;

double get_usertime() {
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  return usage.ru_utime.tv_sec + (usage.ru_utime.tv_usec / 1000000.0);
}

TEST(Table, StringTable) {
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

  /* Initialize structures. */
  std::map<std::string, int32_t> m;
  typedef upb::TypedStrTable<int32_t> Table;
  Table table;
  std::set<std::string> all;
  for (const auto& key : keys) {
    all.insert(key);
    table.Insert(key, key[0]);
    m[key] = key[0];
  }

  /* Test correctness. */
  for (const auto& key : keys) {
    std::pair<bool, int32_t> found = table.Lookup(key);
    if (m.find(key) != m.end()) { /* Assume map implementation is correct. */
      EXPECT_TRUE(found.first);
      EXPECT_EQ(found.second, key[0]);
      EXPECT_EQ(m[key], key[0]);
    } else {
      EXPECT_FALSE(found.first);
    }
  }

  for (Table::iterator it = table.begin(); it != table.end(); ++it) {
    std::set<std::string>::iterator i = all.find((*it).first);
    EXPECT_NE(i, all.end());
    all.erase(i);
  }
  EXPECT_TRUE(all.empty());

  // Test iteration with resizes.

  for (int i = 0; i < 10; i++) {
    for (Table::iterator it = table.begin(); it != table.end(); ++it) {
      // Even if we invalidate the iterator it should only return real elements.
      EXPECT_EQ((*it).second, m[(*it).first]);

      // Force a resize even though the size isn't changing.
      // Also forces the table size to grow so some new buckets end up empty.
      int new_lg2 = table.table_.table_.t.size_lg2 + 1;
      // Don't use more than 64k tables, to avoid exhausting memory.
      new_lg2 = UPB_MIN(new_lg2, 16);
      table.Resize(new_lg2);
    }
  }
}

class IntTableTest : public testing::TestWithParam<int> {
  void SetUp() override {
    if (GetParam() > 0) {
      for (int i = 0; i < GetParam(); i++) {
        keys_.push_back(i + 1);
      }
    } else {
      for (int32_t i = 0; i < 64; i++) {
        if (i < 32)
          keys_.push_back(i + 1);
        else
          keys_.push_back(10101 + i);
      }
    }
  }

 protected:
  std::vector<int32_t> keys_;
};

TEST_P(IntTableTest, TestIntTable) {
  /* Initialize structures. */
  typedef upb::TypedIntTable<uint32_t> Table;
  Table table;
  uint32_t largest_key = 0;
  std::map<uint32_t, uint32_t> m;
  std::unordered_map<uint32_t, uint32_t> hm;
  for (const auto& key : keys_) {
    largest_key = UPB_MAX((int32_t)largest_key, key);
    table.Insert(key, key * 2);
    m[key] = key * 2;
    hm[key] = key * 2;
  }

  /* Test correctness. */
  for (uint32_t i = 0; i <= largest_key; i++) {
    std::pair<bool, uint32_t> found = table.Lookup(i);
    if (m.find(i) != m.end()) { /* Assume map implementation is correct. */
      EXPECT_TRUE(found.first);
      EXPECT_EQ(found.second, i * 2);
      EXPECT_EQ(m[i], i * 2);
      EXPECT_EQ(hm[i], i * 2);
    } else {
      EXPECT_FALSE(found.first);
    }
  }

  for (size_t i = 0; i < keys_.size(); i += 2) {
    std::pair<bool, uint32_t> found = table.Remove(keys_[i]);
    EXPECT_EQ(found.first, m.erase(keys_[i]) == 1);
    if (found.first) {
      EXPECT_EQ(found.second, (uint32_t)keys_[i] * 2);
    }
    hm.erase(keys_[i]);
    m.erase(keys_[i]);
  }

  EXPECT_EQ(table.count(), hm.size());

  /* Test correctness. */
  for (uint32_t i = 0; i <= largest_key; i++) {
    std::pair<bool, uint32_t> found = table.Lookup(i);
    if (m.find(i) != m.end()) { /* Assume map implementation is correct. */
      EXPECT_TRUE(found.first);
      EXPECT_EQ(found.second, i * 2);
      EXPECT_EQ(m[i], i * 2);
      EXPECT_EQ(hm[i], i * 2);
    } else {
      EXPECT_FALSE(found.first);
    }
  }

  // Test replace.
  for (uint32_t i = 0; i <= largest_key; i++) {
    bool replaced = table.Replace(i, i * 3);
    if (m.find(i) != m.end()) { /* Assume map implementation is correct. */
      EXPECT_TRUE(replaced);
      m[i] = i * 3;
      hm[i] = i * 3;
    } else {
      EXPECT_FALSE(replaced);
    }
  }

  // Compact and test correctness again.
  table.Compact();
  for (uint32_t i = 0; i <= largest_key; i++) {
    std::pair<bool, uint32_t> found = table.Lookup(i);
    if (m.find(i) != m.end()) { /* Assume map implementation is correct. */
      EXPECT_TRUE(found.first);
      EXPECT_EQ(found.second, i * 3);
      EXPECT_EQ(m[i], i * 3);
      EXPECT_EQ(hm[i], i * 3);
    } else {
      EXPECT_FALSE(found.first);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(IntTableParams, IntTableTest,
                         testing::Values(8, 64, 512, -32));

/*
 * This test can't pass right now because the table can't store a value of
 * (uint64_t)-1.
 */
TEST(Table, MaxValue) {
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

TEST(Table, Delete) {
  upb::Arena arena;
  upb_inttable t;
  upb_inttable_init(&t, arena.ptr());
  upb_inttable_insert(&t, 0, upb_value_bool(true), arena.ptr());
  upb_inttable_insert(&t, 2, upb_value_bool(true), arena.ptr());
  upb_inttable_insert(&t, 4, upb_value_bool(true), arena.ptr());
  upb_inttable_compact(&t, arena.ptr());
  upb_inttable_remove(&t, 0, NULL);
  upb_inttable_remove(&t, 2, NULL);
  upb_inttable_remove(&t, 4, NULL);

  upb_inttable_iter iter;
  for (upb_inttable_begin(&iter, &t); !upb_inttable_done(&iter);
       upb_inttable_next(&iter)) {
    ASSERT_TRUE(false);
  }
}

TEST(Table, Init) {
  for (int i = 0; i < 2048; i++) {
    /* Tests that the size calculations in init() (lg2 size for target load)
     * work for all expected sizes. */
    upb::Arena arena;
    upb_strtable t;
    upb_strtable_init(&t, i, arena.ptr());
  }
}
