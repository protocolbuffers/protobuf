require 'benchmark/ips'
require 'google/protobuf'
require_relative './bench_messages_pb'

Benchmark.ips do |x|
  x.report("C kwargs initializer (baseline)") do
    BenchMessage.new(field_1: "howdy", field_2: 123)
  end

  x.report("C positional args initializer") do
    BenchMessage.new("howdy", 123)
  end

  x.compare!
end
