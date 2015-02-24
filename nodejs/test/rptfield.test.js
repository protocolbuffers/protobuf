var protobuf = require('google_protobuf');
var assert = require('assert');

// Test a repeated field of int32s.
(function() {
  var r = new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_INT32);
  assert.equal(r.type, protobuf.FieldDescriptor.TYPE_INT32);
  assert.equal(r.length, 0);
  assert.equal(r[0], undefined);

  r.push(1);
  r.push(2);
  r.push(3);
  assert.equal(r.length, 3);
  assert.equal(r[0], 1);
  assert.equal(r[1], 2);
  assert.equal(r[2], 3);
  r.unshift(0);
  assert.equal(r.length, 4);
  assert.equal(r[0], 0);
  assert.equal(r[1], 1);
  assert.equal(r[2], 2);
  assert.equal(r[3], 3);

  assert.equal(r.shift(), 0);
  assert.equal(r.pop(), 3);
  assert.equal(r.length, 2);
  assert.equal(r.pop(), 2);
  assert.equal(r.shift(), 1);
  assert.equal(r.length, 0);

  r.push(0);
  assert.throws(function() {
    delete r[0];
  });

  r[0] = 42;
  assert.equal(r[0], 42);
  assert.throws(function() {
    r[10] = 100;
  });
})();

// Test a repeated field of int64s.
(function() {
  var r = new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_INT64);
  r.push(new protobuf.Int64(42));
  assert.equal(r.length, 1);
  assert.equal(protobuf.Int64.lo(r[0]), 42);
})();

// Test a repeated field of messages.
(function() {
  var pool = new protobuf.DescriptorPool();
  pool.add([
      new protobuf.Descriptor("TestMessage",
      [ new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
          type: protobuf.FieldDescriptor.TYPE_STRING,
          name: "optional_string", number: 1}) ]),
      new protobuf.Descriptor("TestMessage2",
      [ new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
          type: protobuf.FieldDescriptor.TYPE_STRING,
          name: "optional_string", number: 1}) ])]);
  var TestMessage = pool.lookup("TestMessage").msgclass;
  var TestMessage2 = pool.lookup("TestMessage2").msgclass;

  var r = new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_MESSAGE,
                                     TestMessage);
  assert.equal(r.type, protobuf.FieldDescriptor.TYPE_MESSAGE);
  assert.equal(r.subdesc, TestMessage.descriptor);

  r.push(new TestMessage());
  assert.equal(r[0].optional_string, "");

  assert.throws(function() {
    r[0] = new TestMessage2();
  });

  // Creation with the descriptor object should work too.
  r = new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_MESSAGE,
                                 TestMessage.descriptor);
  r.push(new TestMessage());
})();

// Test a repeated field of enums.
(function() {
  var pool = new protobuf.DescriptorPool();
  pool.add([
      new protobuf.EnumDescriptor("TestEnum", "Default", 0, "A", 1, "B", 2)]);
  var TestEnum = pool.lookup("TestEnum").enumobject;

  var r = new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_ENUM,
                                     TestEnum);
  assert.equal(r.type, protobuf.FieldDescriptor.TYPE_ENUM);
  assert.equal(r.subdesc, TestEnum.descriptor);

  r.push(TestEnum.A);
  r.push(TestEnum.B);
  assert.equal(r.toString(), "[A, B]");
})();

// Test resize().
(function() {
  var pool = new protobuf.DescriptorPool();
  pool.add([
      new protobuf.Descriptor("TestMessage",
      [ new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
          type: protobuf.FieldDescriptor.TYPE_STRING,
          name: "optional_string", number: 1}) ])]);
  var TestMessage = pool.lookup("TestMessage").msgclass;

  var r = new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_MESSAGE,
                                     TestMessage);

  r.resize(10);
  assert.equal(r.length, 10);
  assert.equal(r[0].optional_string, "");
  assert.equal(r[5].optional_string, "");
  assert.equal(r[9].optional_string, "");

  var r2 = new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_INT32);
  r2.resize(10);
  assert.equal(r2.length, 10);
  assert.equal(r2[5], 0);

  var r3 = new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_STRING);
  r3.resize(10);
  assert.equal(r3.length, 10);
  assert.equal(r3[5], "");
})();

// Test newEmpty().
(function() {
  var pool = new protobuf.DescriptorPool();
  pool.add([
      new protobuf.Descriptor("TestMessage",
      [ new protobuf.FieldDescriptor({
          label: protobuf.FieldDescriptor.LABEL_OPTIONAL,
          type: protobuf.FieldDescriptor.TYPE_STRING,
          name: "optional_string", number: 1}) ]),
      new protobuf.EnumDescriptor("TestEnum",
        "Default", 0, "A", 1)]);
  var TestMessage = pool.lookup("TestMessage").msgclass;
  var TestEnum = pool.lookup("TestEnum").enumobject;

  var r = new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_INT32,
                                     [1, 2, 3]);
  var r2 = r.newEmpty();
  assert.equal(r2.type, r.type);
  assert.equal(r2.length, 0);

  var r3 = new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_MESSAGE,
                                      TestMessage,
                                      [ new TestMessage({optional_string: "asdf"}) ]);
  var r4 = r3.newEmpty();
  assert.equal(r4.type, r3.type);
  assert.equal(r4.subdesc, r3.subdesc);
  assert.equal(r4.length, 0);

  var r5 = new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_ENUM,
                                      TestEnum,
                                      [ TestEnum.A, TestEnum.Default ]);
  var r6 = r5.newEmpty();
  assert.equal(r6.type, r5.type);
  assert.equal(r6.subdesc, r5.subdesc);
  assert.equal(r6.length, 0);
})();

