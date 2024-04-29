#!/usr/bin/ruby

require 'google/protobuf'
require 'test/unit'

# ------------- generated code --------------

module BasicTest
  pool = Google::Protobuf::DescriptorPool.new
  pool.build do
    add_message "Foo" do
      optional :bar, :message, 1, "Bar"
      repeated :baz, :message, 2, "Baz"
    end

    add_message "Bar" do
      optional :msg, :string, 1
    end

    add_message "Baz" do
      optional :msg, :string, 1
    end

    add_message "TestMessage" do
      optional :optional_int32,  :int32,        1
      optional :optional_int64,  :int64,        2
      optional :optional_uint32, :uint32,       3
      optional :optional_uint64, :uint64,       4
      optional :optional_bool,   :bool,         5
      optional :optional_float,  :float,        6
      optional :optional_double, :double,       7
      optional :optional_string, :string,       8
      optional :optional_bytes,  :bytes,        9
      optional :optional_msg,    :message,      10, "TestMessage2"
      optional :optional_enum,   :enum,         11, "TestEnum"

      repeated :repeated_int32,  :int32,        12
      repeated :repeated_int64,  :int64,        13
      repeated :repeated_uint32, :uint32,       14
      repeated :repeated_uint64, :uint64,       15
      repeated :repeated_bool,   :bool,         16
      repeated :repeated_float,  :float,        17
      repeated :repeated_double, :double,       18
      repeated :repeated_string, :string,       19
      repeated :repeated_bytes,  :bytes,        20
      repeated :repeated_msg,    :message,      21, "TestMessage2"
      repeated :repeated_enum,   :enum,         22, "TestEnum"
    end
    add_message "TestMessage2" do
      optional :foo, :int32, 1
    end

    add_message "Recursive1" do
      optional :foo, :message, 1, "Recursive2"
    end
    add_message "Recursive2" do
      optional :foo, :message, 1, "Recursive1"
    end

    add_enum "TestEnum" do
      value :Default, 0
      value :A, 1
      value :B, 2
      value :C, 3
    end

    add_message "BadFieldNames" do
      optional :dup, :int32, 1
      optional :class, :int32, 2
    end

    add_message "MapMessage" do
      map :map_string_int32, :string, :int32, 1
      map :map_string_msg, :string, :message, 2, "TestMessage2"
    end
    add_message "MapMessageWireEquiv" do
      repeated :map_string_int32, :message, 1, "MapMessageWireEquiv_entry1"
      repeated :map_string_msg, :message, 2, "MapMessageWireEquiv_entry2"
    end
    add_message "MapMessageWireEquiv_entry1" do
      optional :key, :string, 1
      optional :value, :int32, 2
    end
    add_message "MapMessageWireEquiv_entry2" do
      optional :key, :string, 1
      optional :value, :message, 2, "TestMessage2"
    end

    add_message "OneofMessage" do
      oneof :my_oneof do
        optional :a, :string, 1
        optional :b, :int32, 2
        optional :c, :message, 3, "TestMessage2"
        optional :d, :enum, 4, "TestEnum"
      end
    end
  end

  Foo = pool.lookup("Foo").msgclass
  Bar = pool.lookup("Bar").msgclass
  Baz = pool.lookup("Baz").msgclass
  TestMessage = pool.lookup("TestMessage").msgclass
  TestMessage2 = pool.lookup("TestMessage2").msgclass
  Recursive1 = pool.lookup("Recursive1").msgclass
  Recursive2 = pool.lookup("Recursive2").msgclass
  TestEnum = pool.lookup("TestEnum").enummodule
  BadFieldNames = pool.lookup("BadFieldNames").msgclass
  MapMessage = pool.lookup("MapMessage").msgclass
  MapMessageWireEquiv = pool.lookup("MapMessageWireEquiv").msgclass
  MapMessageWireEquiv_entry1 =
    pool.lookup("MapMessageWireEquiv_entry1").msgclass
  MapMessageWireEquiv_entry2 =
    pool.lookup("MapMessageWireEquiv_entry2").msgclass
  OneofMessage = pool.lookup("OneofMessage").msgclass

