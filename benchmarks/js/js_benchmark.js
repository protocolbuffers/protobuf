require('./datasets/google_message1/proto2/benchmark_message1_proto2_pb.js');
require('./datasets/google_message1/proto3/benchmark_message1_proto3_pb.js');
require('./datasets/google_message2/benchmark_message2_pb.js');
require('./datasets/google_message3/benchmark_message3_pb.js');
require('./datasets/google_message4/benchmark_message4_pb.js');
require('./benchmarks_pb.js');

var fs = require('fs');
var benchmarkSuite = require("./benchmark_suite.js");


function getNewPrototype(name) {
  var message = eval("proto." + name);
  if (typeof(message) == "undefined") {
    throw "type " + name + " is undefined";
  }
  return message;
}

var results = [];
var json_file = "";

console.log("#####################################################");
console.log("Js Benchmark: ");
process.argv.forEach(function(filename, index) {
  if (index < 2) {
    return;
  }
  if (filename.indexOf("--json_output") != -1) {
    json_file = filename.replace(/^--json_output=/, '');
    return;
  }

  var benchmarkDataset =
      proto.benchmarks.BenchmarkDataset.deserializeBinary(fs.readFileSync(filename));
  var messageList = [];
  var totalBytes = 0;
  benchmarkDataset.getPayloadList().forEach(function(onePayload) {
    var message = getNewPrototype(benchmarkDataset.getMessageName());
    messageList.push(message.deserializeBinary(onePayload));
    totalBytes += onePayload.length;
  });

  var senarios = benchmarkSuite.newBenchmark(
      benchmarkDataset.getMessageName(), filename, "js");
  senarios.suite
  .add("js deserialize", function() {
    benchmarkDataset.getPayloadList().forEach(function(onePayload) {
      var protoType = getNewPrototype(benchmarkDataset.getMessageName());
      protoType.deserializeBinary(onePayload);
    });
  })
  .add("js serialize", function() {
    var protoType = getNewPrototype(benchmarkDataset.getMessageName());
    messageList.forEach(function(message) {
      message.serializeBinary();
    });
  })
  .run({"Async": false});

  results.push({
    filename: filename,
    benchmarks: {
      protobufjs_decoding: senarios.benches[0] * totalBytes / 1024 / 1024,
      protobufjs_encoding: senarios.benches[1] * totalBytes / 1024 / 1024
    }
  })

  console.log("Throughput for deserialize: "
    + senarios.benches[0] * totalBytes / 1024 / 1024 + "MB/s" );
  console.log("Throughput for serialize: "
    + senarios.benches[1] * totalBytes / 1024 / 1024 + "MB/s" );
  console.log("");
});
console.log("#####################################################");

if (json_file != "") {
  fs.writeFile(json_file, JSON.stringify(results), (err) => {
    if (err) throw err;
  });
}

