var protobuf = require('google_protobuf');
var assert = require('assert');

// Test ReadOnlyArray
(function() {
  var a = new protobuf.ReadOnlyArray([1, 2, 3, 4]);
  assert.equal(a.length, 4);
  for (var i = 0; i < a.length; i++) {
    assert.equal(a[i], i + 1);
  }

  assert.throws(function() {
    a[1] = 42;
  }, /elements cannot be changed/);

  assert.equal(a[1], 2);

  assert.throws(function() {
    delete a[1];
  }, /elements cannot be deleted/);

  // Test iterator protocol. Without ES6, we don't have Symbol.iterator, but the
  // array itself acts as an iterator.
  var iterator = a;
  var l = new Array();
  for (var i = iterator.next(); !i.done; i = iterator.next()) {
    l.push(i.value);
  }
  assert.equal(l.length, 4);
  assert.equal(l[0], 1);
  assert.equal(l[1], 2);
  assert.equal(l[2], 3);
  assert.equal(l[3], 4);
})();

// Test FieldDescriptor and Descriptor
(function() {
  var m = new protobuf.Descriptor();
  m.name = "TestMessage";
  assert.equal(m.name, "TestMessage");

  var f1 = new protobuf.FieldDescriptor();
  f1.name = "field1";
  f1.type = protobuf.FieldDescriptor.TYPE_UINT32;
  f1.label = protobuf.FieldDescriptor.LABEL_OPTIONAL;
  f1.number = 1;
  assert.equal(m.addField(f1), f1);
  assert.equal(f1.descriptor, m);

  var f2 = new protobuf.FieldDescriptor();
  f2.name = "field2";
  f2.type = protobuf.FieldDescriptor.TYPE_STRING;
  f2.label = protobuf.FieldDescriptor.LABEL_REPEATED;
  f2.number = 2;
  assert.equal(m.addField(f2), f2);
  assert.equal(f2.descriptor, m);

  var f3 = new protobuf.FieldDescriptor();
  f3.name = "field3";
  f3.type = protobuf.FieldDescriptor.TYPE_UINT32;
  f3.label = protobuf.FieldDescriptor.LABEL_OPTIONAL;
  f3.number = 3;
  assert.equal(m.addField(f3), f3);
  assert.equal(f3.descriptor, m);

  assert.equal(m.findFieldByName("field3"), f3);
  assert.equal(m.findFieldByName("fieldX"), null);
  assert.equal(m.findFieldByNumber(2), f2);
  assert.equal(m.findFieldByNumber(42), null);

  var fields = m.fields;
  assert.equal(fields.length, 3);
  assert.equal(fields[0], f1);
  assert.equal(fields[1], f2);
  assert.equal(fields[2], f3);

  var f4 = new protobuf.FieldDescriptor();
  f4.name = "field1";  // duplicate name
  f4.type = protobuf.FieldDescriptor.TYPE_UINT32;
  f4.label = protobuf.FieldDescriptor.LABEL_OPTIONAL;
  f4.number = 4;
  assert.throws(function() {
    m.addField(f4);
  }, Error);

  f4.name = "field4";
  f4.number = 1;  // duplicate number
  assert.throws(function() {
    m.addField(f4);
  }, Error);
})();

