// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// To test the code generator, we actually use it to generate code for
// google/protobuf/unittest.proto, then test that.  This means that we
// are actually testing the parser and other parts of the system at the same
// time, and that problems in the generator may show up as compile-time errors
// rather than unittest failures, which may be surprising.  However, testing
// the output of the C++ generator directly would be very hard.  We can't very
// well just check it against golden files since those files would have to be
// updated for any small change; such a test would be very brittle and probably
// not very helpful.  What we really want to test is that the code compiles
// correctly and produces the interfaces we expect, which is why this test
// is written this way.

#include <vector>

#include <google/protobuf/unittest.pb.h>
#include <google/protobuf/unittest_optimize_for.pb.h>
#include <google/protobuf/unittest_embed_optimize_for.pb.h>
#include <google/protobuf/test_util.h>
#include <google/protobuf/compiler/cpp/cpp_test_bad_identifiers.pb.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {


class MockErrorCollector : public MultiFileErrorCollector {
 public:
  MockErrorCollector() {}
  ~MockErrorCollector() {}

  string text_;

  // implements ErrorCollector ---------------------------------------
  void AddError(const string& filename, int line, int column,
                const string& message) {
    strings::SubstituteAndAppend(&text_, "$0:$1:$2: $3\n",
                                 filename, line, column, message);
  }
};

// Test that generated code has proper descriptors:
// Parse a descriptor directly (using google::protobuf::compiler::Importer) and
// compare it to the one that was produced by generated code.
TEST(GeneratedDescriptorTest, IdenticalDescriptors) {
  const FileDescriptor* generated_descriptor =
    unittest::TestAllTypes::descriptor()->file();

  // Set up the Importer.
  MockErrorCollector error_collector;
  DiskSourceTree source_tree;
  source_tree.MapPath("", TestSourceDir());
  Importer importer(&source_tree, &error_collector);

  // Import (parse) unittest.proto.
  const FileDescriptor* parsed_descriptor =
    importer.Import("google/protobuf/unittest.proto");
  EXPECT_EQ("", error_collector.text_);
  ASSERT_TRUE(parsed_descriptor != NULL);

  // Test that descriptors are generated correctly by converting them to
  // FileDescriptorProtos and comparing.
  FileDescriptorProto generated_decsriptor_proto, parsed_descriptor_proto;
  generated_descriptor->CopyTo(&generated_decsriptor_proto);
  parsed_descriptor->CopyTo(&parsed_descriptor_proto);

  EXPECT_EQ(parsed_descriptor_proto.DebugString(),
            generated_decsriptor_proto.DebugString());
}

// ===================================================================

TEST(GeneratedMessageTest, Defaults) {
  // Check that all default values are set correctly in the initial message.
  unittest::TestAllTypes message;

  TestUtil::ExpectClear(message);

  // Messages should return pointers to default instances until first use.
  // (This is not checked by ExpectClear() since it is not actually true after
  // the fields have been set and then cleared.)
  EXPECT_EQ(&unittest::TestAllTypes::OptionalGroup::default_instance(),
            &message.optionalgroup());
  EXPECT_EQ(&unittest::TestAllTypes::NestedMessage::default_instance(),
            &message.optional_nested_message());
  EXPECT_EQ(&unittest::ForeignMessage::default_instance(),
            &message.optional_foreign_message());
  EXPECT_EQ(&unittest_import::ImportMessage::default_instance(),
            &message.optional_import_message());
}

TEST(GeneratedMessageTest, Accessors) {
  // Set every field to a unique value then go back and check all those
  // values.
  unittest::TestAllTypes message;

  TestUtil::SetAllFields(&message);
  TestUtil::ExpectAllFieldsSet(message);

  TestUtil::ModifyRepeatedFields(&message);
  TestUtil::ExpectRepeatedFieldsModified(message);
}

TEST(GeneratedMessageTest, MutableStringDefault) {
  // mutable_foo() for a string should return a string initialized to its
  // default value.
  unittest::TestAllTypes message;

  EXPECT_EQ("hello", *message.mutable_default_string());

  // Note that the first time we call mutable_foo(), we get a newly-allocated
  // string, but if we clear it and call it again, we get the same object again.
  // We should verify that it has its default value in both cases.
  message.set_default_string("blah");
  message.Clear();

  EXPECT_EQ("hello", *message.mutable_default_string());
}

