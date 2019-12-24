/**
 * @fileoverview Exports symbols needed only by tests.
 *
 * This file exports several Closure Library symbols that are only
 * used by tests.  It is used to generate a file
 * closure_asserts_commonjs.js that is only used at testing time.
 */

goog.require('goog.testing.asserts');

var global = Function('return this')();

// All of the closure "assert" functions are exported at the global level.
//
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

// The COMPILED variable is set by Closure compiler to "true" when it compiles
// JavaScript, so in practice this is equivalent to "exports.COMPILED = true".
// This will disable some debugging functionality in debug.js.  We could
// investigate whether this can/should be enabled in CommonJS builds.
exports.COMPILED = COMPILED
