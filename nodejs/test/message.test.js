var protobuf = require('google_protobuf');
var assert = require('assert');

// Helper: create a test message with all field types.
function testMsgDescs() {
  var pool = new protobuf.DescriptorPool();
  pool.add([
      new protobuf.Descriptor("TestMessage",
      [ new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
          type: protobuf.FieldDescriptor.TYPE_STRING,
          name: "optional_string", number: 1}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
          type: protobuf.FieldDescriptor.TYPE_BYTES,
          name: "optional_bytes", number: 2}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
          type: protobuf.FieldDescriptor.TYPE_MESSAGE,
          name: "optional_message", number: 3, subtype_name: "SubMessage"}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
          type: protobuf.FieldDescriptor.TYPE_ENUM,
          name: "optional_enum", number: 4, subtype_name: "EnumType"}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
          type: protobuf.FieldDescriptor.TYPE_BOOL,
          name: "optional_bool", number: 5}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
          type: protobuf.FieldDescriptor.TYPE_FLOAT,
          name: "optional_float", number: 6}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
          type: protobuf.FieldDescriptor.TYPE_DOUBLE,
          name: "optional_double", number: 7}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
          type: protobuf.FieldDescriptor.TYPE_INT32,
          name: "optional_int32", number: 8}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
          type: protobuf.FieldDescriptor.TYPE_UINT32,
          name: "optional_uint32", number: 9}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
          type: protobuf.FieldDescriptor.TYPE_INT64,
          name: "optional_int64", number: 10}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
          type: protobuf.FieldDescriptor.TYPE_UINT64,
          name: "optional_uint64", number: 11}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_REPEATED,
          type: protobuf.FieldDescriptor.TYPE_STRING,
          name: "repeated_string", number: 21}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_REPEATED,
          type: protobuf.FieldDescriptor.TYPE_BYTES,
          name: "repeated_bytes", number: 22}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_REPEATED,
          type: protobuf.FieldDescriptor.TYPE_MESSAGE,
          name: "repeated_message", number: 23, subtype_name: "SubMessage"}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_REPEATED,
          type: protobuf.FieldDescriptor.TYPE_ENUM,
          name: "repeated_enum", number: 24, subtype_name: "EnumType"}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_REPEATED,
          type: protobuf.FieldDescriptor.TYPE_BOOL,
          name: "repeated_bool", number: 25}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_REPEATED,
          type: protobuf.FieldDescriptor.TYPE_FLOAT,
          name: "repeated_float", number: 26}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_REPEATED,
          type: protobuf.FieldDescriptor.TYPE_DOUBLE,
          name: "repeated_double", number: 27}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_REPEATED,
          type: protobuf.FieldDescriptor.TYPE_INT32,
          name: "repeated_int32", number: 28}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_REPEATED,
          type: protobuf.FieldDescriptor.TYPE_UINT32,
          name: "repeated_uint32", number: 29}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_REPEATED,
          type: protobuf.FieldDescriptor.TYPE_INT64,
          name: "repeated_int64", number: 30}),
        new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_REPEATED,
          type: protobuf.FieldDescriptor.TYPE_UINT64,
          name: "repeated_uint64", number: 31}) ]),
      new protobuf.Descriptor("SubMessage",
      [ new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
          type: protobuf.FieldDescriptor.TYPE_STRING,
          name: "field1", number: 1}) ]),
      new protobuf.EnumDescriptor("EnumType",
        "Default", 0,
        "A", 1,
        "B", 2) ]);

  return [pool.lookup("TestMessage"), pool.lookup("SubMessage"),
          pool.lookup("EnumType")];
}