// Test FieldDescriptor exhaustively
(function() {
  var f = new protobuf.FieldDescriptor();
  assert.equal(f.name, "");
  f.name = "test_field";
  assert.equal(f.name, "test_field");
  assert.throws(function() {
    f.name = "---";
  });

  f.type = protobuf.FieldDescriptor.TYPE_INT32;
  assert.equal(f.type, protobuf.FieldDescriptor.TYPE_INT32);
  f.type = protobuf.FieldDescriptor.TYPE_INT64;
  assert.equal(f.type, protobuf.FieldDescriptor.TYPE_INT64);
  f.type = protobuf.FieldDescriptor.TYPE_UINT32;
  assert.equal(f.type, protobuf.FieldDescriptor.TYPE_UINT32);
  f.type = protobuf.FieldDescriptor.TYPE_UINT64;
  assert.equal(f.type, protobuf.FieldDescriptor.TYPE_UINT64);
  f.type = protobuf.FieldDescriptor.TYPE_BOOL;
  assert.equal(f.type, protobuf.FieldDescriptor.TYPE_BOOL);
  f.type = protobuf.FieldDescriptor.TYPE_FLOAT;
  assert.equal(f.type, protobuf.FieldDescriptor.TYPE_FLOAT);
  f.type = protobuf.FieldDescriptor.TYPE_DOUBLE;
  assert.equal(f.type, protobuf.FieldDescriptor.TYPE_DOUBLE);
  f.type = protobuf.FieldDescriptor.TYPE_ENUM;
  assert.equal(f.type, protobuf.FieldDescriptor.TYPE_ENUM);
  f.type = protobuf.FieldDescriptor.TYPE_STRING;
  assert.equal(f.type, protobuf.FieldDescriptor.TYPE_STRING);
  f.type = protobuf.FieldDescriptor.TYPE_BYTES;
  assert.equal(f.type, protobuf.FieldDescriptor.TYPE_BYTES);
  f.type = protobuf.FieldDescriptor.TYPE_MESSAGE;
  assert.equal(f.type, protobuf.FieldDescriptor.TYPE_MESSAGE);

  assert.throws(function() {
    f.type = 42;
  });

  f.label = protobuf.FieldDescriptor.LABEL_OPTIONAL;
  assert.equal(f.label, protobuf.FieldDescriptor.LABEL_OPTIONAL);
  f.label = protobuf.FieldDescriptor.LABEL_REPEATED;
  assert.equal(f.label, protobuf.FieldDescriptor.LABEL_REPEATED);
  assert.throws(function() {
    f.label = 42;
  });

  assert.throws(function() {
    f.number = 0;
  });
  f.number = 1;
  assert.equal(f.number, 1);
  f.number = ((1 << 29) - 1);
  assert.equal(f.number, ((1 << 29) - 1));

  f.type = protobuf.FieldDescriptor.TYPE_INT32;
  f.type = protobuf.FieldDescriptor.TYPE_MESSAGE;
  assert.equal(f.subtype_name, "");
  f.type = protobuf.FieldDescriptor.TYPE_ENUM;
  assert.equal(f.subtype_name, "");

  f.subtype_name = "Subtype";
  assert.equal(f.subtype_name, "Subtype");
})();

