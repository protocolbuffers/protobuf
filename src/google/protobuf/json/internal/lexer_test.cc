// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/json/internal/lexer.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <ostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/algorithm/container.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/io/test_zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/stubs/status_macros.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace json_internal {
namespace {
using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Pair;
using ::testing::SizeIs;
using ::testing::VariantWith;

// TODO: Use the gtest versions once that's available in OSS.
MATCHER_P(IsOkAndHolds, inner,
          absl::StrCat("is OK and holds ", testing::PrintToString(inner))) {
  if (!arg.ok()) {
    *result_listener << arg.status();
    return false;
  }
  return testing::ExplainMatchResult(inner, *arg, result_listener);
}

// absl::Status GetStatus(const absl::Status& s) { return s; }
template <typename T>
absl::Status GetStatus(const absl::StatusOr<T>& s) {
  return s.status();
}

MATCHER_P(StatusIs, status,
          absl::StrCat(".status() is ", testing::PrintToString(status))) {
  return GetStatus(arg).code() == status;
}

#define EXPECT_OK(x) EXPECT_THAT(x, StatusIs(absl::StatusCode::kOk))
#define ASSERT_OK(x) ASSERT_THAT(x, StatusIs(absl::StatusCode::kOk))

// TODO: There are several tests that validate non-standard
// behavior that is assumed to be present in the wild due to Hyrum's Law. These
// tests are grouped under the `NonStandard` suite. These tests ensure the
// non-standard syntax is accepted, and that disabling legacy mode rejects them.
//
// All other tests are strictly-conforming.

// A generic JSON value, which is gtest-matcher friendly and stream-printable.
struct Value {
  static absl::StatusOr<Value> Parse(io::ZeroCopyInputStream* stream,
                                     ParseOptions options = {}) {
    JsonLexer lex(stream, options);
    return Parse(lex);
  }
  static absl::StatusOr<Value> Parse(JsonLexer& lex) {
    absl::StatusOr<JsonLexer::Kind> kind = lex.PeekKind();
    RETURN_IF_ERROR(kind.status());

    switch (*kind) {
      case JsonLexer::kNull:
        RETURN_IF_ERROR(lex.Expect("null"));
        return Value{Null{}};
      case JsonLexer::kFalse:
        RETURN_IF_ERROR(lex.Expect("false"));
        return Value{false};
      case JsonLexer::kTrue:
        RETURN_IF_ERROR(lex.Expect("true"));
        return Value{true};
      case JsonLexer::kNum: {
        absl::StatusOr<LocationWith<double>> num = lex.ParseNumber();
        RETURN_IF_ERROR(num.status());
        return Value{num->value};
      }
      case JsonLexer::kStr: {
        absl::StatusOr<LocationWith<MaybeOwnedString>> str = lex.ParseUtf8();
        RETURN_IF_ERROR(str.status());
        return Value{str->value.ToString()};
      }
      case JsonLexer::kArr: {
        std::vector<Value> arr;
        absl::Status s = lex.VisitArray([&arr, &lex]() -> absl::Status {
          absl::StatusOr<Value> val = Value::Parse(lex);
          RETURN_IF_ERROR(val.status());
          arr.emplace_back(*std::move(val));
          return absl::OkStatus();
        });
        RETURN_IF_ERROR(s);
        return Value{std::move(arr)};
      }
      case JsonLexer::kObj: {
        std::vector<std::pair<std::string, Value>> obj;
        absl::Status s = lex.VisitObject(
            [&obj, &lex](LocationWith<MaybeOwnedString>& key) -> absl::Status {
              absl::StatusOr<Value> val = Value::Parse(lex);
              RETURN_IF_ERROR(val.status());
              obj.emplace_back(std::move(key.value.ToString()),
                               *std::move(val));
              return absl::OkStatus();
            });
        RETURN_IF_ERROR(s);
        return Value{std::move(obj)};
      }
    }
    return absl::InternalError("Unrecognized kind in lexer");
  }

