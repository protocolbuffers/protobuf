// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/io/string.h"

#include <string.h>

#include <gtest/gtest.h>
#include "upb/mem/arena.hpp"

TEST(StringTest, Append) {
  upb::Arena arena;

  upb_String foo;
  EXPECT_TRUE(upb_String_Init(&foo, arena.ptr()));
  EXPECT_EQ(upb_String_Size(&foo), 0);

  EXPECT_TRUE(upb_String_Assign(&foo, "foobar", 3));
  EXPECT_EQ(upb_String_Size(&foo), 3);
  EXPECT_EQ(strcmp(upb_String_Data(&foo), "foo"), 0);

  EXPECT_TRUE(upb_String_Append(&foo, "bar", 3));
  EXPECT_EQ(upb_String_Size(&foo), 6);
  EXPECT_EQ(strcmp(upb_String_Data(&foo), "foobar"), 0);

  EXPECT_TRUE(upb_String_Append(&foo, "baz", 3));
  EXPECT_EQ(upb_String_Size(&foo), 9);
  EXPECT_EQ(strcmp(upb_String_Data(&foo), "foobarbaz"), 0);

  EXPECT_TRUE(upb_String_Append(&foo, "bat", 3));
  EXPECT_EQ(upb_String_Size(&foo), 12);
  EXPECT_EQ(strcmp(upb_String_Data(&foo), "foobarbazbat"), 0);

  EXPECT_TRUE(upb_String_Append(&foo, "feefiefoefoo", 12));
  EXPECT_EQ(upb_String_Size(&foo), 24);
  EXPECT_EQ(strcmp(upb_String_Data(&foo), "foobarbazbatfeefiefoefoo"), 0);

  const char* password = "fiddlesnarf";
  EXPECT_TRUE(upb_String_Assign(&foo, password, strlen(password)));
  EXPECT_EQ(upb_String_Size(&foo), strlen(password));
  EXPECT_EQ(strcmp(upb_String_Data(&foo), password), 0);
}

TEST(StringTest, PushBack) {
  upb::Arena arena;

  upb_String foo;
  EXPECT_TRUE(upb_String_Init(&foo, arena.ptr()));
  EXPECT_EQ(upb_String_Size(&foo), 0);

  const std::string big =
      "asfashfxauwhfwu4fuwafxasnfwxnxwunxuwxufhwfaiwj4w9jvwxssldfjlasviorwnvwij"
      "grsdjrfiasrjrasijgraisjvrvoiasjspjfsjgfasjgiasjidjsrvjsrjrasjfrijwjajsrF"
      "JWJGF4WWJSAVSLJArSJGFrAISJGASrlafjgrivarijrraisrgjiawrijg3874f87f7hqfhpf"
      "f8929hr32p8475902387459023475297328-22-3776-26";
  EXPECT_TRUE(upb_String_Reserve(&foo, big.size() + 1));
  EXPECT_TRUE(upb_String_Append(&foo, big.data(), big.size()));
  EXPECT_EQ(upb_String_Size(&foo), big.size());
  EXPECT_EQ(strcmp(upb_String_Data(&foo), big.data()), 0);

  upb_String bar;
  EXPECT_TRUE(upb_String_Init(&bar, arena.ptr()));
  EXPECT_EQ(upb_String_Size(&bar), 0);

  EXPECT_TRUE(upb_String_PushBack(&bar, 'x'));
  EXPECT_TRUE(upb_String_PushBack(&bar, 'y'));
  EXPECT_TRUE(upb_String_PushBack(&bar, 'z'));
  EXPECT_TRUE(upb_String_PushBack(&bar, 'z'));
  EXPECT_TRUE(upb_String_PushBack(&bar, 'y'));
  EXPECT_EQ(upb_String_Size(&bar), 5);
  EXPECT_EQ(strcmp(upb_String_Data(&bar), "xyzzy"), 0);
}

TEST(StringTest, Erase) {
  upb::Arena arena;

  upb_String foo;
  EXPECT_TRUE(upb_String_Init(&foo, arena.ptr()));

  const char* sent = "This is an example sentence.";
  EXPECT_TRUE(upb_String_Assign(&foo, sent, strlen(sent)));
  EXPECT_EQ(upb_String_Size(&foo), 28);

  upb_String_Erase(&foo, 10, 8);
  EXPECT_EQ(upb_String_Size(&foo), 20);
  EXPECT_EQ(strcmp(upb_String_Data(&foo), "This is an sentence."), 0);

  upb_String_Erase(&foo, 9, 1);
  EXPECT_EQ(upb_String_Size(&foo), 19);
  EXPECT_EQ(strcmp(upb_String_Data(&foo), "This is a sentence."), 0);

  upb_String_Erase(&foo, 5, 5);
  EXPECT_EQ(upb_String_Size(&foo), 14);
  EXPECT_EQ(strcmp(upb_String_Data(&foo), "This sentence."), 0);

  upb_String_Erase(&foo, 4, 99);
  EXPECT_EQ(upb_String_Size(&foo), 4);
  EXPECT_EQ(strcmp(upb_String_Data(&foo), "This"), 0);

  upb_String_Erase(&foo, 0, 4);
  EXPECT_EQ(upb_String_Size(&foo), 0);
  EXPECT_EQ(strcmp(upb_String_Data(&foo), ""), 0);
}
