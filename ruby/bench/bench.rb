require "benchmark/ips"
require "google/protobuf"
require_relative "bench_messages_pb"

message = BenchMessages::Message.new(
  bigger_fld_1: "howdy",
  bigger_fld_2: 123,
  bigger_fld_3: { "a" => "1", "b" => "2", "c" => "3" },
  bigger_fld_4: BenchMessages::SubMessage.new(bigger_fld_1: 456),
)

def assert_equal(val1, val2)
  raise "#{val1} != #{val2}" unless val1 == val2
end

Benchmark.ips do |x|
  x.report("Message#[]") do
    assert_equal("howdy", message["bigger_fld_1"])
    assert_equal(123, message["bigger_fld_2"])
    assert_equal({ "a" => "1", "b" => "2", "c" => "3" }, message["bigger_fld_3"].to_h)
    assert_equal(456, message["bigger_fld_4"]["bigger_fld_1"])
  end

  x.report("Message#get (symbols)") do
    assert_equal("howdy", message.get(:bigger_fld_1))
    assert_equal(123, message.get(:bigger_fld_2))
    assert_equal({ "a" => "1", "b" => "2", "c" => "3" }, message.get(:bigger_fld_3).to_h)
    assert_equal(456, message.get(:bigger_fld_4).get(:bigger_fld_1))
  end

  x.report("Message#get (strings)") do
    assert_equal("howdy", message.get("bigger_fld_1"))
    assert_equal(123, message.get("bigger_fld_2"))
    assert_equal({ "a" => "1", "b" => "2", "c" => "3" }, message.get("bigger_fld_3").to_h)
    assert_equal(456, message.get("bigger_fld_4").get("bigger_fld_1"))
  end

  x.compare!
end