// Test singular field assignment and error checking.
(function() {
  var rets = testMsgDescs();
  var TestMessage = rets[0].msgclass;
  var SubMessage = rets[1].msgclass;
  var EnumType = rets[2].enumobject;

  assert.equal(TestMessage.descriptor, rets[0]);
  assert.equal(SubMessage.descriptor, rets[1]);
  assert.equal(EnumType.descriptor, rets[2]);

  var m1 = new TestMessage();
  assert.equal(m1.descriptor, TestMessage.descriptor);
  assert.equal(m1.optional_int32, 0);
  assert.equal(m1.optional_uint32, 0);
  assert.equal(m1.optional_int64.toString(), "0");
  assert.equal(m1.optional_float, 0.0);
  assert.equal(m1.optional_double, 0.0);
  assert.equal(m1.optional_bool, false);
  assert.equal(m1.optional_string, "");
  assert.equal(m1.optional_bytes, "");
  assert.equal(m1.optional_message, undefined);
  assert.equal(m1.optional_enum, 0);

  assert.throws(function() {
    m1.field_does_not_exist = 1;
  });
  assert.throws(function() {
    var value = m1.field_does_not_exist;
  });

  m1.optional_int32 = 0x7fffffff;
  assert.equal(m1.optional_int32, 0x7fffffff);
  m1.optional_int32 = -0x80000000;
  assert.equal(m1.optional_int32, -0x80000000);
  assert.throws(function() {
    m1.optional_int32 = 0x80000000;
  });
  assert.throws(function() {
    m1.optional_int32 = -0x80000001;
  });
  assert.throws(function() {
    m1.optional_int32 = 1.234;
  });
  assert.throws(function() {
    m1.optional_int32 = "hello";
  });

  m1.optional_uint32 = 0xffffffff;
  assert.equal(m1.optional_uint32, 0xffffffff);
  assert.throws(function() {
    m1.optional_uint32 = -1;
  });
  assert.throws(function() {
    m1.optional_uint32 = 1.234;
  });
  assert.throws(function() {
    m1.optional_uint32 = "hello";
  });

  m1.optional_int64 = new protobuf.Int64(42 * 4 * 1024 * 1024 * 1024);
  assert.equal(protobuf.Int64.hi(m1.optional_int64), 42);
  assert.equal(protobuf.Int64.lo(m1.optional_int64), 0);
  assert.throws(function() {
    m1.optional_int64 = 42;
  });
  assert.throws(function() {
    m1.optional_int64 = new Object();
  });

  m1.optional_uint64 = new protobuf.UInt64(42 * 4 * 1024 * 1024 * 1024);
  assert.equal(protobuf.UInt64.hi(m1.optional_uint64), 42);
  assert.equal(protobuf.UInt64.lo(m1.optional_uint64), 0);
  assert.throws(function() {
    m1.optional_uint64 = 42;
  });
  assert.throws(function() {
    m1.optional_uint64 = new Object();
  });

  m1.optional_bool = true;
  assert.equal(m1.optional_bool, true);
  assert.throws(function() {
    m1.optional_bool = 1;
  });

  assert.equal(m1.optional_enum, EnumType.Default);
  m1.optional_enum = EnumType.A;
  assert.equal(m1.optional_enum, EnumType.A);
  assert.equal(m1.optional_enum, 1);
  m1.optional_enum = 0xffffffff;
  assert.equal(m1.optional_enum, 0xffffffff);
  assert.throws(function() {
    m1.optional_enum = -1;
  });
  assert.throws(function() {
    m1.optional_enum = "A";
  });

  m1.optional_float = 1e100;
  assert.equal(m1.optional_float, 1e100);
  m1.optional_float = -1e100;
  assert.equal(m1.optional_float, -1e100);
  m1.optional_float = 42;
  assert.equal(m1.optional_float, 42);
  assert.throws(function() {
    m1.optional_float = "hello";
  });

  m1.optional_double = 1e100;
  assert.equal(m1.optional_double, 1e100);
  m1.optional_double = -1e100;
  assert.equal(m1.optional_double, -1e100);
  m1.optional_double = 42;
  assert.equal(m1.optional_double, 42);
  assert.throws(function() {
    m1.optional_double = "hello";
  });

  m1.optional_string = "Hello, world";
  assert.equal(m1.optional_string, "Hello, world");
  assert.throws(function() {
    m1.optional_string = 1234;
  });
  assert.throws(function() {
    m1.optional_string = new Object();
  });

  m1.optional_bytes = new Buffer([255, 255]);
  assert.equal(m1.optional_bytes.toString(), new Buffer([255, 255]).toString());
  assert.throws(function() {
    m1.optional_string = 1234;
  });
  assert.throws(function() {
    m1.optional_string = new Object();
  });

  m1.optional_message = new SubMessage();
  assert.equal(m1.optional_message.field1, "");
  m1.optional_message = undefined;
  assert.equal(m1.optional_message, undefined);
  assert.throws(function() {
    m1.optional_message = new TestMessage();
  });
  assert.throws(function() {
    m1.optional_message = new Object();
  });
  assert.throws(function() {
    m1.optional_message = "hello";
  });
})();