TEST(GeneratedMessageTest, Clear) {
  // Set every field to a unique value, clear the message, then check that
  // it is cleared.
  unittest::TestAllTypes message;

  TestUtil::SetAllFields(&message);
  message.Clear();
  TestUtil::ExpectClear(message);

  // Unlike with the defaults test, we do NOT expect that requesting embedded
  // messages will return a pointer to the default instance.  Instead, they
  // should return the objects that were created when mutable_blah() was
  // called.
  EXPECT_NE(&unittest::TestAllTypes::OptionalGroup::default_instance(),
            &message.optionalgroup());
  EXPECT_NE(&unittest::TestAllTypes::NestedMessage::default_instance(),
            &message.optional_nested_message());
  EXPECT_NE(&unittest::ForeignMessage::default_instance(),
            &message.optional_foreign_message());
  EXPECT_NE(&unittest_import::ImportMessage::default_instance(),
            &message.optional_import_message());
}

TEST(GeneratedMessageTest, EmbeddedNullsInBytesCharStar) {
  unittest::TestAllTypes message;

  const char* value = "\0lalala\0\0";
  message.set_optional_bytes(value, 9);
  ASSERT_EQ(9, message.optional_bytes().size());
  EXPECT_EQ(0, memcmp(value, message.optional_bytes().data(), 9));

  message.add_repeated_bytes(value, 9);
  ASSERT_EQ(9, message.repeated_bytes(0).size());
  EXPECT_EQ(0, memcmp(value, message.repeated_bytes(0).data(), 9));
}

TEST(GeneratedMessageTest, ClearOneField) {
  // Set every field to a unique value, then clear one value and insure that
  // only that one value is cleared.
  unittest::TestAllTypes message;

  TestUtil::SetAllFields(&message);
  int64 original_value = message.optional_int64();

  // Clear the field and make sure it shows up as cleared.
  message.clear_optional_int64();
  EXPECT_FALSE(message.has_optional_int64());
  EXPECT_EQ(0, message.optional_int64());

  // Other adjacent fields should not be cleared.
  EXPECT_TRUE(message.has_optional_int32());
  EXPECT_TRUE(message.has_optional_uint32());

  // Make sure if we set it again, then all fields are set.
  message.set_optional_int64(original_value);
  TestUtil::ExpectAllFieldsSet(message);
}


TEST(GeneratedMessageTest, CopyFrom) {
  unittest::TestAllTypes message1, message2;
  string data;

  TestUtil::SetAllFields(&message1);
  message2.CopyFrom(message1);
  TestUtil::ExpectAllFieldsSet(message2);

  // Copying from self should be a no-op.
  message2.CopyFrom(message2);
  TestUtil::ExpectAllFieldsSet(message2);
}

TEST(GeneratedMessageTest, CopyConstructor) {
  unittest::TestAllTypes message1;
  TestUtil::SetAllFields(&message1);

  unittest::TestAllTypes message2(message1);
  TestUtil::ExpectAllFieldsSet(message2);
}

TEST(GeneratedMessageTest, CopyAssignmentOperator) {
  unittest::TestAllTypes message1;
  TestUtil::SetAllFields(&message1);

  unittest::TestAllTypes message2;
  message2 = message1;
  TestUtil::ExpectAllFieldsSet(message2);

  // Make sure that self-assignment does something sane.
  message2 = message2;
  TestUtil::ExpectAllFieldsSet(message2);
}

TEST(GeneratedMessageTest, UpcastCopyFrom) {
  // Test the CopyFrom method that takes in the generic const Message&
  // parameter.
  unittest::TestAllTypes message1, message2;

  TestUtil::SetAllFields(&message1);

  const Message* source = implicit_cast<const Message*>(&message1);
  message2.CopyFrom(*source);

  TestUtil::ExpectAllFieldsSet(message2);
}

