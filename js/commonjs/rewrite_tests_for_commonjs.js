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

var module = null;
lineReader.on('line', function(line) {
  var is_require = line.match(/goog\.require\('([^']*\.)([^'.]+)'\)/);
  var is_loadfromfile = line.match(/CommonJS-LoadFromFile: (.*)/);
  var is_settestonly = line.match(/goog.setTestOnly()/);
  if (is_settestonly) {
    // Remove this line.
  } else if (is_require) {
    if (module) {  // Skip goog.require() lines before the first directive.
      var pkg = is_require[1];
      var sym = is_require[2];
      console.log("google_protobuf.exportSymbol('" + pkg + sym + "', " + module + "." + sym + ', global);');
    }
  } else if (is_loadfromfile) {
    if (!module) {
      console.log("var asserts = require('closure_asserts_commonjs');");
      console.log("var global = Function('return this')();");
      console.log("");
      console.log("// Bring asserts into the global namespace.");
      console.log("for (var key in asserts) {");
      console.log("  if (asserts.hasOwnProperty(key)) {");
      console.log("    global[key] = asserts[key];");
      console.log("  }");
      console.log("}");
      console.log("");
      console.log("var google_protobuf = require('google-protobuf');");
    }
    module = is_loadfromfile[1].replace("-", "_");

    if (module != "google_protobuf") {  // We unconditionally require this in the header.
      console.log("var " + module + " = require('" + is_loadfromfile[1] + "');");
    }
  } else {
    console.log(line);
  }
});
