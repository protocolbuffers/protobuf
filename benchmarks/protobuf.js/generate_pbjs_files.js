var pbjs = require("./protobuf.js/cli").pbjs

var argv = [];
var protoFiles = [];
var prefix = "";
process.argv.forEach(function(val, index) {
  var arg = val;
  if (arg.length > 6 && arg.substring(arg.length - 6) == ".proto") {
    protoFiles.push(arg);
  } else if (arg.length > 15 && arg.substring(0, 15) ==  "--include_path=") {
    prefix = arg.substring(15);
  } else if (index >= 2) {
    argv.push(arg);
  }
});
protoFiles.forEach(function(val) {
  argv.push(prefix + "/" + val);
});

pbjs.main(argv, function(err, output){
  if (err) {
    console.log(err);
  }
});