TEST(GeneratedMessageTest, DynamicMessageCopyFrom) {
  // Test copying from a DynamicMessage, which must fall back to using
  // reflection.
  unittest::TestAllTypes message2;

  // Construct a new version of the dynamic message via the factory.
  DynamicMessageFactory factory;
  scoped_ptr<Message> message1;
  message1.reset(factory.GetPrototype(
                     unittest::TestAllTypes::descriptor())->New());

  TestUtil::ReflectionTester reflection_tester(
    unittest::TestAllTypes::descriptor());
  reflection_tester.SetAllFieldsViaReflection(message1.get());

  message2.CopyFrom(*message1);

  TestUtil::ExpectAllFieldsSet(message2);
}

TEST(GeneratedMessageTest, NonEmptyMergeFrom) {
  // Test merging with a non-empty message. Code is a modified form
  // of that found in google/protobuf/reflection_ops_unittest.cc.
  unittest::TestAllTypes message1, message2;

  TestUtil::SetAllFields(&message1);

  // This field will test merging into an empty spot.
  message2.set_optional_int32(message1.optional_int32());
  message1.clear_optional_int32();

  // This tests overwriting.
  message2.set_optional_string(message1.optional_string());
  message1.set_optional_string("something else");

  // This tests concatenating.
  message2.add_repeated_int32(message1.repeated_int32(1));
  int32 i = message1.repeated_int32(0);
  message1.clear_repeated_int32();
  message1.add_repeated_int32(i);

  message1.MergeFrom(message2);

  TestUtil::ExpectAllFieldsSet(message1);
}

#ifdef GTEST_HAS_DEATH_TEST

TEST(GeneratedMessageTest, MergeFromSelf) {
  unittest::TestAllTypes message;
  EXPECT_DEATH(message.MergeFrom(message), "&from");
  EXPECT_DEATH(message.MergeFrom(implicit_cast<const Message&>(message)),
               "&from");
}

#endif  // GTEST_HAS_DEATH_TEST

TEST(GeneratedMessageTest, Serialization) {
  unittest::TestAllTypes message1, message2;
  string data;

  TestUtil::SetAllFields(&message1);
  message1.SerializeToString(&data);
  EXPECT_TRUE(message2.ParseFromString(data));
  TestUtil::ExpectAllFieldsSet(message2);

}


TEST(GeneratedMessageTest, Required) {
  // Test that IsInitialized() returns false if required fields are missing.
  unittest::TestRequired message;

  EXPECT_FALSE(message.IsInitialized());
  message.set_a(1);
  EXPECT_FALSE(message.IsInitialized());
  message.set_b(2);
  EXPECT_FALSE(message.IsInitialized());
  message.set_c(3);
  EXPECT_TRUE(message.IsInitialized());
}

TEST(GeneratedMessageTest, RequiredForeign) {
  // Test that IsInitialized() returns false if required fields in nested
  // messages are missing.
  unittest::TestRequiredForeign message;

  EXPECT_TRUE(message.IsInitialized());

  message.mutable_optional_message();
  EXPECT_FALSE(message.IsInitialized());

  message.mutable_optional_message()->set_a(1);
  message.mutable_optional_message()->set_b(2);
  message.mutable_optional_message()->set_c(3);
  EXPECT_TRUE(message.IsInitialized());

  message.add_repeated_message();
  EXPECT_FALSE(message.IsInitialized());

  message.mutable_repeated_message(0)->set_a(1);
  message.mutable_repeated_message(0)->set_b(2);
  message.mutable_repeated_message(0)->set_c(3);
  EXPECT_TRUE(message.IsInitialized());
}

TEST(GeneratedMessageTest, ForeignNested) {
  // Test that TestAllTypes::NestedMessage can be embedded directly into
  // another message.
  unittest::TestForeignNested message;

  // If this compiles and runs without crashing, it must work.  We have
  // nothing more to test.
  unittest::TestAllTypes::NestedMessage* nested =
    message.mutable_foreign_nested();
  nested->set_bb(1);
}

