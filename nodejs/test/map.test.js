var protobuf = require('google_protobuf');
var assert = require('assert');
var testmsgs = require('./generated_code');

// Shorthand for field type constants.
var f = protobuf.FieldDescriptor;

// Test (int32, int32) map.
(function() {
  var m = new protobuf.Map(f.TYPE_INT32, f.TYPE_INT32);
  assert.equal(m.keys.length, 0);
  assert.equal(m.values.length, 0);

  m.set(42, 1);
  assert.equal(m.get(42), 1);
  m.set(-0x10000000, 100);
  assert.equal(m.get(-0x10000000),  100);

  for (var i = 0; i < 100; i++) {
    m.set(i, i + 3);
  }
  for (var i = 0; i < 100; i++) {
    assert.equal(m.get(i), i + 3);
  }

  assert.equal(m.get(1000), undefined);

  assert.throws(function() {
    m.set("asdf", 1);
  });
  assert.throws(function() {
    m.set("42", 1);
  });
  assert.throws(function() {
    m.set(42.1, 1);
  });
  assert.throws(function() {
    m.get(42.1);
  });

  m.clear();
  m.set(42, 1);
  assert.equal(m.get(42), 1);
  assert.equal(m.has(42), true);
  m.delete(42);
  assert.equal(m.get(42), undefined);
  assert.equal(m.has(42), false);

  m.clear();
  assert.equal(m.keys.length, 0);
  for (var i = 0; i < 100; i++) {
    m.set(i, i + 20);
  }
  assert.equal(m.keys.length, 100);
  var keys_sum = 0;
  for (var it = m.keys.next(); !it.done; it = it.next()) {
    keys_sum += it.value;
  }
  assert.equal(keys_sum, 99 * 100 / 2);

  assert.equal(m.values.length, 100);
  var values_sum = 0;
  for (var it = m.values.next(); !it.done; it = it.next()) {
    values_sum += it.value;
  }
  assert.equal(values_sum, (99 * 100 / 2) + (20 * 100));
})();

// Test a map with int64 keys.
(function() {
  var m = new protobuf.Map(f.TYPE_INT64, f.TYPE_INT32);

  m.set(protobuf.Int64.join(42, 0), 1234);
  assert.equal(m.has(protobuf.Int64.join(42, 0)), true);
  assert.equal(m.has(protobuf.Int64.join(42, 1)), false);
  assert.equal(m.get(protobuf.Int64.join(42, 0)), 1234);
  m.set(protobuf.Int64.join(42, 1), 5678);

  assert.throws(function() {
    m.set(42, 0);
  });
  assert.throws(function() {
    m.set(new protobuf.UInt64(42, 0), 0);
  });

  m.delete(protobuf.Int64.join(42, 0));
  assert.equal(m.keys.length, 1);
  assert.equal(protobuf.Int64.hi(m.keys[0]), 42);
  assert.equal(protobuf.Int64.lo(m.keys[0]), 1);

  assert.equal(m.toString(), "[ { key: 180388626433 value: 5678 } ]");
})();

// Test a map with bool keys.
(function() {
  var m = new protobuf.Map(f.TYPE_BOOL, f.TYPE_INT32);

  m.set(true, 2);
  m.set(false, 3);
  assert.equal(m.has(true), true);
  assert.equal(m.has(false), true);
  assert.equal(m.get(true), 2);
  assert.equal(m.get(false), 3);
  assert.equal(m.keys.length, 2);
  assert.equal(m.values.length, 2);

  assert.throws(function() {
    m.set(1, 2);
  });

  // Relies on ordered (RB-tree or similar) map underlying implementation so
  // that keys are sorted, and that false < true in the key mapping.
  // Revise this if this assumption ever changes.
  assert.equal(m.toString(),
               "[ { key: false value: 3 }, { key: true value: 2 } ]");
})();

// Test a map with string keys.
(function() {
  var m = new protobuf.Map(f.TYPE_STRING, f.TYPE_INT32);

  m.set("asdf", 1);
  m.set("jkl;", 2);
  assert.equal(m.has("asdf"), true);
  assert.equal(m.has("qwerty"), false);
  assert.equal(m.get("asdf"), 1);
  assert.equal(m.get("jkl;"), 2);

  m.clear();
  m.set("\u1234", 42);
  assert.equal(m.has("\u1234"), true);
  assert.equal(m.keys.length, 1);
  assert.equal(m.keys[0], "\u1234");

  assert.throws(function() {
    m.set(1, 2);
  });

  // This tests Unicode passthrough in the key.
  assert.equal(m.toString(), "[ { key: \"\u1234\" value: 42 } ]");
})();

