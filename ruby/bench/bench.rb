require "benchmark/ips"
require "google/protobuf"
require_relative "bench_messages_pb"

message = BenchMessages::Message.new(
  field_1: "howdy",
  field_2: 123,
  field_3: { "a" => "1", "b" => "2", "c" => "3" },
  field_4: BenchMessages::SubMessage.new(field_1: 456),
)

def assert_equal(val1, val2)
  raise "#{val1} != #{val2}" unless val1 == val2
end

Benchmark.ips do |x|
  x.report("Message#[]") do
    assert_equal("howdy", message["field_1"])
    assert_equal(123, message["field_2"])
    assert_equal({ "a" => "1", "b" => "2", "c" => "3" }, message["field_3"].to_h)
    assert_equal(456, message["field_4"]["field_1"])
  end

  x.report("Message#get") do
    assert_equal("howdy", message.get(:field_1))
    assert_equal(123, message.get(:field_2))
    assert_equal({ "a" => "1", "b" => "2", "c" => "3" }, message.get(:field_3).to_h)
    assert_equal(456, message.get(:field_4).get(:field_1))
  end

  x.compare!
end