TEST(GeneratedMessageTest, ReallyLargeTagNumber) {
  // Test that really large tag numbers don't break anything.
  unittest::TestReallyLargeTagNumber message1, message2;
  string data;

  // For the most part, if this compiles and runs then we're probably good.
  // (The most likely cause for failure would be if something were attempting
  // to allocate a lookup table of some sort using tag numbers as the index.)
  // We'll try serializing just for fun.
  message1.set_a(1234);
  message1.set_bb(5678);
  message1.SerializeToString(&data);
  EXPECT_TRUE(message2.ParseFromString(data));
  EXPECT_EQ(1234, message2.a());
  EXPECT_EQ(5678, message2.bb());
}

TEST(GeneratedMessageTest, MutualRecursion) {
  // Test that mutually-recursive message types work.
  unittest::TestMutualRecursionA message;
  unittest::TestMutualRecursionA* nested = message.mutable_bb()->mutable_a();
  unittest::TestMutualRecursionA* nested2 = nested->mutable_bb()->mutable_a();

  // Again, if the above compiles and runs, that's all we really have to
  // test, but just for run we'll check that the system didn't somehow come
  // up with a pointer loop...
  EXPECT_NE(&message, nested);
  EXPECT_NE(&message, nested2);
  EXPECT_NE(nested, nested2);
}

TEST(GeneratedMessageTest, CamelCaseFieldNames) {
  // This test is mainly checking that the following compiles, which verifies
  // that the field names were coerced to lower-case.
  //
  // Protocol buffers standard style is to use lowercase-with-underscores for
  // field names.  Some old proto1 .protos unfortunately used camel-case field
  // names.  In proto1, these names were forced to lower-case.  So, we do the
  // same thing in proto2.

  unittest::TestCamelCaseFieldNames message;

  message.set_primitivefield(2);
  message.set_stringfield("foo");
  message.set_enumfield(unittest::FOREIGN_FOO);
  message.mutable_messagefield()->set_c(6);

  message.add_repeatedprimitivefield(8);
  message.add_repeatedstringfield("qux");
  message.add_repeatedenumfield(unittest::FOREIGN_BAR);
  message.add_repeatedmessagefield()->set_c(15);

  EXPECT_EQ(2, message.primitivefield());
  EXPECT_EQ("foo", message.stringfield());
  EXPECT_EQ(unittest::FOREIGN_FOO, message.enumfield());
  EXPECT_EQ(6, message.messagefield().c());

  EXPECT_EQ(8, message.repeatedprimitivefield(0));
  EXPECT_EQ("qux", message.repeatedstringfield(0));
  EXPECT_EQ(unittest::FOREIGN_BAR, message.repeatedenumfield(0));
  EXPECT_EQ(15, message.repeatedmessagefield(0).c());
}

TEST(GeneratedMessageTest, TestConflictingSymbolNames) {
  // test_bad_identifiers.proto successfully compiled, then it works.  The
  // following is just a token usage to insure that the code is, in fact,
  // being compiled and linked.

  protobuf_unittest::TestConflictingSymbolNames message;
  message.set_uint32(1);
  EXPECT_EQ(3, message.ByteSize());

  message.set_friend_(5);
  EXPECT_EQ(5, message.friend_());
}

TEST(GeneratedMessageTest, TestOptimizedForSize) {
  // We rely on the tests in reflection_ops_unittest and wire_format_unittest
  // to really test that reflection-based methods work.  Here we are mostly
  // just making sure that TestOptimizedForSize actually builds and seems to
  // function.

  protobuf_unittest::TestOptimizedForSize message, message2;
  message.set_i(1);
  message.mutable_msg()->set_c(2);
  message2.CopyFrom(message);
  EXPECT_EQ(1, message2.i());
  EXPECT_EQ(2, message2.msg().c());
}

TEST(GeneratedMessageTest, TestEmbedOptimizedForSize) {
  // Verifies that something optimized for speed can contain something optimized
  // for size.

  protobuf_unittest::TestEmbedOptimizedForSize message, message2;
  message.mutable_optional_message()->set_i(1);
  message.add_repeated_message()->mutable_msg()->set_c(2);
  string data;
  message.SerializeToString(&data);
  ASSERT_TRUE(message2.ParseFromString(data));
  EXPECT_EQ(1, message2.optional_message().i());
  EXPECT_EQ(2, message2.repeated_message(0).msg().c());
}