  friend std::ostream& operator<<(std::ostream& os, const Value& v) {
    if (std::holds_alternative<Null>(v.value)) {
      os << "null";
    } else if (const auto* x = std::get_if<bool>(&v.value)) {
      os << "bool:" << (*x ? "true" : "false");
    } else if (const auto* x = std::get_if<double>(&v.value)) {
      os << "num:" << *x;
    } else if (const auto* x = std::get_if<std::string>(&v.value)) {
      os << "str:" << absl::CHexEscape(*x);
    } else if (const auto* x = std::get_if<Array>(&v.value)) {
      os << "arr:[";
      bool first = true;
      for (const auto& val : *x) {
        if (!first) {
          os << ", ";
        }
        os << val;
      }
      os << "]";
    } else if (const auto* x = std::get_if<Object>(&v.value)) {
      os << "obj:[";
      bool first = true;
      for (const auto& kv : *x) {
        if (!first) {
          os << ", ";
          first = false;
        }
        os << kv.first << ":" << kv.second;
      }
      os << "]";
    }
    return os;
  }

  struct Null {};
  using Array = std::vector<Value>;
  using Object = std::vector<std::pair<std::string, Value>>;
  std::variant<Null, bool, double, std::string, Array, Object> value;
};

template <typename T, typename M>
testing::Matcher<const Value&> ValueIs(M inner) {
  return Field(&Value::value, VariantWith<T>(inner));
}

// Executes `test` once for each three-segment split of `json`.
void Do(absl::string_view json,
        std::function<void(io::ZeroCopyInputStream*)> test,
        bool verify_all_consumed = true) {
  SCOPED_TRACE(absl::StrCat("json: ", absl::CHexEscape(json)));

  for (size_t i = 0; i < json.size(); ++i) {
    for (size_t j = 0; j < json.size() - i + 1; ++j) {
      SCOPED_TRACE(absl::StrFormat("json[0:%d], json[%d:%d], json[%d:%d]", i, i,
                                   i + j, i + j, json.size()));
      std::string first(json.substr(0, i));
      std::string second(json.substr(i, j));
      std::string third(json.substr(i + j));

      io::internal::TestZeroCopyInputStream in{first, second, third};
      test(&in);
      if (testing::Test::HasFailure()) {
        return;
      }

      if (verify_all_consumed) {
        // Check that any unread bytes are just whitespace.
        int64_t byte_count = in.ByteCount();
        ASSERT_LE(byte_count, json.size());
        EXPECT_TRUE(
            absl::c_all_of(json.substr(byte_count, json.size() - byte_count),
                           [](char c) { return absl::ascii_isspace(c); }));
      }
    }
  }
}

void BadInner(absl::string_view json, ParseOptions opts = {}) {
  Do(
      json,
      [=](io::ZeroCopyInputStream* stream) {
        EXPECT_THAT(Value::Parse(stream, opts),
                    StatusIs(absl::StatusCode::kInvalidArgument));
      },
      false);
}

// Like Do, but runs a legacy syntax test twice: once with legacy settings, once
// without. For the latter, the test is expected to fail; for the former,
// `test` is called so it can run expectations.
void DoLegacy(absl::string_view json, std::function<void(const Value&)> test) {
  Do(json, [&](io::ZeroCopyInputStream* stream) {
    ParseOptions options;
    options.allow_legacy_syntax = true;
    auto value = Value::Parse(stream, options);
    ASSERT_OK(value);
    test(*value);
  });
  BadInner(json);
}

// Like Bad, but ensures json fails to parse in both modes.
void Bad(absl::string_view json) {
  ParseOptions options;
  options.allow_legacy_syntax = true;
  BadInner(json, options);
  BadInner(json);
}

TEST(LexerTest, Null) {
  Do("null", [](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream), IsOkAndHolds(ValueIs<Value::Null>(_)));
  });
}

TEST(LexerTest, False) {
  Do("false", [](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream), IsOkAndHolds(ValueIs<bool>(false)));
  });
}

TEST(LexerTest, True) {
  Do("true", [](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream), IsOkAndHolds(ValueIs<bool>(true)));
  });
}

TEST(LexerTest, Typos) {
  Bad("-");
  Bad("-foo");
  Bad("nule");
}

TEST(LexerTest, UnknownCharacters) {
  Bad("*&#25");
  Bad("[*&#25]");
  Bad("{key: *&#25}");
}

