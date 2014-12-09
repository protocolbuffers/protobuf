/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2014 Google Inc.  See LICENSE for details.
 *
 * A set of tests for JSON parsing and serialization.
 */

#include "tests/upb_test.h"
#include "upb/handlers.h"
#include "upb/symtab.h"
#include "upb/json/printer.h"
#include "upb/json/parser.h"
#include "upb/upb.h"

#include <string>

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

static TestCase kTestRoundtripMessages[] = {
  // Test most fields here.
  {
    TEST("{\"optional_int32\":-42,\"optional_string\":\"Test\\u0001Message\","
         "\"optional_msg\":{\"foo\":42},"
         "\"optional_bool\":true,\"repeated_msg\":[{\"foo\":1},"
         "{\"foo\":2}]}"),
    EXPECT_SAME
  },
  // Test special escapes in strings.
  {
    TEST("{\"repeated_string\":[\"\\b\",\"\\r\",\"\\n\",\"\\f\",\"\\t\","
         "\"\uFFFF\"]}"),
    EXPECT_SAME
  },
  // Test enum symbolic names.
  {
    // The common case: parse and print the symbolic name.
    TEST("{\"optional_enum\":\"A\"}"),
    EXPECT_SAME
  },
  {
    // Unknown enum value: will be printed as an integer.
    TEST("{\"optional_enum\":42}"),
    EXPECT_SAME
  },
  {
    // Known enum value: we're happy to parse an integer but we will re-emit the
    // symbolic name.
    TEST("{\"optional_enum\":1}"),
    EXPECT("{\"optional_enum\":\"B\"}")
  },
  // UTF-8 tests: escapes -> literal UTF8 in output.
  {
    // Note double escape on \uXXXX: we want the escape to be processed by the
    // JSON parser, not by the C++ compiler!
    TEST("{\"optional_string\":\"\\u007F\"}"),
    EXPECT("{\"optional_string\":\"\x7F\"}")
  },
  {
    TEST("{\"optional_string\":\"\\u0080\"}"),
    EXPECT("{\"optional_string\":\"\xC2\x80\"}")
  },
  {
    TEST("{\"optional_string\":\"\\u07FF\"}"),
    EXPECT("{\"optional_string\":\"\xDF\xBF\"}")
  },
  {
    TEST("{\"optional_string\":\"\\u0800\"}"),
    EXPECT("{\"optional_string\":\"\xE0\xA0\x80\"}")
  },
  {
    TEST("{\"optional_string\":\"\\uFFFF\"}"),
    EXPECT("{\"optional_string\":\"\xEF\xBF\xBF\"}")
  },
  TEST_SENTINEL
};

static void AddField(upb::MessageDef* message,
                     int number,
                     const char* name,
                     upb_fieldtype_t type,
                     bool is_repeated,
                     const upb::Def* subdef = NULL) {
  upb::reffed_ptr<upb::FieldDef> field(upb::FieldDef::New());
  upb::Status st;
  field->set_name(name, &st);
  field->set_type(type);
  field->set_label(is_repeated ? UPB_LABEL_REPEATED : UPB_LABEL_OPTIONAL);
  field->set_number(number, &st);
  if (subdef) {
    field->set_subdef(subdef, &st);
  }
  message->AddField(field, &st);
}

