#!/usr/bin/ruby

require 'google/protobuf'
require 'test/unit'

module StressTest
  pool = Google::Protobuf::DescriptorPool.new
  pool.build do
    add_message "TestMessage" do
      optional :a,  :int32,        1
      repeated :b,  :message,      2, "M"
    end
    add_message "M" do
      optional :foo, :string, 1
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
        mnew = mnew.dup
        assert_equal mnew.inspect, m.inspect
        assert TestMessage.encode(mnew) == data
      end
    end
  end
end