// ===================================================================

TEST(GeneratedEnumTest, EnumValuesAsSwitchCases) {
  // Test that our nested enum values can be used as switch cases.  This test
  // doesn't actually do anything, the proof that it works is that it
  // compiles.
  int i =0;
  unittest::TestAllTypes::NestedEnum a = unittest::TestAllTypes::BAR;
  switch (a) {
    case unittest::TestAllTypes::FOO:
      i = 1;
      break;
    case unittest::TestAllTypes::BAR:
      i = 2;
      break;
    case unittest::TestAllTypes::BAZ:
      i = 3;
      break;
    // no default case:  We want to make sure the compiler recognizes that
    //   all cases are covered.  (GCC warns if you do not cover all cases of
    //   an enum in a switch.)
  }

  // Token check just for fun.
  EXPECT_EQ(2, i);
}

TEST(GeneratedEnumTest, IsValidValue) {
  // Test enum IsValidValue.
  EXPECT_TRUE(unittest::TestAllTypes::NestedEnum_IsValid(1));
  EXPECT_TRUE(unittest::TestAllTypes::NestedEnum_IsValid(2));
  EXPECT_TRUE(unittest::TestAllTypes::NestedEnum_IsValid(3));

  EXPECT_FALSE(unittest::TestAllTypes::NestedEnum_IsValid(0));
  EXPECT_FALSE(unittest::TestAllTypes::NestedEnum_IsValid(4));

  // Make sure it also works when there are dups.
  EXPECT_TRUE(unittest::TestEnumWithDupValue_IsValid(1));
  EXPECT_TRUE(unittest::TestEnumWithDupValue_IsValid(2));
  EXPECT_TRUE(unittest::TestEnumWithDupValue_IsValid(3));

  EXPECT_FALSE(unittest::TestEnumWithDupValue_IsValid(0));
  EXPECT_FALSE(unittest::TestEnumWithDupValue_IsValid(4));
}

TEST(GeneratedEnumTest, MinAndMax) {
  EXPECT_EQ(unittest::TestAllTypes::FOO,unittest::TestAllTypes::NestedEnum_MIN);
  EXPECT_EQ(unittest::TestAllTypes::BAZ,unittest::TestAllTypes::NestedEnum_MAX);

  EXPECT_EQ(unittest::FOREIGN_FOO, unittest::ForeignEnum_MIN);
  EXPECT_EQ(unittest::FOREIGN_BAZ, unittest::ForeignEnum_MAX);

  EXPECT_EQ(1, unittest::TestEnumWithDupValue_MIN);
  EXPECT_EQ(3, unittest::TestEnumWithDupValue_MAX);

  EXPECT_EQ(unittest::SPARSE_E, unittest::TestSparseEnum_MIN);
  EXPECT_EQ(unittest::SPARSE_C, unittest::TestSparseEnum_MAX);

  // Make sure we can use _MIN and _MAX as switch cases.
  switch(unittest::SPARSE_A) {
    case unittest::TestSparseEnum_MIN:
    case unittest::TestSparseEnum_MAX:
      break;
    default:
      break;
  }
}

// ===================================================================

// Support code for testing services.
class GeneratedServiceTest : public testing::Test {
 protected:
  class MockTestService : public unittest::TestService {
   public:
    MockTestService()
      : called_(false),
        method_(""),
        controller_(NULL),
        request_(NULL),
        response_(NULL),
        done_(NULL) {}

    ~MockTestService() {}

    void Reset() { called_ = false; }

    // implements TestService ----------------------------------------

    void Foo(RpcController* controller,
             const unittest::FooRequest* request,
             unittest::FooResponse* response,
             Closure* done) {
      ASSERT_FALSE(called_);
      called_ = true;
      method_ = "Foo";
      controller_ = controller;
      request_ = request;
      response_ = response;
      done_ = done;
    }