TEST(LexerTest, EmptyString) {
  Do(R"json("")json", [](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream),
                IsOkAndHolds(ValueIs<std::string>(IsEmpty())));
  });
}

TEST(LexerTest, SimpleString) {
  Do(R"json("My String")json", [](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream),
                IsOkAndHolds(ValueIs<std::string>("My String")));
  });
}

TEST(LexerTest, UTFBoundaries) {
  Do(R"json("\u0001\u07FF\uFFFF\uDBFF\uDFFF")json",
     [](io::ZeroCopyInputStream* stream) {
       EXPECT_THAT(Value::Parse(stream),
                   IsOkAndHolds(ValueIs<std::string>(
                       "\x01\xdf\xbf\xef\xbf\xbf\xf4\x8f\xbf\xbf")));
     });
}

TEST(NonStandard, SingleQuoteString) {
  DoLegacy(R"json('My String')json", [=](const Value& value) {
    EXPECT_THAT(value, ValueIs<std::string>("My String"));
  });
}

TEST(NonStandard, ControlCharsInString) {
  DoLegacy("\"\1\2\3\4\5\6\7\b\n\f\r\"", [=](const Value& value) {
    EXPECT_THAT(value, ValueIs<std::string>("\1\2\3\4\5\6\7\b\n\f\r"));
  });
}

TEST(LexerTest, Latin) {
  Do(R"json("Pokémon")json", [](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream),
                IsOkAndHolds(ValueIs<std::string>("Pokémon")));
  });
}

TEST(LexerTest, Cjk) {
  Do(R"json("施氏食獅史")json", [](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream),
                IsOkAndHolds(ValueIs<std::string>("施氏食獅史")));
  });
}

TEST(LexerTest, BrokenString) {
  Bad(R"json("broken)json");
  Bad(R"json("broken')json");
  Bad(R"json("broken\")json");
}

TEST(NonStandard, BrokenString) {
  Bad(R"json('broken)json");
  Bad(R"json('broken")json");
}

TEST(LexerTest, BrokenEscape) {
  Bad(R"json("\)json");
  Bad(R"json("\a")json");
  Bad(R"json("\u")json");
  Bad(R"json("\u123")json");
  Bad(R"json("\u{1f36f}")json");
  Bad(R"json("\u123$$$")json");
  Bad(R"json("\ud800\udcfg")json");
}

void GoodNumber(absl::string_view json, double value) {
  Do(json, [value](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream), IsOkAndHolds(ValueIs<double>(value)));
  });
}

TEST(LexerTest, Zero) {
  GoodNumber("0", 0);
  GoodNumber("0.0", 0);
  GoodNumber("0.000", 0);
  GoodNumber("-0", -0.0);
  GoodNumber("-0.0", -0.0);

  Bad("00");
  Bad("-00");
}

TEST(LexerTest, Integer) {
  GoodNumber("123456", 123456);
  GoodNumber("-79497823553162768", -79497823553162768);
  GoodNumber("11779497823553163264", 11779497823553163264u);

  Bad("0777");
}

TEST(LexerTest, Overflow) {
  GoodNumber("18446744073709551616", 18446744073709552000.0);
  GoodNumber("-18446744073709551616", -18446744073709551616.0);

  Bad("1.89769e308");
  Bad("-1.89769e308");
}

TEST(LexerTest, Double) {
  GoodNumber("42.5", 42.5);
  GoodNumber("42.50", 42.50);
  GoodNumber("-1045.235", -1045.235);
  GoodNumber("-0.235", -0.235);

  Bad("42.");
  Bad("01.3");
  Bad(".5");
  Bad("-.5");
}

TEST(LexerTest, Scientific) {
  GoodNumber("1.2345e+10", 1.2345e+10);
  GoodNumber("1.2345e-10", 1.2345e-10);
  GoodNumber("1.2345e10", 1.2345e10);
  GoodNumber("1.2345E+10", 1.2345e+10);
  GoodNumber("1.2345E-10", 1.2345e-10);
  GoodNumber("1.2345E10", 1.2345e10);
  GoodNumber("0e0", 0);
  GoodNumber("9E9", 9e9);

  Bad("1.e5");
  Bad("-e5");
  Bad("1e");
  Bad("1e-");
  Bad("1e+");
}