# ------------ test cases ---------------

  class MessageContainerTest < Test::Unit::TestCase

    def test_defaults
      m = TestMessage.new
      assert_equal 0, m.optional_int32
      assert_equal 0, m.optional_int64
      assert_equal 0, m.optional_uint32
      assert_equal 0, m.optional_uint64
      refute m.optional_bool
      assert_equal 0.0, m.optional_float
      assert_equal 0.0, m.optional_double
      assert_empty m.optional_string
      assert_empty m.optional_bytes
      assert_nil m.optional_msg
      assert_equal :Default, m.optional_enum
    end

    def test_setters
      m = TestMessage.new
      m.optional_int32 = -42
      assert_equal -42, m.optional_int32
      m.optional_int64 = -0x1_0000_0000
      assert_equal -0x1_0000_0000, m.optional_int64
      m.optional_uint32 = 0x9000_0000
      assert_equal 0x9000_0000, m.optional_uint32
      m.optional_uint64 = 0x9000_0000_0000_0000
      assert_equal 0x9000_0000_0000_0000, m.optional_uint64
      m.optional_bool = true
      assert m.optional_bool
      m.optional_float = 0.5
      assert_equal 0.5, m.optional_float
      m.optional_double = 0.5
      assert_equal 0.5, m.optional_double
      m.optional_string = "hello"
      assert_equal "hello", m.optional_string
      m.optional_bytes = "world".encode!('ASCII-8BIT')
      assert_equal "world", m.optional_bytes
      m.optional_msg = TestMessage2.new(:foo => 42)
      assert_equal m.optional_msg, TestMessage2.new(:foo => 42)
      m.optional_msg = nil
      assert_nil m.optional_msg
    end

    def test_ctor_args
      m = TestMessage.new(:optional_int32 => -42,
                          :optional_msg => TestMessage2.new,
                          :optional_enum => :C,
                          :repeated_string => ["hello", "there", "world"])
      assert_equal -42, m.optional_int32
      assert_instance_of TestMessage2, m.optional_msg
      assert_equal 3, m.repeated_string.length
      assert_equal :C, m.optional_enum
      assert_equal "hello", m.repeated_string[0]
      assert_equal "there", m.repeated_string[1]
      assert_equal "world", m.repeated_string[2]
    end

    def test_inspect
      m = TestMessage.new(:optional_int32 => -42,
                          :optional_enum => :A,
                          :optional_msg => TestMessage2.new,
                          :repeated_string => ["hello", "there", "world"])
      expected = '<BasicTest::TestMessage: optional_int32: -42, optional_int64: 0, optional_uint32: 0, optional_uint64: 0, optional_bool: false, optional_float: 0.0, optional_double: 0.0, optional_string: "", optional_bytes: "", optional_msg: <BasicTest::TestMessage2: foo: 0>, optional_enum: :A, repeated_int32: [], repeated_int64: [], repeated_uint32: [], repeated_uint64: [], repeated_bool: [], repeated_float: [], repeated_double: [], repeated_string: ["hello", "there", "world"], repeated_bytes: [], repeated_msg: [], repeated_enum: []>'
      assert_equal expected, m.inspect
    end

    def test_hash
      m1 = TestMessage.new(:optional_int32 => 42)
      m2 = TestMessage.new(:optional_int32 => 102)
      refute_equal 0, m1.hash
      refute_equal 0, m2.hash
      # relying on the randomness here -- if hash function changes and we are
      # unlucky enough to get a collision, then change the values above.
      refute_equal m1.hash, m2.hash
    end

    def test_unknown_field_errors
      e = assert_raises NoMethodError do
        TestMessage.new.hello
      end
      assert_match(/hello/, e.message)

      e = assert_raises NoMethodError do
        TestMessage.new.hello = "world"
      end
      assert_match(/hello/, e.message)
    end

    def test_initialization_map_errors
      e = assert_raises ArgumentError do
        TestMessage.new(:hello => "world")
      end
      assert_match(/hello/, e.message)

      e = assert_raises ArgumentError do
        MapMessage.new(:map_string_int32 => "hello")
      end
      assert_equal "Expected Hash object as initializer value for map field 'map_string_int32' (given String).", e.message
      e = assert_raises ArgumentError do
        TestMessage.new(:repeated_uint32 => "hello")
      end
      assert_equal "Expected array as initializer value for repeated field 'repeated_uint32' (given String).", e.message
    end

    def test_type_errors
      m = TestMessage.new

      assert_raises Google::Protobuf::TypeError do
        m.optional_int32 = "hello"
      end

      assert_raises Google::Protobuf::TypeError do
        m.optional_string = nil
      end

      assert_raises Google::Protobuf::TypeError do
        m.optional_bool = 42
      end

      assert_raises Google::Protobuf::TypeError do
        m.optional_msg = TestMessage.new  # expects TestMessage2
      end

      assert_raises Google::Protobuf::TypeError do
        m.repeated_int32 = []  # needs RepeatedField
      end

      assert_raises Google::Protobuf::TypeError do
        m.repeated_msg.push TestMessage.new
      end
    end

    def test_string_encoding
      m = TestMessage.new

      # Assigning a normal (ASCII or UTF8) string to a bytes field, or
      # ASCII-8BIT to a string field will convert to the proper encoding.
      m.optional_bytes = "Test string ASCII".encode!('ASCII')
      assert m.optional_bytes.frozen?
      assert_equal Encoding::ASCII_8BIT, m.optional_bytes.encoding
      assert_equal "Test string ASCII", m.optional_bytes

      assert_raises Encoding::UndefinedConversionError do
        m.optional_bytes = "Test string UTF-8 \u0100".encode!('UTF-8')
      end

      assert_raises Encoding::UndefinedConversionError do
        m.optional_string = ["FFFF"].pack('H*')
      end

      # "Ordinary" use case.
      m.optional_bytes = ["FFFF"].pack('H*')
      m.optional_string = "\u0100"

      # strings are immutable so we can't do this, but serialize should catch it.
      m.optional_string = "asdf".encode!('UTF-8')
      assert_raises do
        m.optional_string.encode!('ASCII-8BIT')
      end
    end

    def test_rptfield_int32
      l = Google::Protobuf::RepeatedField.new(:int32)
      assert_equal 0, l.count
      l = Google::Protobuf::RepeatedField.new(:int32, [1, 2, 3])
      assert_equal 3, l.count
      assert_equal [1, 2, 3], l
      assert_equal [1, 2, 3], l
      l.push 4
      assert_equal [1, 2, 3, 4], l
      dst_list = []
      l.each { |val| dst_list.push val }
      assert_equal [1, 2, 3, 4], dst_list
      assert_equal [1, 2, 3, 4], l.to_a
      assert_equal 1, l[0]
      assert_equal 4, l[3]
      l[0] = 5
      assert_equal [5, 2, 3, 4], l
      l2 = l.dup
      assert_equal l, l2
      refute_same l, l2
      l2.push 6
      assert_equal 4, l.count
      assert_equal 5, l2.count
      assert_equal '[5, 2, 3, 4]', l.inspect
      l.concat([7, 8, 9])
      assert_equal [5, 2, 3, 4, 7, 8, 9], l
      assert_equal 9, l.pop
      assert_equal [5, 2, 3, 4, 7, 8], l
      m = TestMessage.new
      assert_raises Google::Protobuf::TypeError do
        l.push m
      end

      m.repeated_int32 = l
      assert_equal [5, 2, 3, 4, 7, 8], m.repeated_int32
      assert_same m.repeated_int32, l
      l.push 42
      assert_equal 42, m.repeated_int32.pop
      l3 = l + l.dup
      assert_equal l.count * 2, l3.count
      l.count.times do |i|
        assert_equal l[i], l3[i]
        assert_equal l[i], l3[l.count + i]
      end

      l.clear
      assert_equal 0, l.count
      l += [1, 2, 3, 4]
      l.replace([5, 6, 7, 8])
      assert_equal [5, 6, 7, 8], l
      l4 = Google::Protobuf::RepeatedField.new(:int32)
      l4[5] = 42
      assert_equal [0, 0, 0, 0, 0, 42], l4
      l4 << 100
      assert_equal [0, 0, 0, 0, 0, 42, 100], l4
      l4 << 101 << 102
      assert_equal [0, 0, 0, 0, 0, 42, 100, 101, 102], l4
    end

    def test_parent_rptfield
      #make sure we set the RepeatedField and can add to it
      m = TestMessage.new
      assert_empty m.repeated_string
      m.repeated_string << 'ok'
      m.repeated_string.push('ok2')
      assert_equal ['ok', 'ok2'], m.repeated_string
      m.repeated_string += ['ok3']
      assert_equal ['ok', 'ok2', 'ok3'], m.repeated_string
    end

    def test_rptfield_msg
      l = Google::Protobuf::RepeatedField.new(:message, TestMessage)
      l.push TestMessage.new
      assert_equal 1, l.count
      assert_raises Google::Protobuf::TypeError do
        l.push TestMessage2.new
      end
      assert_raises Google::Protobuf::TypeError do
        l.push 42
      end

      l2 = l.dup
      assert_equal l[0], l2[0]
      assert_same l2[0], l[0]
      l2 = Google::Protobuf.deep_copy(l)
      assert_equal l[0], l2[0]
      refute_same l2[0], l[0]
      l3 = l + l2
      assert_equal 2, l3.count
      assert_equal l[0], l3[0]
      assert_equal l2[0], l3[1]
      l3[0].optional_int32 = 1000
      assert_equal 1000, l[0].optional_int32
      new_msg = TestMessage.new(:optional_int32 => 200)
      l4 = l + [new_msg]
      assert_equal 2, l4.count
      new_msg.optional_int32 = 1000
      assert_equal 1000, l4[1].optional_int32
    end

    def test_rptfield_enum
      l = Google::Protobuf::RepeatedField.new(:enum, TestEnum)
      l.push :A
      l.push :B
      l.push :C
      assert_equal 3, l.count
      assert_raises RangeError do
        l.push :D
      end
      assert_equal :A, l[0]
      l.push 4
      assert_equal 4, l[3]
    end

    def test_rptfield_initialize
      assert_raises ArgumentError do
        l = Google::Protobuf::RepeatedField.new
      end
      assert_raises ArgumentError do
        l = Google::Protobuf::RepeatedField.new(:message)
      end
      assert_raises ArgumentError do
        l = Google::Protobuf::RepeatedField.new([1, 2, 3])
      end
      assert_raises ArgumentError do
        l = Google::Protobuf::RepeatedField.new(:message, [TestMessage2.new])
      end
    end

    def test_rptfield_array_ducktyping
      l = Google::Protobuf::RepeatedField.new(:int32)
      length_methods = %w(count length size)
      length_methods.each do |lm|
        assert_equal 0, l.send(lm)
      end
      # out of bounds returns a nil
      assert_nil l[0]
      assert_nil l[1]
      assert_nil l[-1]
      l.push 4
      length_methods.each do |lm|
        assert_equal 1, l.send(lm)
      end
      assert_equal 4, l[0]
      assert_nil l[1]
      assert_equal 4, l[-1]
      assert_nil l[-2]
      l.push 2
      length_methods.each do |lm|
        assert_equal 2, l.send(lm)
      end
      assert_equal 4, l[0]
      assert_equal 2, l[1]
      assert_nil l[2]
      assert_equal 2, l[-1]
      assert_equal 4, l[-2]
      assert_nil l[-3]
      #adding out of scope will backfill with empty objects
    end

    def test_map_basic
      # allowed key types:
      # :int32, :int64, :uint32, :uint64, :bool, :string, :bytes.

      m = Google::Protobuf::Map.new(:string, :int32)
      m["asdf"] = 1
      assert_equal 1, m["asdf"]
      m["jkl;"] = 42
      assert_equal({ "jkl;" => 42, "asdf" => 1 }, m.to_h)
      assert_includes m.to_h, "asdf"
      refute_includes m, "qwerty"
      assert_equal 2, m.length
      m2 = m.dup
      assert_equal m, m2
      refute_equal 0, m.hash
      assert_equal m.hash, m2.hash
      collected = {}
      m.each { |k,v| collected[v] = k }
      assert_equal({ 42 => "jkl;", 1 => "asdf" }, collected)
      assert_equal 1, m.delete("asdf")
      refute_includes m, "asdf"
      assert_nil m["asdf"]
      refute_includes m, "asdf"

      # We only assert on inspect value when there is one map entry because the
      # order in which elements appear is unspecified (depends on the internal
      # hash function). We don't want a brittle test.
      assert_equal "{\"jkl;\"=>42}", m.inspect
      assert_equal ["jkl;"], m.keys
      assert_equal [42], m.values
      m.clear
      assert_equal 0, m.length
      assert_empty m.to_h
      assert_raises Google::Protobuf::TypeError do
        m[1] = 1
      end

      assert_raises RangeError do
        m["asdf"] = 0x1_0000_0000
      end
    end

    def test_map_ctor
      m = Google::Protobuf::Map.new(:string, :int32,
                                    {"a" => 1, "b" => 2, "c" => 3})
      assert_equal({"a" => 1, "c" => 3, "b" => 2}, m.to_h)
    end

    def test_map_keytypes
      m = Google::Protobuf::Map.new(:int32, :int32)
      m[1] = 42
      m[-1] = 42
      assert_raises RangeError do
        m[0x8000_0000] = 1
      end

      assert_raises Google::Protobuf::TypeError do
        m["asdf"] = 1
      end

      m = Google::Protobuf::Map.new(:int64, :int32)
      m[0x1000_0000_0000_0000] = 1
      assert_raises RangeError do
        m[0x1_0000_0000_0000_0000] = 1
      end

      assert_raises Google::Protobuf::TypeError do
        m["asdf"] = 1
      end

      m = Google::Protobuf::Map.new(:uint32, :int32)
      m[0x8000_0000] = 1
      assert_raises RangeError do
        m[0x1_0000_0000] = 1
      end
      assert_raises RangeError do
        m[-1] = 1
      end

      m = Google::Protobuf::Map.new(:uint64, :int32)
      m[0x8000_0000_0000_0000] = 1
      assert_raises RangeError do
        m[0x1_0000_0000_0000_0000] = 1
      end
      assert_raises RangeError do
        m[-1] = 1
      end

      m = Google::Protobuf::Map.new(:bool, :int32)
      m[true] = 1
      m[false] = 2

      assert_raises Google::Protobuf::TypeError do
        m[1] = 1
      end

      assert_raises Google::Protobuf::TypeError do
        m["asdf"] = 1
      end

      m = Google::Protobuf::Map.new(:string, :int32)
      m["asdf"] = 1
      assert_raises Google::Protobuf::TypeError do
        m[1] = 1
      end
      bytestring = ["FFFF"].pack("H*")
      assert_raises Encoding::UndefinedConversionError do
        m[bytestring] = 1
      end

      m = Google::Protobuf::Map.new(:bytes, :int32)
      bytestring = ["FFFF"].pack("H*")
      m[bytestring] = 1
      # Allowed -- we will automatically convert to ASCII-8BIT.
      m["asdf"] = 1
      assert_raises Google::Protobuf::TypeError do
        m[1] = 1
      end
    end

    def test_map_msg_enum_valuetypes
      m = Google::Protobuf::Map.new(:string, :message, TestMessage)
      m["asdf"] = TestMessage.new
      assert_raises Google::Protobuf::TypeError do
        m["jkl;"] = TestMessage2.new
      end

      m = Google::Protobuf::Map.new(
        :string, :message, TestMessage,
        { "a" => TestMessage.new(:optional_int32 => 42),
          "b" => TestMessage.new(:optional_int32 => 84) })
      assert_equal 2, m.length
      assert_equal [42, 84], m.values.map{|msg| msg.optional_int32}.sort
      m = Google::Protobuf::Map.new(:string, :enum, TestEnum,
                                    { "x" => :A, "y" => :B, "z" => :C })
      assert_equal 3, m.length
      assert_equal :C, m["z"]
      m["z"] = 2
      assert_equal :B, m["z"]
      m["z"] = 5
      assert_equal 5, m["z"]
      assert_raises RangeError do
        m["z"] = :Z
      end
      assert_raises RangeError do
        m["z"] = "z"
      end
    end

    def test_map_dup_deep_copy
      m = Google::Protobuf::Map.new(
        :string, :message, TestMessage,
        { "a" => TestMessage.new(:optional_int32 => 42),
          "b" => TestMessage.new(:optional_int32 => 84) })

      m2 = m.dup
      assert_equal m, m2
      refute_same m, m2
      assert_same m["a"], m2["a"]
      assert_same m["b"], m2["b"]
      m2 = Google::Protobuf.deep_copy(m)
      assert_equal m, m2
      refute_same m, m2
      refute_same m["a"], m2["a"]
      refute_same m["b"], m2["b"]
    end

    def test_map_field
      m = MapMessage.new
      assert_empty m.map_string_int32.to_h
      assert_empty m.map_string_msg.to_h
      m = MapMessage.new(
        :map_string_int32 => {"a" => 1, "b" => 2},
        :map_string_msg => {"a" => TestMessage2.new(:foo => 1),
                            "b" => TestMessage2.new(:foo => 2)})
      assert_equal ["a", "b"], m.map_string_int32.keys.sort
      assert_equal 1, m.map_string_int32["a"]
      assert_equal 2, m.map_string_msg["b"].foo
      m.map_string_int32["c"] = 3
      assert_equal 3, m.map_string_int32["c"]
      m.map_string_msg["c"] = TestMessage2.new(:foo => 3)
      assert_equal TestMessage2.new(:foo => 3), m.map_string_msg["c"]
      m.map_string_msg.delete("b")
      m.map_string_msg.delete("c")
      assert_equal({ "a" => TestMessage2.new(:foo => 1).to_h }, m.map_string_msg.to_h)
      assert_raises Google::Protobuf::TypeError do
        m.map_string_msg["e"] = TestMessage.new # wrong value type
      end
      # ensure nothing was added by the above
      assert_equal({ "a" => TestMessage2.new(:foo => 1).to_h }, m.map_string_msg.to_h)
      m.map_string_int32 = Google::Protobuf::Map.new(:string, :int32)
      assert_raises Google::Protobuf::TypeError do
        m.map_string_int32 = Google::Protobuf::Map.new(:string, :int64)
      end
      assert_raises Google::Protobuf::TypeError do
        m.map_string_int32 = {}
      end
      assert_raises Google::Protobuf::TypeError do
        m = MapMessage.new(:map_string_int32 => { 1 => "I am not a number" })
      end
    end

    def test_map_encode_decode
      m = MapMessage.new(
        :map_string_int32 => {"a" => 1, "b" => 2},
        :map_string_msg => {"a" => TestMessage2.new(:foo => 1),
                            "b" => TestMessage2.new(:foo => 2)})
      m2 = MapMessage.decode(MapMessage.encode(m))
      assert_equal m, m2
      m3 = MapMessageWireEquiv.decode(MapMessage.encode(m))
      assert_equal 2, m3.map_string_int32.length
      kv = {}
      m3.map_string_int32.map { |msg| kv[msg.key] = msg.value }
      assert_equal({"a" => 1, "b" => 2}, kv)
      kv = {}
      m3.map_string_msg.map { |msg| kv[msg.key] = msg.value }
      assert_equal({"a" => TestMessage2.new(:foo => 1),
                    "b" => TestMessage2.new(:foo => 2)}, kv)
    end

    def test_oneof_descriptors
      d = OneofMessage.descriptor
      o = d.lookup_oneof("my_oneof")
      refute_nil o
      assert_instance_of Google::Protobuf::OneofDescriptor, o
      assert_equal "my_oneof", o.name
      oneof_count = 0
      d.each_oneof{ |oneof|
        oneof_count += 1
        assert_equal o, oneof
      }
      assert_equal 1, oneof_count
      assert_equal 4, o.count
      field_names = o.map{|f| f.name}.sort
      assert_equal ["a", "b", "c", "d"], field_names
    end

    def test_oneof
      d = OneofMessage.new
      assert_empty d.a
      assert_equal 0, d.b
      assert_nil d.c
      assert_equal :Default, d.d
      assert_nil d.my_oneof
      d.a = "hi"
      assert_equal "hi", d.a
      assert_equal 0, d.b
      assert_nil d.c
      assert_equal :Default, d.d
      assert_equal :a, d.my_oneof
      d.b = 42
      assert_empty d.a
      assert_equal 42, d.b
      assert_nil d.c
      assert_equal :Default, d.d
      assert_equal :b, d.my_oneof
      d.c = TestMessage2.new(:foo => 100)
      assert_empty d.a
      assert_equal 0, d.b
      assert_equal 100, d.c.foo
      assert_equal :Default, d.d
      assert_equal :c, d.my_oneof
      d.d = :C
      assert_empty d.a
      assert_equal 0, d.b
      assert_nil d.c
      assert_equal :C, d.d
      assert_equal :d, d.my_oneof
      d2 = OneofMessage.decode(OneofMessage.encode(d))
      assert_equal d2, d
      encoded_field_a = OneofMessage.encode(OneofMessage.new(:a => "string"))
      encoded_field_b = OneofMessage.encode(OneofMessage.new(:b => 1000))
      encoded_field_c = OneofMessage.encode(
        OneofMessage.new(:c => TestMessage2.new(:foo => 1)))
      encoded_field_d = OneofMessage.encode(OneofMessage.new(:d => :B))

      d3 = OneofMessage.decode(
        encoded_field_c + encoded_field_a + encoded_field_d)
      assert_empty d3.a
      assert_equal 0, d3.b
      assert_nil d3.c
      assert_equal :B, d3.d
      d4 = OneofMessage.decode(
        encoded_field_c + encoded_field_a + encoded_field_d +
        encoded_field_c)
      assert_empty d4.a
      assert_equal 0, d4.b
      assert_equal 1, d4.c.foo
      assert_equal :Default, d4.d
      d5 = OneofMessage.new(:a => "hello")
      assert_equal "hello", d5.a
      d5.a = nil
      assert_empty d5.a
      assert_empty OneofMessage.encode(d5)
      assert_nil d5.my_oneof
    end

    def test_enum_field
      m = TestMessage.new
      assert_equal :Default, m.optional_enum
      m.optional_enum = :A
      assert_equal :A, m.optional_enum
      assert_raises RangeError do
        m.optional_enum = :ASDF
      end
      m.optional_enum = 1
      assert_equal :A, m.optional_enum
      m.optional_enum = 100
      assert_equal 100, m.optional_enum
    end

    def test_dup
      m = TestMessage.new
      m.optional_string = "hello"
      m.optional_int32 = 42
      tm1 = TestMessage2.new(:foo => 100)
      tm2 = TestMessage2.new(:foo => 200)
      m.repeated_msg.push tm1
      assert_equal m.repeated_msg[-1], tm1
      m.repeated_msg.push tm2
      assert_equal m.repeated_msg[-1], tm2
      m2 = m.dup
      assert_equal m, m2
      m.optional_int32 += 1
      refute_equal m2, m
      assert_equal m.repeated_msg[0], m2.repeated_msg[0]
      assert_same m.repeated_msg[0], m2.repeated_msg[0]
    end

    def test_deep_copy
      m = TestMessage.new(:optional_int32 => 42,
                          :repeated_msg => [TestMessage2.new(:foo => 100)])
      m2 = Google::Protobuf.deep_copy(m)
      assert_equal m, m2
      assert_equal m.repeated_msg, m2.repeated_msg
      refute_same m.repeated_msg, m2.repeated_msg
      refute_same m.repeated_msg[0], m2.repeated_msg[0]
    end

    def test_eq
      m = TestMessage.new(:optional_int32 => 42,
                          :repeated_int32 => [1, 2, 3])
      m2 = TestMessage.new(:optional_int32 => 43,
                           :repeated_int32 => [1, 2, 3])
      refute_equal m2, m
    end

    def test_enum_lookup
      assert_equal 1, TestEnum::A
      assert_equal 2, TestEnum::B
      assert_equal 3, TestEnum::C
      assert_equal :A, TestEnum::lookup(1)
      assert_equal :B, TestEnum::lookup(2)
      assert_equal :C, TestEnum::lookup(3)
      assert_equal 1, TestEnum::resolve(:A)
      assert_equal 2, TestEnum::resolve(:B)
      assert_equal 3, TestEnum::resolve(:C)
    end

    def test_parse_serialize
      m = TestMessage.new(:optional_int32 => 42,
                          :optional_string => "hello world",
                          :optional_enum => :B,
                          :repeated_string => ["a", "b", "c"],
                          :repeated_int32 => [42, 43, 44],
                          :repeated_enum => [:A, :B, :C, 100],
                          :repeated_msg => [TestMessage2.new(:foo => 1),
                                            TestMessage2.new(:foo => 2)])
      data = TestMessage.encode m
      m2 = TestMessage.decode data
      assert_equal m, m2
      data = Google::Protobuf.encode m
      m2 = Google::Protobuf.decode(TestMessage, data)
      assert_equal m, m2
    end

    def test_encode_decode_helpers
      m = TestMessage.new(:optional_string => 'foo', :repeated_string => ['bar1', 'bar2'])
      assert_equal 'foo', m.optional_string
      assert_equal ['bar1', 'bar2'], m.repeated_string

      json = m.to_json
      m2 = TestMessage.decode_json(json)
      assert_equal 'foo', m2.optional_string
      assert_equal ['bar1', 'bar2'], m2.repeated_string
      if RUBY_PLATFORM != "java"
        assert m2.optional_string.frozen?
        assert m2.repeated_string[0].frozen?
      end

      proto = m.to_proto
      m2 = TestMessage.decode(proto)
      assert_equal 'foo', m2.optional_string
      assert_equal ['bar1', 'bar2'], m2.repeated_string
    end

    def test_protobuf_encode_decode_helpers
      m = TestMessage.new(:optional_string => 'foo', :repeated_string => ['bar1', 'bar2'])
      encoded_msg = Google::Protobuf.encode(m)
      assert_equal m.to_proto, encoded_msg

      decoded_msg = Google::Protobuf.decode(TestMessage, encoded_msg)
      assert_equal TestMessage.decode(m.to_proto), decoded_msg
    end

    def test_protobuf_encode_decode_json_helpers
      m = TestMessage.new(:optional_string => 'foo', :repeated_string => ['bar1', 'bar2'])
      encoded_msg = Google::Protobuf.encode_json(m)
      assert_equal m.to_json, encoded_msg

      decoded_msg = Google::Protobuf.decode_json(TestMessage, encoded_msg)
      assert_equal TestMessage.decode_json(m.to_json), decoded_msg
    end

    def test_to_h
      m = TestMessage.new(:optional_bool => true, :optional_double => -10.100001, :optional_string => 'foo', :repeated_string => ['bar1', 'bar2'])
      expected_result = {
        :optional_bool=>true,
        :optional_bytes=>"",
        :optional_double=>-10.100001,
        :optional_enum=>:Default,
        :optional_float=>0.0,
        :optional_int32=>0,
        :optional_int64=>0,
        :optional_msg=>nil,
        :optional_string=>"foo",
        :optional_uint32=>0,
        :optional_uint64=>0,
        :repeated_bool=>[],
        :repeated_bytes=>[],
        :repeated_double=>[],
        :repeated_enum=>[],
        :repeated_float=>[],
        :repeated_int32=>[],
        :repeated_int64=>[],
        :repeated_msg=>[],
        :repeated_string=>["bar1", "bar2"],
        :repeated_uint32=>[],
        :repeated_uint64=>[]
      }
      assert_equal expected_result, m.to_h
    end


    def test_def_errors
      s = Google::Protobuf::DescriptorPool.new
      assert_raises Google::Protobuf::TypeError do
        s.build do
          # enum with no default (integer value 0)
          add_enum "MyEnum" do
            value :A, 1
          end
        end
      end
      assert_raises Google::Protobuf::TypeError do
        s.build do
          # message with required field (unsupported in proto3)
          add_message "MyMessage" do
            required :foo, :int32, 1
          end
        end
      end
    end

    def test_corecursive
      # just be sure that we can instantiate types with corecursive field-type
      # references.
      m = Recursive1.new(:foo => Recursive2.new(:foo => Recursive1.new))
      assert_equal Recursive2.descriptor, Recursive1.descriptor.lookup("foo").subtype
      assert_equal Recursive1.descriptor, Recursive2.descriptor.lookup("foo").subtype
      serialized = Recursive1.encode(m)
      m2 = Recursive1.decode(serialized)
      assert_equal m, m2
    end

    def test_serialize_cycle
      m = Recursive1.new(:foo => Recursive2.new)
      m.foo.foo = m
      assert_raises RuntimeError do
        Recursive1.encode(m)
      end
    end

    def test_bad_field_names
      m = BadFieldNames.new(:dup => 1, :class => 2)
      m2 = m.dup
      assert_equal m, m2
      assert_equal 1, m['dup']
      assert_equal 2, m['class']
      m['dup'] = 3
      assert_equal 3, m['dup']
    end

    def test_int_ranges
      m = TestMessage.new

      m.optional_int32 = 0
      m.optional_int32 = -0x8000_0000
      m.optional_int32 = +0x7fff_ffff
      m.optional_int32 = 1.0
      m.optional_int32 = -1.0
      m.optional_int32 = 2e9
      assert_raises RangeError do
        m.optional_int32 = -0x8000_0001
      end
      assert_raises RangeError do
        m.optional_int32 = +0x8000_0000
      end
      assert_raises RangeError do
        m.optional_int32 = +0x1000_0000_0000_0000_0000_0000 # force Bignum
      end
      assert_raises RangeError do
        m.optional_int32 = 1e12
      end
      assert_raises RangeError do
        m.optional_int32 = 1.5
      end

      m.optional_uint32 = 0
      m.optional_uint32 = +0xffff_ffff
      m.optional_uint32 = 1.0
      m.optional_uint32 = 4e9
      assert_raises RangeError do
        m.optional_uint32 = -1
      end
      assert_raises RangeError do
        m.optional_uint32 = -1.5
      end
      assert_raises RangeError do
        m.optional_uint32 = -1.5e12
      end
      assert_raises RangeError do
        m.optional_uint32 = -0x1000_0000_0000_0000
      end
      assert_raises RangeError do
        m.optional_uint32 = +0x1_0000_0000
      end
      assert_raises RangeError do
        m.optional_uint32 = +0x1000_0000_0000_0000_0000_0000 # force Bignum
      end
      assert_raises RangeError do
        m.optional_uint32 = 1e12
      end
      assert_raises RangeError do
        m.optional_uint32 = 1.5
      end

      m.optional_int64 = 0
      m.optional_int64 = -0x8000_0000_0000_0000
      m.optional_int64 = +0x7fff_ffff_ffff_ffff
      m.optional_int64 = 1.0
      m.optional_int64 = -1.0
      m.optional_int64 = 8e18
      m.optional_int64 = -8e18
      assert_raises RangeError do
        m.optional_int64 = -0x8000_0000_0000_0001
      end
      assert_raises RangeError do
        m.optional_int64 = +0x8000_0000_0000_0000
      end
      assert_raises RangeError do
        m.optional_int64 = +0x1000_0000_0000_0000_0000_0000 # force Bignum
      end
      assert_raises RangeError do
        m.optional_int64 = 1e50
      end
      assert_raises RangeError do
        m.optional_int64 = 1.5
      end

      m.optional_uint64 = 0
      m.optional_uint64 = +0xffff_ffff_ffff_ffff
      m.optional_uint64 = 1.0
      m.optional_uint64 = 16e18
      assert_raises RangeError do
        m.optional_uint64 = -1
      end
      assert_raises RangeError do
        m.optional_uint64 = -1.5
      end
      assert_raises RangeError do
        m.optional_uint64 = -1.5e12
      end
      assert_raises RangeError do
        m.optional_uint64 = -0x1_0000_0000_0000_0000
      end
      assert_raises RangeError do
        m.optional_uint64 = +0x1_0000_0000_0000_0000
      end
      assert_raises RangeError do
        m.optional_uint64 = +0x1000_0000_0000_0000_0000_0000 # force Bignum
      end
      assert_raises RangeError do
        m.optional_uint64 = 1e50
      end
      assert_raises RangeError do
        m.optional_uint64 = 1.5
      end
    end

    def test_stress_test
      m = TestMessage.new
      m.optional_int32 = 42
      m.optional_int64 = 0x100000000
      m.optional_string = "hello world"
      10.times do m.repeated_msg.push TestMessage2.new(:foo => 42) end
      10.times do m.repeated_string.push "hello world" end

      data = TestMessage.encode(m)

      l = 0
      10_000.times do
        m = TestMessage.decode(data)
        data_new = TestMessage.encode(m)
        assert_equal data, data_new
        data = data_new
      end
    end

    def test_reflection
      m = TestMessage.new(:optional_int32 => 1234)
      msgdef = m.class.descriptor
      assert_instance_of Google::Protobuf::Descriptor, msgdef
      assert msgdef.any? {|field| field.name == "optional_int32"}
      optional_int32 = msgdef.lookup "optional_int32"
      assert_instance_of Google::Protobuf::FieldDescriptor, optional_int32
      refute_nil optional_int32
      assert_equal "optional_int32", optional_int32.name
      assert_equal :int32, optional_int32.type
      optional_int32.set(m, 5678)
      assert_equal 5678, m.optional_int32
      m.optional_int32 = 1000
      assert_equal 1000, optional_int32.get(m)
      optional_msg = msgdef.lookup "optional_msg"
      assert_equal TestMessage2.descriptor, optional_msg.subtype
      optional_msg.set(m, optional_msg.subtype.msgclass.new)

      assert_equal TestMessage, msgdef.msgclass
      optional_enum = msgdef.lookup "optional_enum"
      assert_equal TestEnum.descriptor, optional_enum.subtype
      assert_instance_of Google::Protobuf::EnumDescriptor, optional_enum.subtype
      optional_enum.subtype.each do |k, v|
        # set with integer, check resolution to symbolic name
        optional_enum.set(m, v)
        assert_equal k, optional_enum.get(m)
      end
    end

    def test_json
      # TODO: Fix JSON in JRuby version.
      return if RUBY_PLATFORM == "java"
      m = TestMessage.new(:optional_int32 => 1234,
                          :optional_int64 => -0x1_0000_0000,
                          :optional_uint32 => 0x8000_0000,
                          :optional_uint64 => 0xffff_ffff_ffff_ffff,
                          :optional_bool => true,
                          :optional_float => 1.0,
                          :optional_double => -1e100,
                          :optional_string => "Test string",
                          :optional_bytes => ["FFFFFFFF"].pack('H*'),
                          :optional_msg => TestMessage2.new(:foo => 42),
                          :repeated_int32 => [1, 2, 3, 4],
                          :repeated_string => ["a", "b", "c"],
                          :repeated_bool => [true, false, true, false],
                          :repeated_msg => [TestMessage2.new(:foo => 1),
                                            TestMessage2.new(:foo => 2)])

      json_text = TestMessage.encode_json(m)
      m2 = TestMessage.decode_json(json_text)
      assert_equal m, m2
      # Crash case from GitHub issue 283.
      bar = Bar.new(msg: "bar")
      baz1 = Baz.new(msg: "baz")
      baz2 = Baz.new(msg: "quux")
      Foo.encode_json(Foo.new)
      Foo.encode_json(Foo.new(bar: bar))
      Foo.encode_json(Foo.new(bar: bar, baz: [baz1, baz2]))
    end

    def test_json_maps
      # TODO: Fix JSON in JRuby version.
      return if RUBY_PLATFORM == "java"
      m = MapMessage.new(:map_string_int32 => {"a" => 1})
      expected = '{"mapStringInt32":{"a":1},"mapStringMsg":{}}'
      expected_preserve = '{"map_string_int32":{"a":1},"map_string_msg":{}}'
      assert_equal expected, MapMessage.encode_json(m, :emit_defaults => true)

      json = MapMessage.encode_json(m, :preserve_proto_fieldnames => true, :emit_defaults => true)
      assert_equal expected_preserve, json

      m2 = MapMessage.decode_json(MapMessage.encode_json(m))
      assert_equal m, m2
    end
  end
end
