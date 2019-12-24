var root = require("./generated_bundle_code.js");
var fs = require('fs');
var benchmark = require("./node_modules/benchmark");
var benchmarkSuite = require("./benchmark_suite.js");


function getNewPrototype(name) {
  var message = eval("root." + name);
  if (typeof(message) == "undefined") {
    throw "type " + name + " is undefined";
  }
  return message;
}


var results = [];

console.log("#####################################################");
console.log("ProtobufJs Benchmark: ");
process.argv.forEach(function(filename, index) {
  if (index < 2) {
    return;
  }
  var benchmarkDataset =
      root.benchmarks.BenchmarkDataset.decode(fs.readFileSync(filename));
  var messageList = [];
  var totalBytes = 0;
  benchmarkDataset.payload.forEach(function(onePayload) {
    var message = getNewPrototype(benchmarkDataset.messageName);
    messageList.push(message.decode(onePayload));
    totalBytes += onePayload.length;
  });

  var senarios = benchmarkSuite.newBenchmark(
    benchmarkDataset.messageName, filename, "protobufjs");
  senarios.suite
  .add("protobuf.js static decoding", function() {
    benchmarkDataset.payload.forEach(function(onePayload) {
      var protoType = getNewPrototype(benchmarkDataset.messageName);
      protoType.decode(onePayload);
    });
  })
  .add("protobuf.js static encoding", function() {
    var protoType = getNewPrototype(benchmarkDataset.messageName);
    messageList.forEach(function(message) {
      protoType.encode(message).finish();
    });
  })
  .run({"Async": false});

  results.push({
    filename: filename,
    benchmarks: {
      protobufjs_decoding: senarios.benches[0] * totalBytes,
      protobufjs_encoding: senarios.benches[1] * totalBytes
    }
  })

  console.log("Throughput for decoding: "
    + senarios.benches[0] * totalBytes / 1024 / 1024 + "MB/s" );
  console.log("Throughput for encoding: "
    + senarios.benches[1] * totalBytes / 1024 / 1024 + "MB/s" );
  console.log("");
});
console.log("#####################################################");