// Test repeated fields. We already tested the repeated-field in
// rptfield.test.js, so here we just test the message's interaction with the
// container.
(function() {
  var rets = testMsgDescs();
  var TestMessage = rets[0].msgclass;
  var SubMessage = rets[1].msgclass;
  var EnumType = rets[2].enumobject;

  var m = new TestMessage();
  assert.equal(m.repeated_int32.length, 0);
  m.repeated_int32.push(1);
  m.repeated_int32.push(2);
  m.repeated_int32.push(3);

  m.repeated_int32 =
      new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_INT32);
  assert.equal(m.repeated_int32.length, 0);

  assert.throws(function() {
    m.repeated_int32 =
        new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_INT64);
  });
  assert.throws(function() {
    m.repeated_int32 = [1, 2, 3];
  });

  m.repeated_message.push(new SubMessage());
  assert.equal(m.repeated_message.length, 1);

  m.repeated_message =
      new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_MESSAGE,
                                 SubMessage);
  m.repeated_message.push(new SubMessage());
  assert.equal(m.repeated_message.length, 1);

  assert.throws(function() {
    m.repeated_message =
        new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_MESSAGE,
                                   TestMessage);
  });
})();

// Test constructor arguments.
(function() {
  var rets = testMsgDescs();
  var TestMessage = rets[0].msgclass;
  var SubMessage = rets[1].msgclass;
  var EnumType = rets[2].enumobject;

  var m = new TestMessage({
    optional_int32: 42,
    optional_string: "hello",
    optional_message: new SubMessage({ field1: "foo" }),
    repeated_string: ["a", "b", "c"]
  });

  assert.equal(m.optional_int32, 42);
  assert.equal(m.optional_string, "hello");
  assert.equal(m.optional_message.field1, "foo");
  assert.equal(m.repeated_string.length, 3);
  assert.equal(m.repeated_string[0], "a");
  assert.equal(m.repeated_string[1], "b");
  assert.equal(m.repeated_string[2], "c");
})();

// Test toString().
(function() {
  var rets = testMsgDescs();
  var TestMessage = rets[0].msgclass;
  var SubMessage = rets[1].msgclass;
  var EnumType = rets[2].enumobject;

  var m = new TestMessage();
  assert.equal(m.toString(), "{ optional_string: \"\" optional_bytes: \"\" optional_message: undefined optional_enum: Default optional_bool: false optional_float: 0 optional_double: 0 optional_int32: 0 optional_uint32: 0 optional_int64: 0 optional_uint64: 0 repeated_string: [] repeated_bytes: [] repeated_message: [] repeated_enum: [] repeated_bool: [] repeated_float: [] repeated_double: [] repeated_int32: [] repeated_uint32: [] repeated_int64: [] repeated_uint64: [] }");

  m.optional_string = "hello world";
  m.optional_bytes = new Buffer([255, 255]);
  m.optional_message = new SubMessage();
  m.optional_message.field1 = "stringvalue";
  m.optional_bool = true;
  m.optional_int32 = -42;
  m.optional_enum = EnumType.A;
  m.repeated_string.push("a");
  m.repeated_string.push("b");
  m.repeated_string.push("c");
  assert.equal(m.toString(), "{ optional_string: \"hello world\" optional_bytes: \"\\xff\\xff\" optional_message: { field1: \"stringvalue\" } optional_enum: A optional_bool: true optional_float: 0 optional_double: 0 optional_int32: -42 optional_uint32: 0 optional_int64: 0 optional_uint64: 0 repeated_string: [\"a\", \"b\", \"c\"] repeated_bytes: [] repeated_message: [] repeated_enum: [] repeated_bool: [] repeated_float: [] repeated_double: [] repeated_int32: [] repeated_uint32: [] repeated_int64: [] repeated_uint64: [] }");
})();

