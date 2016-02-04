/**
 * @fileoverview Description of this file.
 */

goog.require('goog.testing.asserts');

var global = Function('return this')();

// The Google Closure assert functions start with assert, eg.
//   assertThrows
//   assertNotThrows
//   assertTrue
//   ...
//
// The one exception is the "fail" function.
function shouldExport(str) {
  return str.lastIndexOf('assert') === 0 || str == 'fail';
}

for (var key in global) {
  if ((typeof key == "string") && global.hasOwnProperty(key) &&
      shouldExport(key)) {
    exports[key] = global[key];
  }
}

exports.COMPILED = COMPILED
