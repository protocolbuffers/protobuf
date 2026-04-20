// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>
#include "upb/test/parse_text_proto.h"
#include "upb/util/def_to_proto_fuzz_impl.h"

namespace upb_test {

TEST(FuzzTest, EmptyPackage) {
  RoundTripDescriptor(ParseTextProtoOrDie(R"pb(file { package: "" })pb"));
}

TEST(FuzzTest, EmptyName) {
  RoundTripDescriptor(ParseTextProtoOrDie(R"pb(file { name: "" })pb"));
}

TEST(FuzzTest, EmptyPackage2) {
  RoundTripDescriptor(
      ParseTextProtoOrDie(R"pb(file { name: "n" package: "" })pb"));
}

TEST(FuzzTest, FileNameEmbeddedNull) {
  RoundTripDescriptor(ParseTextProtoOrDie(R"pb(file { name: "\000" })pb"));
}

TEST(FuzzTest, DuplicateOneofIndex) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             name: "F"
             message_type {
               name: "M"
               oneof_decl { name: "O" }
               field { name: "f1" number: 1 type: TYPE_INT32 oneof_index: 0 }
               field { name: "f2" number: 1 type: TYPE_INT32 oneof_index: 0 }
             }
           })pb"));
}

TEST(FuzzTest, NanValue) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             enum_type {
               value {
                 number: 0
                 options { uninterpreted_option { double_value: nan } }
               }
             }
           })pb"));
}

TEST(FuzzTest, EnumValueEmbeddedNull) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             name: "\035"
             enum_type {
               name: "f"
               value { name: "\000" number: 0 }
             }
           })pb"));
}

TEST(FuzzTest, EnumValueNoNumber) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             name: "\035"
             enum_type {
               name: "f"
               value { name: "abc" }
             }
           })pb"));
}

TEST(FuzzTest, DefaultWithUnterminatedHex) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             name: "\035"
             message_type {
               name: "A"
               field {
                 name: "f"
                 number: 1
                 label: LABEL_OPTIONAL
                 type: TYPE_BYTES
                 default_value: "\\x"
               }
             }
           })pb"));
}

TEST(FuzzTest, DefaultWithValidHexEscape) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             name: "\035"
             message_type {
               name: "A"
               field {
                 name: "f"
                 number: 1
                 label: LABEL_OPTIONAL
                 type: TYPE_BYTES
                 default_value: "\\x03"
               }
             }
           })pb"));
}

TEST(FuzzTest, DefaultWithValidHexEscapePrintable) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             name: "\035"
             message_type {
               name: "A"
               field {
                 name: "f"
                 number: 1
                 label: LABEL_OPTIONAL
                 type: TYPE_BYTES
                 default_value: "\\x23"  # 0x32 = '#'
               }
             }
           })pb"));
}

TEST(FuzzTest, PackageStartsWithNumber) {
  RoundTripDescriptor(
      ParseTextProtoOrDie(R"pb(file { name: "" package: "0" })pb"));
}

TEST(FuzzTest, RoundTripDescriptorRegression) {
  RoundTripDescriptor(ParseTextProtoOrDie(R"pb(file {
                                                 name: ""
                                                 message_type {
                                                   name: "A"
                                                   field {
                                                     name: "B"
                                                     number: 1
                                                     type: TYPE_BYTES
                                                     default_value: "\007"
                                                   }
                                                 }
                                               })pb"));
}

// Multiple oneof fields which have the same name.
TEST(FuzzTest, RoundTripDescriptorRegressionOneofSameName) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             name: "N"
             package: ""
             message_type {
               name: "b"
               field { name: "W" number: 1 type: TYPE_BYTES oneof_index: 0 }
               field { name: "W" number: 17 type: TYPE_UINT32 oneof_index: 0 }
               oneof_decl { name: "k" }
             }
           })pb"));
}

TEST(FuzzTest, NegativeOneofIndex) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             message_type {
               name: "A"
               field { name: "A" number: 0 type_name: "" oneof_index: -1 }
             }
           }
      )pb"));
}

TEST(FuzzTest, EnumVisibility) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             name: "n"
             package: "p"
             enum_type {
               name: "E"
               value { name: "MM" number: 0 }
               visibility: VISIBILITY_EXPORT
             }
             message_type { name: "M" visibility: VISIBILITY_EXPORT }
           }
      )pb"));
}

}  // namespace upb_test
