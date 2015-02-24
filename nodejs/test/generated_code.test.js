var protobuf = require('google_protobuf');
var assert = require('assert');
var generated_code = require('./generated_code');

(function() {
  var m = new generated_code.TestMessage({
    optional_int32: 42,
    optional_bool: true,
    optional_string: "hello world",
    repeated_string: ["a", "b", "c"],
    repeated_msg: [new generated_code.TestMessage({optional_int32:1})]
  });

  assert.equal(m.optional_int32, 42);
  assert.equal(m.optional_string, "hello world");
  assert.equal(m.repeated_string[2], "c");
  assert.equal(m.repeated_msg[0].optional_int32, 1);

  m.oneof_int32 = 42;
  m.oneof_bool = true;
  assert.equal(m.oneof_int32, undefined);
  assert.equal(m.my_oneof, "oneof_bool");

  var n = new generated_code.TestMessage.NestedMessage({foo: 42});
  assert.equal(n.foo, 42);

  assert.equal(generated_code.TestEnum.C, 3);
  assert.equal(generated_code.TestEnum.descriptor.name, "A.B.C.TestEnum");
})();

console.log("generated_code.test.js PASS");