TEST(LexerTest, EmptyArray) {
  Do("[]", [](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream),
                IsOkAndHolds(ValueIs<Value::Array>(IsEmpty())));
  });
}

TEST(LexerTest, PrimitiveArray) {
  absl::string_view json = R"json(
    [true, false, null, "string"]
  )json";
  Do(json, [](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream),
                IsOkAndHolds(ValueIs<Value::Array>(ElementsAre(
                    ValueIs<bool>(true),            //
                    ValueIs<bool>(false),           //
                    ValueIs<Value::Null>(_),        //
                    ValueIs<std::string>("string")  //
                    ))));
  });
}

TEST(LexerTest, BrokenArray) {
  Bad("[");
  Bad("[[");
  Bad("[true, null}");
}

TEST(LexerTest, BrokenStringInArray) { Bad(R"json(["Unterminated])json"); }

TEST(LexerTest, NestedArray) {
  absl::string_view json = R"json(
    [
      [22, -127, 45.3, -1056.4, 11779497823553162765],
      {"key": true}
    ]
  )json";
  Do(json, [](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream),
                IsOkAndHolds(ValueIs<Value::Array>(ElementsAre(
                    ValueIs<Value::Array>(ElementsAre(
                        ValueIs<double>(22),                    //
                        ValueIs<double>(-127),                  //
                        ValueIs<double>(45.3),                  //
                        ValueIs<double>(-1056.4),               //
                        ValueIs<double>(11779497823553162765u)  //
                        )),
                    ValueIs<Value::Object>(
                        ElementsAre(Pair("key", ValueIs<bool>(true))))))));
  });
}

TEST(LexerTest, EmptyObject) {
  Do("{}", [](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream),
                IsOkAndHolds(ValueIs<Value::Object>(IsEmpty())));
  });
}

TEST(LexerTest, BrokenObject) {
  Bad("{");
  Bad("{{");
  Bad(R"json({"key": true])json");
  Bad(R"json({"key")json");
  Bad(R"json({"key":})json");
}

TEST(LexerTest, BrokenStringInObject) {
  Bad(R"json({"oops": "Unterminated})json");
}

TEST(LexerTest, NonPairInObject) {
  Bad("{null}");
  Bad("{true}");
  Bad("{false}");
  Bad("{42}");
  Bad("{[null]}");
  Bad(R"json({{"nest_pas": true}})json");
  Bad(R"json({"missing colon"})json");
}

TEST(NonStandard, NonPairInObject) {
  Bad("{'missing colon'}");
  Bad("{missing_colon}");
}

TEST(LexerTest, WrongCommas) {
  Bad("[null null]");
  Bad("[null,, null]");
  Bad(R"json({"a": 0 "b": true})json");
  Bad(R"json({"a": 0,, "b": true})json");
}

TEST(NonStandard, Keys) {
  DoLegacy(R"json({'s': true})json", [](const Value& value) {
    EXPECT_THAT(value, ValueIs<Value::Object>(
                           ElementsAre(Pair("s", ValueIs<bool>(true)))));
  });
  DoLegacy(R"json({key: null})json", [](const Value& value) {
    EXPECT_THAT(value, ValueIs<Value::Object>(
                           ElementsAre(Pair("key", ValueIs<Value::Null>(_)))));
  });
  DoLegacy(R"json({snake_key: []})json", [](const Value& value) {
    EXPECT_THAT(value, ValueIs<Value::Object>(ElementsAre(Pair(
                           "snake_key", ValueIs<Value::Array>(IsEmpty())))));
  });
  DoLegacy(R"json({camelKey: {}})json", [](const Value& value) {
    EXPECT_THAT(value, ValueIs<Value::Object>(ElementsAre(Pair(
                           "camelKey", ValueIs<Value::Object>(IsEmpty())))));
  });
}

TEST(NonStandard, KeywordPrefixedKeys) {
  DoLegacy(R"json({nullkey: "a"})json", [](const Value& value) {
    EXPECT_THAT(value, ValueIs<Value::Object>(ElementsAre(
                           Pair("nullkey", ValueIs<std::string>("a")))));
  });
  DoLegacy(R"json({truekey: "b"})json", [](const Value& value) {
    EXPECT_THAT(value, ValueIs<Value::Object>(ElementsAre(
                           Pair("truekey", ValueIs<std::string>("b")))));
  });
  DoLegacy(R"json({falsekey: "c"})json", [](const Value& value) {
    EXPECT_THAT(value, ValueIs<Value::Object>(ElementsAre(
                           Pair("falsekey", ValueIs<std::string>("c")))));
  });
}