static const upb::MessageDef* BuildTestMessage(
    upb::reffed_ptr<upb::SymbolTable> symtab) {
  upb::Status st;

  // Create SubMessage.
  upb::reffed_ptr<upb::MessageDef> submsg(upb::MessageDef::New());
  submsg->set_full_name("SubMessage", &st);
  AddField(submsg.get(), 1, "foo", UPB_TYPE_INT32, false);

  // Create MyEnum.
  upb::reffed_ptr<upb::EnumDef> myenum(upb::EnumDef::New());
  myenum->set_full_name("MyEnum", &st);
  myenum->AddValue("A", 0, &st);
  myenum->AddValue("B", 1, &st);
  myenum->AddValue("C", 2, &st);

  // Create TestMessage.
  upb::reffed_ptr<upb::MessageDef> md(upb::MessageDef::New());
  md->set_full_name("TestMessage", &st);

  AddField(md.get(), 1, "optional_int32",  UPB_TYPE_INT32, false);
  AddField(md.get(), 2, "optional_int64",  UPB_TYPE_INT64, false);
  AddField(md.get(), 3, "optional_uint32", UPB_TYPE_UINT32, false);
  AddField(md.get(), 4, "optional_uint64", UPB_TYPE_UINT64, false);
  AddField(md.get(), 5, "optional_string", UPB_TYPE_STRING, false);
  AddField(md.get(), 6, "optional_bytes",  UPB_TYPE_BYTES, false);
  AddField(md.get(), 7, "optional_bool" ,  UPB_TYPE_BOOL, false);
  AddField(md.get(), 8, "optional_msg" ,   UPB_TYPE_MESSAGE, false,
           upb::upcast(submsg.get()));
  AddField(md.get(), 9, "optional_enum",   UPB_TYPE_ENUM, false,
           upb::upcast(myenum.get()));

  AddField(md.get(), 11, "repeated_int32",  UPB_TYPE_INT32, true);
  AddField(md.get(), 12, "repeated_int64",  UPB_TYPE_INT64, true);
  AddField(md.get(), 13, "repeated_uint32", UPB_TYPE_UINT32, true);
  AddField(md.get(), 14, "repeated_uint64", UPB_TYPE_UINT64, true);
  AddField(md.get(), 15, "repeated_string", UPB_TYPE_STRING, true);
  AddField(md.get(), 16, "repeated_bytes",  UPB_TYPE_BYTES, true);
  AddField(md.get(), 17, "repeated_bool" ,  UPB_TYPE_BOOL, true);
  AddField(md.get(), 18, "repeated_msg" ,   UPB_TYPE_MESSAGE, true,
           upb::upcast(submsg.get()));
  AddField(md.get(), 19, "optional_enum",   UPB_TYPE_ENUM, true,
           upb::upcast(myenum.get()));

  // Add both to our symtab.
  upb::Def* defs[3] = {
    upb::upcast(submsg.ReleaseTo(&defs)),
    upb::upcast(myenum.ReleaseTo(&defs)),
    upb::upcast(md.ReleaseTo(&defs)),
  };
  symtab->Add(defs, 3, &defs, &st);

  // Return TestMessage.
  return symtab->LookupMessage("TestMessage");
}

class StringSink {
 public:
  StringSink() {
    upb_byteshandler_init(&byteshandler_);
    upb_byteshandler_setstring(&byteshandler_, &str_handler, NULL);
    upb_bytessink_reset(&bytessink_, &byteshandler_, &s_);
  }
  ~StringSink() { }

  upb_bytessink* Sink() { return &bytessink_; }

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

// Starts with a message in JSON format, parses and directly serializes again,
// and compares the result.
void test_json_roundtrip() {
  upb::reffed_ptr<upb::SymbolTable> symtab(upb::SymbolTable::New());
  const upb::MessageDef* md = BuildTestMessage(symtab.get());
  upb::reffed_ptr<const upb::Handlers> serialize_handlers(
      upb::json::Printer::NewHandlers(md));

  for (const TestCase* test_case = kTestRoundtripMessages;
       test_case->input != NULL; test_case++) {

    const char *json_src = test_case->input;
    const char *json_expected = test_case->expected;
    if (json_expected == EXPECT_SAME) {
      json_expected = json_src;
    }

    upb::Status st;
    upb::json::Parser parser(&st);
    upb::json::Printer printer(serialize_handlers.get());
    StringSink data_sink;

    parser.ResetOutput(printer.input());
    printer.ResetOutput(data_sink.Sink());

    bool ok = upb::BufferSource::PutBuffer(json_src, strlen(json_src),
                                           parser.input());
    if (!ok) {
      fprintf(stderr, "upb parse error: %s\n", st.error_message());
    }
    ASSERT(ok);

    if (memcmp(json_expected,
               data_sink.Data().data(),
               data_sink.Data().size())) {
      fprintf(stderr,
              "JSON parse/serialize roundtrip result differs:\n"
              "Original:\n%s\nParsed/Serialized:\n%s\n",
              json_src, data_sink.Data().c_str());
      abort();
    }
  }
}

extern "C" {
int run_tests(int argc, char *argv[]) {
  UPB_UNUSED(argc);
  UPB_UNUSED(argv);
  test_json_roundtrip();
  return 0;
}
}