// Test oneofs
(function() {
  var o = new protobuf.OneofDescriptor();
  assert.equal(o.name, "");
  o.name = "TestOneof";
  assert.equal(o.name, "TestOneof");
  assert.throws(function() {
    o.name = "---";
  });

  var f1 = new protobuf.FieldDescriptor();
  f1.name = "field1";
  f1.type = protobuf.FieldDescriptor.TYPE_UINT32;
  f1.label = protobuf.FieldDescriptor.LABEL_OPTIONAL;
  f1.number = 1;
  assert.equal(o.addField(f1), f1);
  assert.equal(f1.oneof, o);

  var f2 = new protobuf.FieldDescriptor();
  f2.name = "field2";
  f2.type = protobuf.FieldDescriptor.TYPE_STRING;
  f2.label = protobuf.FieldDescriptor.LABEL_OPTIONAL;
  f2.number = 2;
  assert.equal(o.addField(f2), f2);
  assert.equal(f2.oneof, o);

  assert.equal(o.findFieldByName("field1"), f1);
  assert.equal(o.findFieldByName("fieldX"), null);
  assert.equal(o.findFieldByNumber(2), f2);
  assert.equal(o.findFieldByNumber(42), null);

  var f3 = new protobuf.FieldDescriptor();
  f3.name = "field3";
  f3.type = protobuf.FieldDescriptor.TYPE_STRING;
  f3.label = protobuf.FieldDescriptor.LABEL_REPEATED;
  f3.number = 3;
  assert.throws(function() {
    // Throws due to REPEATED
    o.addField(f3);
  });

  f3.label = protobuf.FieldDescriptor.LABEL_OPTIONAL;
  f3.number = 2;
  assert.throws(function() {
    // Throws due to duplicate number
    o.addField(f3);
  });

  f3.label = protobuf.FieldDescriptor.LABEL_OPTIONAL;
  f3.number = 3;
  f3.name = "field2";
  assert.throws(function() {
    // Throws due to duplicate name
    o.addField(f3);
  });

  // Add to a Descriptor and test that fields appear at top level.
  var m = new protobuf.Descriptor();
  assert.equal(m.addOneof(o), o);
  assert.equal(o.descriptor, m);
  assert.equal(m.oneofs.length, 1);
  assert.equal(m.oneofs[0], o);
  assert.equal(m.findOneof("TestOneof"), o);
  assert.equal(m.findOneof("asdf"), null);
  assert.equal(o.descriptor, m);

  assert.equal(m.findFieldByName("field1"), f1);
  assert.equal(f1.descriptor, m);
  assert.equal(m.findFieldByNumber(1), f1);

  // Add a new field to the oneof and test that it appears in both places.
  var f4 = new protobuf.FieldDescriptor();
  f4.name = "field4";

  f4.type = protobuf.FieldDescriptor.TYPE_STRING;
  f4.label = protobuf.FieldDescriptor.LABEL_OPTIONAL;
  f4.number = 4;
  assert.equal(o.addField(f4), f4);
  assert.equal(f4.oneof, o);
  assert.equal(f4.descriptor, m);

  // Add a new field to the descriptor first, then to the oneof.
  var f5 = new protobuf.FieldDescriptor();
  f5.name = "field5";
  f5.type = protobuf.FieldDescriptor.TYPE_STRING;
  f5.label = protobuf.FieldDescriptor.LABEL_OPTIONAL;
  f5.number = 5;
  assert.equal(m.addField(f5), f5);
  assert.equal(f5.descriptor, m);
  assert.equal(f5.oneof, null);
  assert.equal(o.addField(f5), f5);
  assert.equal(f5.descriptor, m);
  assert.equal(f5.oneof, o);
})();

// Test EnumDescriptor
(function() {
  var e = new protobuf.EnumDescriptor();
  assert.equal(e.name, "");
  e.name = "MyEnum";
  assert.equal(e.name, "MyEnum");
  assert.throws(function() {
    e.name = "---";
  });

  assert.equal(e.keys.length, 0);
  assert.equal(e.values.length, 0);

  assert.equal(e.add("A", 1), "A");
  assert.equal(e.add("B", 2), "B");
  assert.throws(function() {
    e.add("A", 3);
  });

  assert.equal(e.keys.length, 2);
  assert.equal(e.keys[0], "A");
  assert.equal(e.keys[1], "B");
  assert.equal(e.values.length, 2);
  assert.equal(e.values[0], 1);
  assert.equal(e.values[1], 2);

  assert.equal(e.findByName("A"), 1);
  assert.equal(e.findByName("B"), 2);
  assert.equal(e.findByName("C"), null);
  assert.equal(e.findByValue(1), "A");
  assert.equal(e.findByValue(2), "B");
  assert.equal(e.findByValue(3), null);
})();