TEST(LexerTest, BadKeys) {
  Bad("{null: 0}");
  Bad("{true: 0}");
  Bad("{false: 0}");
  Bad("{lisp-kebab: 0}");
  Bad("{42: true}");
}

TEST(LexerTest, NestedObject) {
  absl::string_view json = R"json(
    {
      "t": true,
      "f": false,
      "n": null,
      "s": "a string",
      "pi": 22,
      "ni": -127,
      "pd": 45.3,
      "nd": -1056.4,
      "pl": 11779497823553162765,
      "l": [ [ ] ],
      "o": { "key": true }
    }
  )json";
  Do(json, [](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream),
                IsOkAndHolds(ValueIs<Value::Object>(ElementsAre(
                    Pair("t", ValueIs<bool>(true)),                      //
                    Pair("f", ValueIs<bool>(false)),                     //
                    Pair("n", ValueIs<Value::Null>(_)),                  //
                    Pair("s", ValueIs<std::string>("a string")),         //
                    Pair("pi", ValueIs<double>(22)),                     //
                    Pair("ni", ValueIs<double>(-127)),                   //
                    Pair("pd", ValueIs<double>(45.3)),                   //
                    Pair("nd", ValueIs<double>(-1056.4)),                //
                    Pair("pl", ValueIs<double>(11779497823553162765u)),  //
                    Pair("l", ValueIs<Value::Array>(ElementsAre(
                                  ValueIs<Value::Array>(IsEmpty())))),  //
                    Pair("o", ValueIs<Value::Object>(ElementsAre(
                                  Pair("key", ValueIs<bool>(true)))))  //
                    ))));
  });
}

TEST(LexerTest, RejectNonUtf8) {
  absl::string_view json = R"json(
    { "address": x"施氏食獅史" }
  )json";
  Bad(absl::StrReplaceAll(json, {{"x", "\xff"}}));
}

TEST(LexerTest, RejectNonUtf8String) {
  absl::string_view json = R"json(
    { "address": "施氏x食獅史" }
  )json";
  Bad(absl::StrReplaceAll(json, {{"x", "\xff"}}));
}

TEST(LexerTest, RejectNonUtf8Prefix) { Bad("\xff{}"); }

TEST(LexerTest, RejectOverlongUtf8) {
  // This is the NUL character (U+0000) encoded in three bytes instead of one.
  // Such "overlong" encodings are not considered valid.
  Bad("\"\340\200\200\"");
}

TEST(LexerTest, MixtureOfEscapesAndRawMultibyteCharacters) {
  Do(R"json("😁\t")json", [](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream),
                IsOkAndHolds(ValueIs<std::string>("😁\t")));
  });
  Do(R"json("\t😁")json", [](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream),
                IsOkAndHolds(ValueIs<std::string>("\t😁")));
  });
}

TEST(LexerTest, SurrogateEscape) {
  absl::string_view json = R"json(
    [ "\ud83d\udc08\u200D\u2b1B\ud83d\uDdA4" ]
  )json";
  Do(json, [](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream),
                IsOkAndHolds(ValueIs<Value::Array>(
                    ElementsAre(ValueIs<std::string>("🐈‍⬛🖤")))));
  });
}

TEST(LexerTest, InvalidCodePoint) { Bad(R"json(["\ude36"])json"); }

TEST(LexerTest, LonelyHighSurrogate) {
  Bad(R"json(["\ud83d"])json");
  Bad(R"json(["\ud83d|trailing"])json");
  Bad(R"json(["\ud83d\ude--"])json");
  Bad(R"json(["\ud83d\ud83d"])json");
}

