/**
 * @fileoverview Description of this file.
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