// Test DescriptorPool
(function() {
  var pool = new protobuf.DescriptorPool();
  assert.equal(pool.descriptors.length, 0);
  assert.equal(pool.enums.length, 0);

  var msg1 = new protobuf.Descriptor();
  msg1.name = "TestMessage";

  var f1 = new protobuf.FieldDescriptor();
  f1.name = "field1";
  f1.type = protobuf.FieldDescriptor.TYPE_INT32;
  f1.label = protobuf.FieldDescriptor.LABEL_OPTIONAL;
  f1.number = 1;
  assert.equal(msg1.addField(f1), f1);

  var f2 = new protobuf.FieldDescriptor();
  f2.name = "field2";
  f2.type = protobuf.FieldDescriptor.TYPE_MESSAGE;
  f2.label = protobuf.FieldDescriptor.LABEL_OPTIONAL;
  f2.number = 2;
  f2.subtype_name = "SubMessage";
  assert.equal(msg1.addField(f2), f2);

  var f3 = new protobuf.FieldDescriptor();
  f3.name = "field3";
  f3.type = protobuf.FieldDescriptor.TYPE_ENUM;
  f3.label = protobuf.FieldDescriptor.LABEL_OPTIONAL;
  f3.number = 3;
  f3.subtype_name = "MyEnum";
  assert.equal(msg1.addField(f3), f3);

  var msg2 = new protobuf.Descriptor();
  msg2.name = "SubMessage";

  var f4 = new protobuf.FieldDescriptor();
  f4.name = "field4";
  f4.type = protobuf.FieldDescriptor.TYPE_ENUM;
  f4.label = protobuf.FieldDescriptor.LABEL_OPTIONAL;
  f4.number = 4;
  f4.subtype_name = "MyEnum";
  assert.equal(msg2.addField(f4), f4);

  var enum1 = new protobuf.EnumDescriptor();
  enum1.name = "MyEnum";
  enum1.add("Default", 0);
  enum1.add("A", 1);
  enum1.add("B", 2);
  assert.equal(enum1.keys.length, 3);
  assert.equal(enum1.values.length, 3);

  pool.add([msg1, msg2, enum1]);

  assert.equal(pool.descriptors.length, 2);
  assert.equal(pool.enums.length, 1);

  assert.equal(pool.lookup("TestMessage"), msg1);
  assert.equal(pool.lookup("SubMessage"), msg2);
  assert.equal(pool.lookup("MyEnum"), enum1);

  assert.equal(f2.subtype, msg2);
  assert.equal(f3.subtype, enum1);
  assert.equal(f4.subtype, enum1);

  assert.throws(function() {
    msg1.name = "NewName";
  });

})();

// Test constructor-based message, oneof, field, and enum setup.

