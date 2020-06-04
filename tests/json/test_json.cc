/*
 *
 * A set of tests for JSON parsing and serialization.
 */

#include <string>

#include "tests/json/test.upb.h"  // Test that it compiles for C++.
#include "tests/json/test.upbdefs.h"
#include "tests/test_util.h"
#include "tests/upb_test.h"
#include "upb/def.hpp"
#include "upb/handlers.h"
#include "upb/json/parser.h"
#include "upb/json/printer.h"
#include "upb/port_def.inc"
#include "upb/upb.h"

// Macros for readability in test case list: allows us to give TEST("...") /
// EXPECT("...") pairs.
#define TEST(x)     x
#define EXPECT_SAME NULL
#define EXPECT(x)   x
#define TEST_SENTINEL { NULL, NULL }

struct TestCase {
  const char* input;
  const char* expected;
};

bool verbose = false;

static TestCase kTestRoundtripMessages[] = {
  // Test most fields here.
  {
    TEST("{\"optionalInt32\":-42,\"optionalString\":\"Test\\u0001Message\","
         "\"optionalMsg\":{\"foo\":42},"
         "\"optionalBool\":true,\"repeatedMsg\":[{\"foo\":1},"
         "{\"foo\":2}]}"),
    EXPECT_SAME
  },
  // We must also recognize raw proto names.
  {
    TEST("{\"optional_int32\":-42,\"optional_string\":\"Test\\u0001Message\","
         "\"optional_msg\":{\"foo\":42},"
         "\"optional_bool\":true,\"repeated_msg\":[{\"foo\":1},"
         "{\"foo\":2}]}"),
    EXPECT("{\"optionalInt32\":-42,\"optionalString\":\"Test\\u0001Message\","
           "\"optionalMsg\":{\"foo\":42},"
           "\"optionalBool\":true,\"repeatedMsg\":[{\"foo\":1},"
           "{\"foo\":2}]}")
  },
  // Test special escapes in strings.
  {
    TEST("{\"repeatedString\":[\"\\b\",\"\\r\",\"\\n\",\"\\f\",\"\\t\","
         "\"\uFFFF\"]}"),
    EXPECT_SAME
  },
  // Test enum symbolic names.
  {
    // The common case: parse and print the symbolic name.
    TEST("{\"optionalEnum\":\"A\"}"),
    EXPECT_SAME
  },
  {
    // Unknown enum value: will be printed as an integer.
    TEST("{\"optionalEnum\":42}"),
    EXPECT_SAME
  },
  {
    // Known enum value: we're happy to parse an integer but we will re-emit the
    // symbolic name.
    TEST("{\"optionalEnum\":1}"),
    EXPECT("{\"optionalEnum\":\"B\"}")
  },
  // UTF-8 tests: escapes -> literal UTF8 in output.
  {
    // Note double escape on \uXXXX: we want the escape to be processed by the
    // JSON parser, not by the C++ compiler!
    TEST("{\"optionalString\":\"\\u007F\"}"),
    EXPECT("{\"optionalString\":\"\x7F\"}")
  },
  {
    TEST("{\"optionalString\":\"\\u0080\"}"),
    EXPECT("{\"optionalString\":\"\xC2\x80\"}")
  },
  {
    TEST("{\"optionalString\":\"\\u07FF\"}"),
    EXPECT("{\"optionalString\":\"\xDF\xBF\"}")
  },
  {
    TEST("{\"optionalString\":\"\\u0800\"}"),
    EXPECT("{\"optionalString\":\"\xE0\xA0\x80\"}")
  },
  {
    TEST("{\"optionalString\":\"\\uFFFF\"}"),
    EXPECT("{\"optionalString\":\"\xEF\xBF\xBF\"}")
  },
  // map-field tests
  {
    TEST("{\"mapStringString\":{\"a\":\"value1\",\"b\":\"value2\","
         "\"c\":\"value3\"}}"),
    EXPECT_SAME
  },
  {
    TEST("{\"mapInt32String\":{\"1\":\"value1\",\"-1\":\"value2\","
         "\"1234\":\"value3\"}}"),
    EXPECT_SAME
  },
  {
    TEST("{\"mapBoolString\":{\"false\":\"value1\",\"true\":\"value2\"}}"),
    EXPECT_SAME
  },
  {
    TEST("{\"mapStringInt32\":{\"asdf\":1234,\"jkl;\":-1}}"),
    EXPECT_SAME
  },
  {
    TEST("{\"mapStringBool\":{\"asdf\":true,\"jkl;\":false}}"),
    EXPECT_SAME
  },
  {
    TEST("{\"mapStringMsg\":{\"asdf\":{\"foo\":42},\"jkl;\":{\"foo\":84}}}"),
    EXPECT_SAME
  },
  TEST_SENTINEL
};

