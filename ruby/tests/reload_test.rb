#!/usr/bin/ruby

require 'google/protobuf'
require 'test/unit'

class ReloadTest < Test::Unit::TestCase
  def test_reload_basic
    pool = Google::Protobuf::DescriptorPool.new
    pool.build do
      add_message "redefine.TestMessage" do
        optional :foo, :string, 1
        optional :bar, :string, 2
      end
    end

    m = pool.lookup("redefine.TestMessage").msgclass.new(foo: "foo", bar: "bar")
    assert m.foo == "foo"
    assert m.bar == "bar"

    pool.clear

    pool.build do
      add_message "redefine.TestMessage" do
        optional :foo, :int32, 1
        optional :qux, :int32, 2
      end
    end

    m2 = pool.lookup("redefine.TestMessage").msgclass.new(foo: 111, qux: 222)
    assert_raise NoMethodError do
      m2.bar
    end
    assert m2.foo == 111
    assert m2.qux == 222
  end
end