(function() {

  // Field: four-arg form (no subtype).
  var f1 = new protobuf.FieldDescriptor({
    label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
    type: protobuf.FieldDescriptor.TYPE_STRING,
    name: "field1",
    number: 42});
  assert.equal(f1.label, protobuf.FieldDescriptor.LABEL_OPTIONAL);
  assert.equal(f1.type, protobuf.FieldDescriptor.TYPE_STRING);
  assert.equal(f1.name, "field1");
  assert.equal(f1.number, 42);

  // Field: five-arg form (with subtype).
  var f2 = new protobuf.FieldDescriptor({
    label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
    type: protobuf.FieldDescriptor.TYPE_MESSAGE,
    name: "field2",
    number: 43,
    subtype_name: "SubMessage"});
  assert.equal(f2.label, protobuf.FieldDescriptor.LABEL_OPTIONAL);
  assert.equal(f2.type, protobuf.FieldDescriptor.TYPE_MESSAGE);
  assert.equal(f2.name, "field2");
  assert.equal(f2.number, 43);
  assert.equal(f2.subtype_name, "SubMessage");
  assert.throws(function() {
    f2.subtype = null;
  });
  assert.throws(function() {
    var s = f2.subtype;
  });

  // Test that errors in args cause exceptions.
  assert.throws(function() {
    var f = new protobuf.FieldDescriptor({
      label: 1000,
      type: protobuf.FieldDescriptor.TYPE_MESSAGE,
      name: "field2",
      number: 43,
      subtype_name: "SubMessage"});
  });
  assert.throws(function() {
    var f = new protobuf.FieldDescriptor({
        label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
        type: 1000,
        name: "field2",
        number: 43,
        subtype_name: "SubMessage"});
  });
  assert.throws(function() {
    var f = new protobuf.FieldDescriptor({
        label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
        type: protobuf.FieldDescriptor.TYPE_MESSAGE,
        name: null,
        number: 43,
        subtype_name: "SubMessage"});
  });
  assert.throws(function() {
    var f = new protobuf.FieldDescriptor({
        label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
        type: protobuf.FieldDescriptor.TYPE_MESSAGE,
        name: "field2",
        number: -1,
        subtype_name: "SubMessage"});
  });
  assert.throws(function() {
    var f = new protobuf.FieldDescriptor({
        label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
        type: protobuf.FieldDescriptor.TYPE_MESSAGE,
        name: "field2",
        number: 43,
        subtype_name: null});
  });

  // Oneof: one-arg form.
  var o1 = new protobuf.OneofDescriptor("MyOneof");
  assert.equal(o1.name, "MyOneof");
  assert.equal(o1.fields.length, 0);

  // Oneof: two-arg form.
  var o2_f1 = new protobuf.FieldDescriptor({
    label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
    type: protobuf.FieldDescriptor.TYPE_STRING,
    name: "field1",
    number: 42});
  var o2 = new protobuf.OneofDescriptor("MyOneof", [o2_f1]);
  assert.equal(o2.name, "MyOneof");
  assert.equal(o2.fields.length, 1);
  assert.equal(o2.fields[0], o2_f1);

  // Oneof: validation failures.
  assert.throws(function() {
    var o = new protobuf.OneofDescriptor(null);
  });
  assert.throws(function() {
    var o = new protobuf.OneofDescriptor("MyOneof", new Object());
  });
  assert.throws(function() {
    var o = new protobuf.OneofDescriptor("MyOneof", ["f1", "f2"]);
  });

  // Message: one-arg form.
  var m1 = new protobuf.Descriptor("MyMessage");
  assert.equal(m1.name, "MyMessage");
  assert.equal(m1.fields.length, 0);
  assert.equal(m1.oneofs.length, 0);

  // Message: two-arg form.
  var m2_f1 = new protobuf.FieldDescriptor({
    label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
    type: protobuf.FieldDescriptor.TYPE_STRING,
    name: "field1",
    number: 42});
  var m2 = new protobuf.Descriptor("MyMessage", [m2_f1]);
  assert.equal(m2.name, "MyMessage");
  assert.equal(m2.fields.length, 1);
  assert.equal(m2.fields[0], m2_f1);
  assert.equal(m2.oneofs.length, 0);

  // Message: three-arg form.
  var m3_f1 = new protobuf.FieldDescriptor({
    label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
    type: protobuf.FieldDescriptor.TYPE_STRING,
    name: "field1",
    number: 42});
  var m3_f2 = new protobuf.FieldDescriptor({
    label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
    type: protobuf.FieldDescriptor.TYPE_STRING,
    name: "field2",
    number: 43});
  var m3_f3 = new protobuf.FieldDescriptor({
    label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
    type: protobuf.FieldDescriptor.TYPE_STRING,
    name: "field3",
    number: 44});
  var m3_f4 = new protobuf.FieldDescriptor({
    label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
    type: protobuf.FieldDescriptor.TYPE_STRING,
    name: "field4",
    number: 45});
  var m3_o1 = new protobuf.OneofDescriptor("MyOneof", [m3_f1, m3_f2]);
  var m3 = new protobuf.Descriptor("MyMessage", [m3_f3, m3_f4], [m3_o1]);
  assert.equal(m3.name, "MyMessage");
  assert.equal(m3.oneofs.length, 1);
  assert.equal(m3.oneofs[0], m3_o1);
  assert.equal(m3.fields.length, 4);
  assert.equal(m3.fields[0], m3_f1);
  assert.equal(m3.fields[1], m3_f2);
  assert.equal(m3.fields[2], m3_f3);
  assert.equal(m3.fields[3], m3_f4);

  // Message: validation failure tests.
  assert.throws(function() {
    var m = new protobuf.Descriptor(null);
  });
  assert.throws(function() {
    var m = new protobuf.Descriptor("MyMessage", new Object());
  });
  assert.throws(function() {
    var m = new protobuf.Descriptor("MyMessage", [], new Object());
  });
  assert.throws(function() {
    var m = new protobuf.Descriptor("MyMessage", ["f1", "f2"]);
  });
  assert.throws(function() {
    var m = new protobuf.Descriptor("MyMessage", [f1, f2], ["o1"]);
  });

  // Enum: one-arg form.
  var e1 = new protobuf.EnumDescriptor("MyEnum");
  assert.equal(e1.name, "MyEnum");
  // Enum: 2n+1-arg form.
  var e2 = new protobuf.EnumDescriptor("MyEnum",
      "Default", 0,
      "A", 1,
      "B", 2);
  assert.equal(e2.name, "MyEnum");
  assert.equal(e2.keys.length, 3);
  assert.equal(e2.findByName("A"), 1);
  assert.equal(e2.findByValue(2), "B");

  // Enum: validation failures.
  assert.throws(function() {
    var e = new protobuf.EnumDescriptor(null);
  });
  assert.throws(function() {
    var e = new protobuf.EnumDescriptor("MyEnum", "A");
  });
  assert.throws(function() {
    var e = new protobuf.EnumDescriptor("MyEnum", null, 1);
  });
  assert.throws(function() {
    var e = new protobuf.EnumDescriptor("MyEnum", "A", new Object());
  });
})();