static TestCase kTestRoundtripMessagesPreserve[] = {
  // Test most fields here.
  {
    TEST("{\"optional_int32\":-42,\"optional_string\":\"Test\\u0001Message\","
         "\"optional_msg\":{\"foo\":42},"
         "\"optional_bool\":true,\"repeated_msg\":[{\"foo\":1},"
         "{\"foo\":2}]}"),
    EXPECT_SAME
  },
  TEST_SENTINEL
};

static TestCase kTestSkipUnknown[] = {
  {
    TEST("{\"optionalEnum\":\"UNKNOWN_ENUM_VALUE\"}"),
    EXPECT("{}"),
  },
  TEST_SENTINEL
};

static TestCase kTestFailure[] = {
  {
    TEST("{\"optionalEnum\":\"UNKNOWN_ENUM_VALUE\"}"),
    EXPECT("{}"),  /* Actually we expect error, this is checked later. */
  },
  TEST_SENTINEL
};

class StringSink {
 public:
  StringSink() {
    upb_byteshandler_init(&byteshandler_);
    upb_byteshandler_setstring(&byteshandler_, &str_handler, NULL);
    upb_bytessink_reset(&bytessink_, &byteshandler_, &s_);
  }
  ~StringSink() { }

  upb_bytessink Sink() { return bytessink_; }

  const std::string& Data() { return s_; }

 private:

  static size_t str_handler(void* _closure, const void* hd,
                            const char* data, size_t len,
                            const upb_bufhandle* handle) {
    UPB_UNUSED(hd);
    UPB_UNUSED(handle);
    std::string* s = static_cast<std::string*>(_closure);
    std::string appended(data, len);
    s->append(data, len);
    return len;
  }

  upb_byteshandler byteshandler_;
  upb_bytessink bytessink_;
  std::string s_;
};

void test_json_roundtrip_message(const char* json_src,
                                 const char* json_expected,
                                 const upb::Handlers* serialize_handlers,
                                 const upb::json::ParserMethodPtr parser_method,
                                 int seam,
                                 bool ignore_unknown) {
  VerboseParserEnvironment env(verbose);
  StringSink data_sink;
  upb::json::PrinterPtr printer = upb::json::PrinterPtr::Create(
      env.arena(), serialize_handlers, data_sink.Sink());
  upb::json::ParserPtr parser = upb::json::ParserPtr::Create(
      env.arena(), parser_method, NULL, printer.input(),
      env.status(), ignore_unknown);
  env.ResetBytesSink(parser.input());
  env.Reset(json_src, strlen(json_src), false, false);

  bool ok = env.Start() &&
            env.ParseBuffer(seam) &&
            env.ParseBuffer(-1) &&
            env.End();

  ASSERT(ok);
  ASSERT(env.CheckConsistency());

  if (memcmp(json_expected,
             data_sink.Data().data(),
             data_sink.Data().size())) {
    fprintf(stderr,
            "JSON parse/serialize roundtrip result differs:\n"
            "Expected:\n%s\nParsed/Serialized:\n%s\n",
            json_expected, data_sink.Data().c_str());
    abort();
  }
}