TEST(LexerTest, AsciiEscape) {
  absl::string_view json = R"json(
    ["\b", "\ning", "test\f", "\r\t", "test\\\"\/ing"]
  )json";
  Do(json, [](io::ZeroCopyInputStream* stream) {
    EXPECT_THAT(Value::Parse(stream),
                IsOkAndHolds(ValueIs<Value::Array>(ElementsAre(
                    ValueIs<std::string>("\b"),           //
                    ValueIs<std::string>("\ning"),        //
                    ValueIs<std::string>("test\f"),       //
                    ValueIs<std::string>("\r\t"),         //
                    ValueIs<std::string>("test\\\"/ing")  //
                    ))));
  });
}

TEST(NonStandard, AsciiEscape) {
  DoLegacy(R"json(["\'", '\''])json", [](const Value& value) {
    EXPECT_THAT(value,
                ValueIs<Value::Array>(ElementsAre(ValueIs<std::string>("'"),  //
                                                  ValueIs<std::string>("'")   //
                                                  )));
  });
}

TEST(NonStandard, TrailingCommas) {
  DoLegacy(R"json({"foo": 42,})json", [](const Value& value) {
    EXPECT_THAT(value, ValueIs<Value::Object>(
                           ElementsAre(Pair("foo", ValueIs<double>(42)))));
  });
  DoLegacy(R"json({"foo": [42,],})json", [](const Value& value) {
    EXPECT_THAT(
        value,
        ValueIs<Value::Object>(ElementsAre(Pair(
            "foo", ValueIs<Value::Array>(ElementsAre(ValueIs<double>(42)))))));
  });
  DoLegacy(R"json([42,])json", [](const Value& value) {
    EXPECT_THAT(value, ValueIs<Value::Array>(ElementsAre(ValueIs<double>(42))));
  });
  DoLegacy(R"json([{},])json", [](const Value& value) {
    EXPECT_THAT(value, ValueIs<Value::Array>(
                           ElementsAre(ValueIs<Value::Object>(IsEmpty()))));
  });
}

// These strings are enormous; so that the test actually finishes in a
// reasonable time, we skip using Do().

TEST(LexerTest, ArrayRecursion) {
  std::string ok = std::string(ParseOptions::kDefaultDepth, '[') +
                   std::string(ParseOptions::kDefaultDepth, ']');

  {
    io::ArrayInputStream stream(ok.data(), static_cast<int>(ok.size()));
    auto value = Value::Parse(&stream);
    ASSERT_OK(value);

    Value* v = &*value;
    for (int i = 0; i < ParseOptions::kDefaultDepth - 1; ++i) {
      ASSERT_THAT(*v, ValueIs<Value::Array>(SizeIs(1)));
      v = &std::get<Value::Array>(v->value)[0];
    }
    ASSERT_THAT(*v, ValueIs<Value::Array>(IsEmpty()));
  }

  {
    std::string evil = absl::StrFormat("[%s]", ok);
    io::ArrayInputStream stream(evil.data(), static_cast<int>(evil.size()));
    ASSERT_THAT(Value::Parse(&stream),
                StatusIs(absl::StatusCode::kInvalidArgument));
  }
}

TEST(LexerTest, ObjectRecursion) {
  std::string ok;
  for (int i = 0; i < ParseOptions::kDefaultDepth - 1; ++i) {
    absl::StrAppend(&ok, "{\"k\":");
  }
  absl::StrAppend(&ok, "{");
  ok += std::string(ParseOptions::kDefaultDepth, '}');

  {
    io::ArrayInputStream stream(ok.data(), static_cast<int>(ok.size()));
    auto value = Value::Parse(&stream);
    ASSERT_OK(value);

    Value* v = &*value;
    for (int i = 0; i < ParseOptions::kDefaultDepth - 1; ++i) {
      ASSERT_THAT(*v, ValueIs<Value::Object>(ElementsAre(Pair("k", _))));
      v = &std::get<Value::Object>(v->value)[0].second;
    }
    ASSERT_THAT(*v, ValueIs<Value::Object>(IsEmpty()));
  }
  {
    std::string evil = absl::StrFormat("{\"k\":%s}", ok);
    io::ArrayInputStream stream(evil.data(), static_cast<int>(evil.size()));
    ASSERT_THAT(Value::Parse(&stream),
                StatusIs(absl::StatusCode::kInvalidArgument));
  }
}
}  // namespace
}  // namespace json_internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
