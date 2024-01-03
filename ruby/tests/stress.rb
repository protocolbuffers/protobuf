#!/usr/bin/ruby

require 'google/protobuf'
require 'stress_pb'
require 'test/unit'

module StressTest
  pool = Google::Protobuf::DescriptorPool.new
  pool.build do
    add_message "TestMessage" do
      proto3_optional :a,  :int32,        1
      repeated :b,  :message,      2, "M"
    end
    add_message "M" do
      proto3_optional :foo, :string, 1
    end
  end

  TestMessage = pool.lookup("TestMessage").msgclass
  M = pool.lookup("M").msgclass

  class StressTest < Test::Unit::TestCase
    def get_msg
      TestMessage.new(:a => 1000,
                      :b => [M.new(:foo => "hello"),
                             M.new(:foo => "world")])
    end
    def test_stress
      m = get_msg
      data = TestMessage.encode(m)
      100_000.times do
        mnew = TestMessage.decode(data)
        mnew2 = mnew.dup
        assert_equal m.inspect, mnew.inspect
        assert_equal data, TestMessage.encode(mnew)
        assert_equal m.inspect, mnew2.inspect
        assert_equal data, TestMessage.encode(mnew2)
      end
    end
  end
end