// Test a map with string keys and message values.
(function() {
  var m = new protobuf.Map(f.TYPE_STRING, f.TYPE_MESSAGE,
                           testmsgs.TestMessage);
  assert.equal(m.keyType, f.TYPE_STRING);
  assert.equal(m.valueType, f.TYPE_MESSAGE);
  assert.equal(m.valueSubDesc, testmsgs.TestMessage.descriptor);

  var submsg = new testmsgs.TestMessage({ optional_int32: 42 });
  m.set("message1", submsg);
  assert.equal(m.has("message1"), true);
  assert.equal(m.get("message1"), submsg);

  assert.throws(function() {
    m.set("message3", new testmsgs.TestMessage.NestedMessage());
  });

  m.delete("message2");

  assert.equal(m.toString(), "[ { key: \"message1\" value: " + submsg.toString() + " } ]");

  var m2 = m.newEmpty();
  assert.equal(m2.keyType, m.keyType);
  assert.equal(m2.valueType, m.valueType);
  assert.equal(m2.valueSubDesc, m.valueSubDesc);
})();

// Test a map with string keys and enum values.
(function() {
  var m = new protobuf.Map(f.TYPE_STRING, f.TYPE_ENUM,
                           testmsgs.TestEnum);
  assert.equal(m.keyType, f.TYPE_STRING);
  assert.equal(m.valueType, f.TYPE_ENUM);
  assert.equal(m.valueSubDesc, testmsgs.TestEnum.descriptor);

  m.set("val1", testmsgs.TestEnum.A);
  m.set("val2", testmsgs.TestEnum.B);
  m.set("val3", 1000);

  assert.equal(m.get("val1"), testmsgs.TestEnum.A);
  assert.equal(m.get("val2"), testmsgs.TestEnum.B);
  assert.equal(m.get("val3"), 1000);

  m.clear();
  m.set("val1", testmsgs.TestEnum.A);
  assert.equal(m.toString(), "[ { key: \"val1\" value: A } ]");
})();

// Test maps with invalid parameters (float/enum/message keys, etc).
(function() {
  assert.throws(function() {
    var m = new protobuf.Map();
  });
  assert.throws(function() {
    var m = new protobuf.Map(f.TYPE_STRING, f.TYPE_ENUM);
  });
  assert.throws(function() {
    var m = new protobuf.Map(f.TYPE_STRING, f.TYPE_MESSAGE);
  });
  assert.throws(function() {
    var m = new protobuf.Map(f.TYPE_STRING, f.TYPE_INT32, testmsgs.TestMessage);
  });
  assert.throws(function() {
    var m = new protobuf.Map(f.TYPE_FLOAT, f.TYPE_INT32);
  });
  assert.throws(function() {
    var m = new protobuf.Map(f.TYPE_DOUBLE, f.TYPE_INT32);
  });
  assert.throws(function() {
    var m = new protobuf.Map(f.TYPE_ENUM, f.TYPE_INT32);
  });
  assert.throws(function() {
    var m = new protobuf.Map(f.TYPE_MESSAGE, f.TYPE_INT32);
  });
})();

// Test constructor args.
(function() {
  var m = new protobuf.Map(f.TYPE_STRING, f.TYPE_INT32,
                { "val1": 42, "val2": 84, "val3": 100 });
  assert.equal(m.keys.length, 3);
  assert.equal(m.get("val1"), 42);
  assert.equal(m.get("val2"), 84);
  assert.equal(m.get("val3"), 100);

  var submsg1 = new testmsgs.TestMessage({optional_int32: 42});
  var submsg2 = new testmsgs.TestMessage({optional_int32: 84});
  var m = new protobuf.Map(f.TYPE_STRING, f.TYPE_MESSAGE, testmsgs.TestMessage,
                { "msg1": submsg1, "msg2": submsg2 });
  assert.equal(m.keys.length, 2);
  assert.equal(m.get("msg1"), submsg1);
  assert.equal(m.get("msg2"), submsg2);

  assert.throws(function() {
    var m = new protobuf.Map(f.TYPE_STRING, f.TYPE_MESSAGE,
                  testmsgs.TestMessage,
                  { "msg1": new testmsgs.TestMessage.NestedMessage() });
  });
})();

// Test exceptions.
(function() {
  var m = new protobuf.Map(f.TYPE_STRING, f.TYPE_INT32);
  assert.throws(function() {
    m.asdf = 42;
  });
  assert.throws(function() {
    m["asdf"] = 42;
  });

  var m2 = new protobuf.Map(f.TYPE_INT32, f.TYPE_INT32);
  assert.throws(function() {
    m[42] = 84;
  });

})();

// Test Map.forEach.
(function() {
  var m = new protobuf.Map(f.TYPE_STRING, f.TYPE_INT32,
                           { "a": 1,
                             "b": 2 });
  var sum = 0;
  var keys = "";
  m.forEach(function(k, v, mp) {
    assert.equal(mp, m);
    keys += k;
    sum += v;
  });
  assert.equal((keys == "ab" || keys == "ba"), true);
  assert.equal(sum, 3);
})();

console.log("map.test.js PASS");