    void Bar(RpcController* controller,
             const unittest::BarRequest* request,
             unittest::BarResponse* response,
             Closure* done) {
      ASSERT_FALSE(called_);
      called_ = true;
      method_ = "Bar";
      controller_ = controller;
      request_ = request;
      response_ = response;
      done_ = done;
    }

    // ---------------------------------------------------------------

    bool called_;
    string method_;
    RpcController* controller_;
    const Message* request_;
    Message* response_;
    Closure* done_;
  };

  class MockRpcChannel : public RpcChannel {
   public:
    MockRpcChannel()
      : called_(false),
        method_(NULL),
        controller_(NULL),
        request_(NULL),
        response_(NULL),
        done_(NULL),
        destroyed_(NULL) {}

    ~MockRpcChannel() {
      if (destroyed_ != NULL) *destroyed_ = true;
    }

    void Reset() { called_ = false; }

    // implements TestService ----------------------------------------

    void CallMethod(const MethodDescriptor* method,
                    RpcController* controller,
                    const Message* request,
                    Message* response,
                    Closure* done) {
      ASSERT_FALSE(called_);
      called_ = true;
      method_ = method;
      controller_ = controller;
      request_ = request;
      response_ = response;
      done_ = done;
    }

    // ---------------------------------------------------------------

    bool called_;
    const MethodDescriptor* method_;
    RpcController* controller_;
    const Message* request_;
    Message* response_;
    Closure* done_;
    bool* destroyed_;
  };

  class MockController : public RpcController {
   public:
    void Reset() {
      ADD_FAILURE() << "Reset() not expected during this test.";
    }
    bool Failed() const {
      ADD_FAILURE() << "Failed() not expected during this test.";
      return false;
    }
    string ErrorText() const {
      ADD_FAILURE() << "ErrorText() not expected during this test.";
      return "";
    }
    void StartCancel() {
      ADD_FAILURE() << "StartCancel() not expected during this test.";
    }
    void SetFailed(const string& reason) {
      ADD_FAILURE() << "SetFailed() not expected during this test.";
    }
    bool IsCanceled() const {
      ADD_FAILURE() << "IsCanceled() not expected during this test.";
      return false;
    }
    void NotifyOnCancel(Closure* callback) {
      ADD_FAILURE() << "NotifyOnCancel() not expected during this test.";
    }
  };

  GeneratedServiceTest()
    : descriptor_(unittest::TestService::descriptor()),
      foo_(descriptor_->FindMethodByName("Foo")),
      bar_(descriptor_->FindMethodByName("Bar")),
      stub_(&mock_channel_),
      done_(NewPermanentCallback(&DoNothing)) {}

  virtual void SetUp() {
    ASSERT_TRUE(foo_ != NULL);
    ASSERT_TRUE(bar_ != NULL);
  }

  const ServiceDescriptor* descriptor_;
  const MethodDescriptor* foo_;
  const MethodDescriptor* bar_;

  MockTestService mock_service_;
  MockController mock_controller_;

  MockRpcChannel mock_channel_;
  unittest::TestService::Stub stub_;

  // Just so we don't have to re-define these with every test.
  unittest::FooRequest foo_request_;
  unittest::FooResponse foo_response_;
  unittest::BarRequest bar_request_;
  unittest::BarResponse bar_response_;
  scoped_ptr<Closure> done_;
};

TEST_F(GeneratedServiceTest, GetDescriptor) {
  // Test that GetDescriptor() works.

  EXPECT_EQ(descriptor_, mock_service_.GetDescriptor());
}

TEST_F(GeneratedServiceTest, GetChannel) {
  EXPECT_EQ(&mock_channel_, stub_.channel());
}

TEST_F(GeneratedServiceTest, OwnsChannel) {
  MockRpcChannel* channel = new MockRpcChannel;
  bool destroyed = false;
  channel->destroyed_ = &destroyed;

  {
    unittest::TestService::Stub owning_stub(channel,
                                            Service::STUB_OWNS_CHANNEL);
    EXPECT_FALSE(destroyed);
  }

  EXPECT_TRUE(destroyed);
}