(function() {
  // Test the usual generated-code descriptor setup style.
  protobuf.DescriptorPool.generatedPool.add([
      new protobuf.Descriptor("Message1",
        [ new protobuf.FieldDescriptor({
            label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
            type: protobuf.FieldDescriptor.TYPE_STRING,
            name: "field1", number: 1}),
          new protobuf.FieldDescriptor({
            label: protobuf.FieldDescriptor.LABEL_REPEATED,
            type: protobuf.FieldDescriptor.TYPE_BOOL,
            name: "field2", number: 2}),
          new protobuf.FieldDescriptor({
            label: protobuf.FieldDescriptor.LABEL_REPEATED,
            type: protobuf.FieldDescriptor.TYPE_MESSAGE,
            name: "field3", number: 3, subtype_name: "SubMessage"}),
          new protobuf.FieldDescriptor({
            label: protobuf.FieldDescriptor.LABEL_REPEATED,
            type: protobuf.FieldDescriptor.TYPE_ENUM,
            name: "field4", number: 4, subtype_name: "EnumType"}) ],
        [ new protobuf.OneofDescriptor("oneof1",
          [ new protobuf.FieldDescriptor({
              label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
              type: protobuf.FieldDescriptor.TYPE_STRING,
              name: "oneof_field_1", number: 5}),
            new protobuf.FieldDescriptor({
              label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
              type: protobuf.FieldDescriptor.TYPE_STRING,
              name: "oneof_field_2", number: 6}) ]) ]),
      new protobuf.Descriptor("SubMessage",
        [ new protobuf.FieldDescriptor({
            label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
            type: protobuf.FieldDescriptor.TYPE_STRING,
            name: "field1", number: 1}) ]),
      new protobuf.EnumDescriptor("EnumType",
          "Default", 0,
          "A", 1,
          "B", 2) ]);

  var p = protobuf.DescriptorPool.generatedPool;
  var m1 = p.lookup("Message1");
  var m2 = p.lookup("SubMessage");
  var e1 = p.lookup("EnumType");
  assert.equal(m1.name, "Message1");
  assert.equal(m2.name, "SubMessage");
  assert.equal(e1.name, "EnumType");
  var field3 = m1.findFieldByName("field3");
  assert.equal(field3.subtype, m2);
  var field4 = m1.findFieldByName("field4");
  assert.equal(field4.subtype, e1);
})();

console.log('defs.test.js PASS');