// Test methods pulled in from Array.
(function() {
  var r = new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_INT32,
                                     [0, 1, 2, 3, 4]);
  var r2 = new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_INT32,
                                     [5, 6, 7, 8, 9]);

  var concated = r.concat(r2);
  assert.equal(concated.length, 10);
  assert.equal(concated[4], 4);
  assert.equal(concated[5], 5);

  var r3 = new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_INT32,
                                     [0, 1, 2, 3, 4]);
  r3.copyWithin(0, 3);
  assert.equal(r3[0], 3);
  assert.equal(r3[1], 4);
  assert.equal(r3[2], 2);
  assert.equal(r3[3], 3);
  assert.equal(r3[4], 4);

  assert.equal(r.every(function(i) { return i < 5; }), true);
  assert.equal(r.every(function(i) { return i < 4; }), false);

  r3.fill(42, 0, 5);
  assert.equal(r3[0], 42);
  assert.equal(r3[4], 42);

  var filtered = r.filter(function(i) { return i > 2; });
  assert.equal(filtered.length, 2);
  assert.equal(filtered[0], 3);
  assert.equal(filtered[1], 4);

  assert.equal(r2.find(function(i) { return i % 3 == 1; }), 7);
  assert.equal(r2.findIndex(function(i) { return i % 3 == 1; }), 2);

  var sum = 0;
  r.forEach(function(i) { sum += i; });
  assert.equal(sum, 10);

  assert.equal(r2.includes(7), true);
  assert.equal(r2.includes(7, 3), false);
  assert.equal(r2.includes(42), false);

  var r4 = new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_DOUBLE);

  var nan = 0.0 / 0.0;
  r4.push(nan);
  assert.equal(r4.includes(nan), true);
  r4.pop();

  assert.equal(r2.indexOf(7), 2);
  assert.equal(r2.indexOf(7, 3), -1);

  assert.equal(r2.join(), "5,6,7,8,9");
  assert.equal(r2.join(":"), "5:6:7:8:9");

  r2.push(5);
  assert.equal(r2.lastIndexOf(5), 5);
  assert.equal(r2.indexOf(5), 0);
  r2.pop();

  var mapped = r2.map(function(i) { return i * 2; });
  assert.equal(mapped.length, 5);
  assert.equal(mapped[0], 10);
  assert.equal(mapped[4], 18);

  assert.equal(
      r2.reduce(function(accum, val, idx, arr) { return accum + val; }),
      35);
  assert.equal(
      r2.reduceRight(function(accum, val, idx, arr) {
                        return accum + "," + val;
                     }, "list:"),
      "list:,9,8,7,6,5");

  r2.reverse();
  assert.equal(r2.length, 5);
  assert.equal(r2[0], 9);
  assert.equal(r2[4], 5);
  r2.reverse();

  var sliced = r2.slice(1, 4);
  assert.equal(sliced.length, 3);
  assert.equal(sliced.type, r2.type);
  assert.equal(sliced.subdesc, r2.subdesc);
  assert.equal(sliced[0], 6);
  assert.equal(sliced[1], 7);
  assert.equal(sliced[2], 8);

  assert.equal(r2.some(function(i) { return i > 7; }), true);
  assert.equal(r2.some(function(i) { return i > 10; }), false);

  var sortlist = new protobuf.RepeatedField(protobuf.FieldDescriptor.TYPE_INT32);
  var randseq = 1;
  for (var i = 0; i < 10000; i++) {
    randseq = (randseq * 2719) % 10000; // 2719 is prime
    sortlist.push(randseq);
  }
  // Provide a numeric comparator because by default, .sort() must sort by
  // string comparison.
  sortlist.sort(function(a, b) { return a - b; });

  for (var i = 0; i < 10000 - 1; i++) {
    assert.equal(sortlist[i + 1] >= sortlist[i], true);
  }

  var del = r2.splice(1, 2);
  assert.equal(del.length, 2);
  assert.equal(del[0], 6);
  assert.equal(del[1], 7);
  assert.equal(r2.length, 3);
  assert.equal(r2[0], 5);
  assert.equal(r2[1], 8);
  assert.equal(r2[2], 9);

  del = r2.splice(1, 0, 6, 7);
  assert.equal(del.length, 0);
  assert.equal(r2.length, 5);
  assert.equal(r2[0], 5);
  assert.equal(r2[1], 6);
  assert.equal(r2[2], 7);
  assert.equal(r2[3], 8);
  assert.equal(r2[4], 9);

  del = r2.splice(0, 3, 0, 1);
  assert.equal(del.length, 3);
  assert.equal(r2.length, 4);
  assert.equal(r2[0], 0);
  assert.equal(r2[1], 1);
  assert.equal(r2[2], 8);
  assert.equal(r2[3], 9);

  del = r2.splice(0, 2, 5, 6, 7);
  assert.equal(r2.length, 5);
  assert.equal(r2[0], 5);
  assert.equal(r2[1], 6);
  assert.equal(r2[2], 7);
  assert.equal(r2[3], 8);
  assert.equal(r2[4], 9);
})();

console.log("rptfield.test.js PASS");