// Test oneofs.
(function() {
  var p = new protobuf.DescriptorPool();
  p.add([
      new protobuf.Descriptor("TestMessage",
        [],
        [ new protobuf.OneofDescriptor("my_oneof",
          [ new protobuf.FieldDescriptor({
              label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
              type: protobuf.FieldDescriptor.TYPE_INT32,
              name: "oneof_int32", number: 1}),
            new protobuf.FieldDescriptor({
              label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
              type: protobuf.FieldDescriptor.TYPE_STRING,
              name: "oneof_string", number: 2}),
            new protobuf.FieldDescriptor({
              label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
              type: protobuf.FieldDescriptor.TYPE_MESSAGE,
              name: "oneof_message", number: 3, subtype_name: "SubMessage"}) ]) ]),
      new protobuf.Descriptor("SubMessage",
        [ new protobuf.FieldDescriptor({
              label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
              type: protobuf.FieldDescriptor.TYPE_INT32,
              name: "foo", number: 1}) ])]);

  var TestMessage = p.lookup("TestMessage").msgclass;
  var SubMessage = p.lookup("SubMessage").msgclass;

  var m = new TestMessage();

  assert.equal(m.my_oneof, undefined);
  assert.equal(m.oneof_int32, undefined);
  assert.equal(m.oneof_string, undefined);
  assert.equal(m.oneof_message, undefined);

  m.oneof_int32 = 42;
  assert.equal(m.my_oneof, "oneof_int32");
  assert.equal(m.oneof_int32, 42);
  assert.equal(m.oneof_string, undefined);
  assert.equal(m.oneof_message, undefined);

  m.oneof_int32 = undefined;
  assert.equal(m.my_oneof, undefined);
  assert.equal(m.oneof_int32, undefined);

  m.oneof_int32 = 42;
  assert.equal(m.my_oneof, "oneof_int32");
  m.oneof_string = "hello";
  assert.equal(m.my_oneof, "oneof_string");
  assert.equal(m.oneof_int32, undefined);
  assert.equal(m.oneof_string, "hello");

  m.oneof_message = new SubMessage({ foo: 42 });
  assert.equal(m.oneof_message.foo, 42);
  assert.equal(m.oneof_string, undefined);

  m.oneof_message = undefined;
  assert.equal(m.my_oneof, undefined);
})();

// Test encoding.
(function() {
  var rets = testMsgDescs();
  var TestMessage = rets[0].msgclass;
  var SubMessage = rets[1].msgclass;
  var EnumType = rets[2];

  var m = new TestMessage();
  m.optional_string = "hello world";
  m.optional_bytes = new Buffer([255, 255]);
  m.optional_message = new SubMessage();
  m.optional_message.field1 = "ABCD";
  m.optional_bool = true;
  m.optional_int32 = -42;
  m.repeated_string.push("a");
  m.repeated_string.push("b");
  m.repeated_string.push("c");

  var m2 = TestMessage.decodeBinary(TestMessage.encodeBinary(m));
  assert.equal(m.toString(), m2.toString());

  var m3 = TestMessage.decodeJson(TestMessage.encodeJson(m));
  assert.equal(m.toString(), m3.toString());
})();

// Test global versions of encode/decode functions.
(function() {
  var rets = testMsgDescs();
  var TestMessage = rets[0].msgclass;
  var SubMessage = rets[1].msgclass;
  var EnumType = rets[2];

  var m = new TestMessage();
  m.optional_string = "hello world";
  m.optional_bytes = new Buffer([255, 255]);
  m.optional_message = new SubMessage();
  m.optional_message.field1 = "ABCD";
  m.optional_bool = true;
  m.optional_int32 = -42;
  m.repeated_string.push("a");
  m.repeated_string.push("b");
  m.repeated_string.push("c");

  assert.equal(m.toString(),
      protobuf.decodeBinary(TestMessage, protobuf.encodeBinary(TestMessage, m)));
  assert.equal(m.toString(),
      protobuf.decodeJson(TestMessage, protobuf.encodeJson(TestMessage, m)));
})();

// Test constructor form that allows recursively converting plain-old JavaScript
// objects to messages.
(function() {
  var rets = testMsgDescs();
  var TestMessage = rets[0].msgclass;

  var m = new TestMessage({
    optional_message: { field1: "ABCD" },
    repeated_message: [
      { field1: "A" },
      { field1: "B" },
      { field1: "C" }
    ]
  });

  assert.equal(m.optional_message.field1, "ABCD");
  assert.equal(m.repeated_message.length, 3);
  assert.equal(m.repeated_message[0].field1, "A");
  assert.equal(m.repeated_message[1].field1, "B");
  assert.equal(m.repeated_message[2].field1, "C");
})();

console.log("message.test.js PASS");