// Starts with a message in JSON format, parses and directly serializes again,
// and compares the result.
void test_json_roundtrip() {
  upb::SymbolTable symtab;
  upb::HandlerCache serialize_handlercache(
      upb::json::PrinterPtr::NewCache(false));
  upb::json::CodeCache parse_codecache;

  upb::MessageDefPtr md(upb_test_json_TestMessage_getmsgdef(symtab.ptr()));
  ASSERT(md);
  const upb::Handlers* serialize_handlers = serialize_handlercache.Get(md);
  const upb::json::ParserMethodPtr parser_method = parse_codecache.Get(md);
  ASSERT(serialize_handlers);

  for (const TestCase* test_case = kTestRoundtripMessages;
       test_case->input != NULL; test_case++) {
    const char *expected =
        (test_case->expected == EXPECT_SAME) ?
        test_case->input :
        test_case->expected;

    for (size_t i = 0; i < strlen(test_case->input); i++) {
      test_json_roundtrip_message(test_case->input, expected,
                                  serialize_handlers, parser_method, (int)i,
                                  false);
    }
  }

  // Tests ignore unknown.
  for (const TestCase* test_case = kTestSkipUnknown;
       test_case->input != NULL; test_case++) {
    const char *expected =
        (test_case->expected == EXPECT_SAME) ?
        test_case->input :
        test_case->expected;

    for (size_t i = 0; i < strlen(test_case->input); i++) {
      test_json_roundtrip_message(test_case->input, expected,
                                  serialize_handlers, parser_method, (int)i,
                                  true);
    }
  }

  serialize_handlercache = upb::json::PrinterPtr::NewCache(true);
  serialize_handlers = serialize_handlercache.Get(md);

  for (const TestCase* test_case = kTestRoundtripMessagesPreserve;
       test_case->input != NULL; test_case++) {
    const char *expected =
        (test_case->expected == EXPECT_SAME) ?
        test_case->input :
        test_case->expected;

    for (size_t i = 0; i < strlen(test_case->input); i++) {
      test_json_roundtrip_message(test_case->input, expected,
                                  serialize_handlers, parser_method, (int)i,
                                  false);
    }
  }
}

void test_json_parse_failure(const char* json_src,
                             const upb::Handlers* serialize_handlers,
                             const upb::json::ParserMethodPtr parser_method,
                             int seam) {
  VerboseParserEnvironment env(verbose);
  StringSink data_sink;
  upb::json::PrinterPtr printer = upb::json::PrinterPtr::Create(
      env.arena(), serialize_handlers, data_sink.Sink());
  upb::json::ParserPtr parser = upb::json::ParserPtr::Create(
      env.arena(), parser_method, NULL, printer.input(), env.status(), false);
  env.ResetBytesSink(parser.input());
  env.Reset(json_src, strlen(json_src), false, true);

  bool ok = env.Start() &&
            env.ParseBuffer(seam) &&
            env.ParseBuffer(-1) &&
            env.End();

  ASSERT(!ok);
  ASSERT(env.CheckConsistency());
}

// Starts with a proto message in JSON format, parses and expects failre.
void test_json_failure() {
  upb::SymbolTable symtab;
  upb::HandlerCache serialize_handlercache(
      upb::json::PrinterPtr::NewCache(false));
  upb::json::CodeCache parse_codecache;

  upb::MessageDefPtr md(upb_test_json_TestMessage_getmsgdef(symtab.ptr()));
  ASSERT(md);
  const upb::Handlers* serialize_handlers = serialize_handlercache.Get(md);
  const upb::json::ParserMethodPtr parser_method = parse_codecache.Get(md);
  ASSERT(serialize_handlers);

  for (const TestCase* test_case = kTestFailure;
       test_case->input != NULL; test_case++) {
    for (size_t i = 0; i < strlen(test_case->input); i++) {
      test_json_parse_failure(test_case->input, serialize_handlers,
                              parser_method, (int)i);
    }
  }
}

extern "C" {
int run_tests(int argc, char *argv[]) {
  UPB_UNUSED(argc);
  UPB_UNUSED(argv);
  test_json_roundtrip();
  test_json_failure();
  return 0;
}
}
