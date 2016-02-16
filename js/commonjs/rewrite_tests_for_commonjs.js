/**
 * @fileoverview Utility to translate test files to CommonJS imports.
 *
 * This is a somewhat hacky tool designed to do one very specific thing.
 * All of the test files in *_test.js are written with Closure-style
 * imports (goog.require()).  This works great for running the tests
 * against Closure-style generated code, but we also want to run the
 * tests against CommonJS-style generated code without having to fork
 * the tests.
 *
 * Closure-style imports import each individual type by name.  This is
 * very different than CommonJS imports which are by file.  So we put
 * special comments in these tests like:
 *
 * // CommonJS-LoadFromFile: test_pb
 * goog.require('proto.jspb.test.CloneExtension');
 * goog.require('proto.jspb.test.Complex');
 * goog.require('proto.jspb.test.DefaultValues');
 *
 * This script parses that special comment and uses it to generate proper
 * CommonJS require() statements so that the tests can run and pass using
 * CommonJS imports.
 */

var lineReader = require('readline').createInterface({
  input: process.stdin,
  output: process.stdout
});

function tryStripPrefix(str, prefix) {
  if (str.lastIndexOf(prefix) !== 0) {
    throw "String: " + str + " didn't start with: " + prefix;
  }
  return str.substr(prefix.length);
}

var module = null;
var pkg = null;
lineReader.on('line', function(line) {
  var is_require = line.match(/goog\.require\('([^']*)'\)/);
  var is_loadfromfile = line.match(/CommonJS-LoadFromFile: ([^ ]*) (.*)/);
  var is_settestonly = line.match(/goog.setTestOnly()/);
  if (is_settestonly) {
    // Remove this line.
  } else if (is_require) {
    if (module) {  // Skip goog.require() lines before the first directive.
      var full_sym = is_require[1];
      var sym = tryStripPrefix(full_sym, pkg);
      console.log("googleProtobuf.exportSymbol('" + full_sym + "', " + module + sym + ', global);');
    }
  } else if (is_loadfromfile) {
    if (!module) {
      console.log("var googleProtobuf = require('google-protobuf');");
      console.log("var asserts = require('closure_asserts_commonjs');");
      console.log("var global = Function('return this')();");
      console.log("");
      console.log("// Bring asserts into the global namespace.");
      console.log("googleProtobuf.object.extend(global, asserts);");
    }
    module = is_loadfromfile[1].replace("-", "_");
    pkg = is_loadfromfile[2];

    if (module != "googleProtobuf") {  // We unconditionally require this in the header.
      console.log("var " + module + " = require('" + is_loadfromfile[1] + "');");
    }
  } else {
    console.log(line);
  }
});
