var benchmark = require("benchmark");

function newBenchmark(messageName, filename, language) {
  var benches = [];
  return {
    suite: new benchmark.Suite(messageName + filename + language )
      .on("add", function(event) {
          benches.push(event.target);
      })
      .on("start", function() {
          process.stdout.write(
            "benchmarking message " + messageName
            + " of dataset file " + filename
            + "'s performance ..." + "\n\n");
      })
      .on("cycle", function(event) {
          process.stdout.write(String(event.target) + "\n");
      })
      .on("complete", function() {
          var getHz = function(bench) {
            return 1 / (bench.stats.mean + bench.stats.moe);
          }
          benches.forEach(function(val, index) {
            benches[index] = getHz(val);
          });
      }),
     benches: benches
  }
}

module.exports = {
        newBenchmark: newBenchmark
}
