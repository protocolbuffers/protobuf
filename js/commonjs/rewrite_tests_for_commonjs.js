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
 * CommonJS imports.  The script will change the above statements into:
 *
 *   var test_pb = require('test_pb');
 *   googleProtobuf.exportSymbol('proto.jspb.test.CloneExtension', test_pb.CloneExtension, global);
 *   googleProtobuf.exportSymbol('proto.jspb.test.Complex', test_pb.Complex, global);
 *   googleProtobuf.exportSymbol('proto.jspb.test.DefaultValues', test_pb.DefaultValues, global);
 *
 * (The "exportSymbol" function will define the given names in the global
 * namespace, taking care not to overwrite any previous value for
 * "proto.jspb.test").
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

function camelCase(str) {
  var ret = '';
  var ucaseNext = false;
  for (var i = 0; i < str.length; i++) {
    if (str[i] == '-') {
      ucaseNext = true;
    } else if (ucaseNext) {
      ret += str[i].toUpperCase();
      ucaseNext = false;
    } else {
      ret += str[i];
    }
  }
  return ret;
}

var module = null;
var pkg = null;
lineReader.on('line', function(line) {
  var isRequire = line.match(/goog\.require\('([^']*)'\)/);
  var isLoadFromFile = line.match(/CommonJS-LoadFromFile: (\S*) (.*)/);
  var isSetTestOnly = line.match(/goog.setTestOnly()/);
  if (isRequire) {
    if (module) {  // Skip goog.require() lines before the first directive.
      var fullSym = isRequire[1];
      var sym = tryStripPrefix(fullSym, pkg);
      console.log("googleProtobuf.exportSymbol('" + fullSym + "', " + module + sym + ', global);');
    }
  } else if (isLoadFromFile) {
    if (!module) {
      console.log("var googleProtobuf = require('google-protobuf');");
      console.log("var asserts = require('closure_asserts_commonjs');");
      console.log("var global = Function('return this')();");
      console.log("");
      console.log("// Bring asserts into the global namespace.");
      console.log("googleProtobuf.object.extend(global, asserts);");
    }
    module = camelCase(isLoadFromFile[1])
    pkg = isLoadFromFile[2];

    if (module != "googleProtobuf") {  // We unconditionally require this in the header.
      console.log("var " + module + " = require('" + isLoadFromFile[1] + "');");
    }
  } else if (!isSetTestOnly) {  // Remove goog.setTestOnly() lines.
    console.log(line);
  }
});