TEST_F(GeneratedServiceTest, CallMethod) {
  // Test that CallMethod() works.

  // Call Foo() via CallMethod().
  mock_service_.CallMethod(foo_, &mock_controller_,
                           &foo_request_, &foo_response_, done_.get());

  ASSERT_TRUE(mock_service_.called_);

  EXPECT_EQ("Foo"            , mock_service_.method_    );
  EXPECT_EQ(&mock_controller_, mock_service_.controller_);
  EXPECT_EQ(&foo_request_    , mock_service_.request_   );
  EXPECT_EQ(&foo_response_   , mock_service_.response_  );
  EXPECT_EQ(done_.get()      , mock_service_.done_      );

  // Try again, but call Bar() instead.
  mock_service_.Reset();
  mock_service_.CallMethod(bar_, &mock_controller_,
                           &bar_request_, &bar_response_, done_.get());

  ASSERT_TRUE(mock_service_.called_);
  EXPECT_EQ("Bar", mock_service_.method_);
}

TEST_F(GeneratedServiceTest, CallMethodTypeFailure) {
  // Verify death if we call Foo() with Bar's message types.

#ifdef GTEST_HAS_DEATH_TEST  // death tests do not work on Windows yet
  EXPECT_DEBUG_DEATH(
    mock_service_.CallMethod(foo_, &mock_controller_,
                             &foo_request_, &bar_response_, done_.get()),
    "dynamic_cast");

  mock_service_.Reset();
  EXPECT_DEBUG_DEATH(
    mock_service_.CallMethod(foo_, &mock_controller_,
                             &bar_request_, &foo_response_, done_.get()),
    "dynamic_cast");
#endif  // GTEST_HAS_DEATH_TEST
}

TEST_F(GeneratedServiceTest, GetPrototypes) {
  // Test Get{Request,Response}Prototype() methods.

  EXPECT_EQ(&unittest::FooRequest::default_instance(),
            &mock_service_.GetRequestPrototype(foo_));
  EXPECT_EQ(&unittest::BarRequest::default_instance(),
            &mock_service_.GetRequestPrototype(bar_));

  EXPECT_EQ(&unittest::FooResponse::default_instance(),
            &mock_service_.GetResponsePrototype(foo_));
  EXPECT_EQ(&unittest::BarResponse::default_instance(),
            &mock_service_.GetResponsePrototype(bar_));
}

TEST_F(GeneratedServiceTest, Stub) {
  // Test that the stub class works.

  // Call Foo() via the stub.
  stub_.Foo(&mock_controller_, &foo_request_, &foo_response_, done_.get());

  ASSERT_TRUE(mock_channel_.called_);

  EXPECT_EQ(foo_             , mock_channel_.method_    );
  EXPECT_EQ(&mock_controller_, mock_channel_.controller_);
  EXPECT_EQ(&foo_request_    , mock_channel_.request_   );
  EXPECT_EQ(&foo_response_   , mock_channel_.response_  );
  EXPECT_EQ(done_.get()      , mock_channel_.done_      );

  // Call Bar() via the stub.
  mock_channel_.Reset();
  stub_.Bar(&mock_controller_, &bar_request_, &bar_response_, done_.get());

  ASSERT_TRUE(mock_channel_.called_);
  EXPECT_EQ(bar_, mock_channel_.method_);
}

TEST_F(GeneratedServiceTest, NotImplemented) {
  // Test that failing to implement a method of a service causes it to fail
  // with a "not implemented" error message.

  // A service which doesn't implement any methods.
  class UnimplementedService : public unittest::TestService {
   public:
    UnimplementedService() {}
  };

  UnimplementedService unimplemented_service;

  // And a controller which expects to get a "not implemented" error.
  class ExpectUnimplementedController : public MockController {
   public:
    ExpectUnimplementedController() : called_(false) {}

    void SetFailed(const string& reason) {
      EXPECT_FALSE(called_);
      called_ = true;
      EXPECT_EQ("Method Foo() not implemented.", reason);
    }

    bool called_;
  };

  ExpectUnimplementedController controller;

  // Call Foo.
  unimplemented_service.Foo(&controller, &foo_request_, &foo_response_,
                            done_.get());

  EXPECT_TRUE(controller.called_);
}

}  // namespace

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
