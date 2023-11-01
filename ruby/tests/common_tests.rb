require 'google/protobuf/wrappers_pb.rb'

# Defines tests which are common between proto2 and proto3 syntax.
#
# Requires that the proto messages are exactly the same in proto2 and proto3 syntax
# and that the including class should define a 'proto_module' method which returns
# the enclosing module of the proto message classes.

require 'bigdecimal'

module CommonTests
  def test_defaults
    m = proto_module::TestMessage.new
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
    m = proto_module::TestMessage.new
    m.optional_int32 = -42
    assert_equal -42, m.optional_int32
    m.optional_int64 = -0x1_0000_0000
    assert_equal -0x1_0000_0000, m.optional_int64
    m.optional_uint32 = 0x9000_0000
    assert_equal 0x9000_0000, m.optional_uint32
    m.optional_uint64 = 0x9000_0000_0000_0000
    assert_equal 0x9000_0000_0000_0000, m.optional_uint64
    m.optional_bool = true
    assert_equal m.optional_bool, true
    m.optional_float = 0.5
    assert_equal 0.5, m.optional_float
    m.optional_double = 0.5
    assert_equal 0.5, m.optional_double
    m.optional_string = "hello"
    assert_equal "hello", m.optional_string
    m.optional_string = :hello
    assert_equal "hello", m.optional_string
    m.optional_bytes = "world".encode!('ASCII-8BIT')
    assert_equal "world", m.optional_bytes
    m.optional_msg = proto_module::TestMessage2.new(:foo => 42)
    assert_equal proto_module::TestMessage2.new(:foo => 42), m.optional_msg
    m.optional_msg = nil
    assert_nil m.optional_msg
    m.optional_enum = :C
    assert_equal :C, m.optional_enum
    m.optional_enum = 'C'
    assert_equal :C, m.optional_enum
  end

  def test_ctor_args
    m = proto_module::TestMessage.new(:optional_int32 => -42,
                                      :optional_msg => proto_module::TestMessage2.new,
                                      :optional_enum => :C,
                                      :repeated_string => ["hello", "there", "world"])
    assert_equal -42, m.optional_int32
    assert_instance_of proto_module::TestMessage2, m.optional_msg
    assert_equal 3, m.repeated_string.length
    assert_equal :C, m.optional_enum
    assert_equal "hello", m.repeated_string[0]
    assert_equal "there", m.repeated_string[1]
    assert_equal "world", m.repeated_string[2]
  end

  def test_ctor_string_symbol_args
    m = proto_module::TestMessage.new(:optional_enum => 'C', :repeated_enum => ['A', 'B'])
    assert_equal :C, m.optional_enum
    assert_equal [:A, :B], m.repeated_enum
    m = proto_module::TestMessage.new(:optional_string => :foo, :repeated_string => [:foo, :bar])
    assert_equal 'foo', m.optional_string
    assert_equal ['foo', 'bar'], m.repeated_string
  end

  def test_ctor_nil_args
    m = proto_module::TestMessage.new(:optional_enum => nil, :optional_int32 => nil, :optional_string => nil, :optional_msg => nil)

    assert_equal :Default, m.optional_enum
    assert_equal 0, m.optional_int32
    assert_empty m.optional_string
    assert_nil m.optional_msg
  end

  def test_embeddedmsg_hash_init
    m = proto_module::TestEmbeddedMessageParent.new(
      :child_msg => {sub_child: {optional_int32: 1}},
      :number => 2,
      :repeated_msg => [{sub_child: {optional_int32: 3}}],
      :repeated_number => [10, 20, 30])

    assert_equal 2, m.number
    assert_equal [10, 20, 30], m.repeated_number

    refute_nil m.child_msg
    refute_nil m.child_msg.sub_child
    assert_equal 1, m.child_msg.sub_child.optional_int32
    refute_nil m.repeated_msg
    assert_equal 1, m.repeated_msg.length
    assert_equal 3, m.repeated_msg.first.sub_child.optional_int32
  end

  def test_inspect_eq_to_s
    m = proto_module::TestMessage.new(
      :optional_int32 => -42,
      :optional_enum => :A,
      :optional_msg => proto_module::TestMessage2.new(foo: 0),
      :repeated_string => ["hello", "there", "world"])
    expected = "<#{proto_module}::TestMessage: optional_int32: -42, optional_msg: <#{proto_module}::TestMessage2: foo: 0>, optional_enum: :A, repeated_int32: [], repeated_int64: [], repeated_uint32: [], repeated_uint64: [], repeated_bool: [], repeated_float: [], repeated_double: [], repeated_string: [\"hello\", \"there\", \"world\"], repeated_bytes: [], repeated_msg: [], repeated_enum: []>"
    assert_equal expected, m.inspect
    assert_equal expected, m.to_s

    m = proto_module::OneofMessage.new(:b => -42)
    expected = "<#{proto_module}::OneofMessage: b: -42>"
    assert_equal expected, m.inspect
    assert_equal expected, m.to_s
  end

  def test_hash
    m1 = proto_module::TestMessage.new(:optional_int32 => 42)
    m2 = proto_module::TestMessage.new(:optional_int32 => 102, repeated_string: ['please', 'work', 'ok?'])
    m3 = proto_module::TestMessage.new(:optional_int32 => 102, repeated_string: ['please', 'work', 'ok?'])
    refute_equal 0, m1.hash
    refute_equal 0, m2.hash
    refute_equal 0, m3.hash
    # relying on the randomness here -- if hash function changes and we are
    # unlucky enough to get a collision, then change the values above.
    refute_equal m1.hash, m2.hash
    assert_equal m2.hash, m3.hash
  end

  def test_unknown_field_errors
    e = assert_raises NoMethodError do
      proto_module::TestMessage.new.hello
    end
    assert_match(/hello/, e.message)

    e = assert_raises NoMethodError do
      proto_module::TestMessage.new.hello = "world"
    end
    assert_match(/hello/, e.message)
  end

  def test_type_errors
    m = proto_module::TestMessage.new
    e = assert_raises Google::Protobuf::TypeError do
      m.optional_int32 = "hello"
    end

    # Google::Protobuf::TypeError should inherit from TypeError for backwards compatibility
    # TODO: This can be removed when we can safely migrate to Google::Protobuf::TypeError
    assert e.is_a?(::TypeError)

    assert_raises Google::Protobuf::TypeError do
      m.optional_string = 42
    end
    assert_raises Google::Protobuf::TypeError do
      m.optional_string = nil
    end
    assert_raises Google::Protobuf::TypeError do
      m.optional_bool = 42
    end
    assert_raises Google::Protobuf::TypeError do
      m.optional_msg = proto_module::TestMessage.new  # expects TestMessage2
    end

    assert_raises Google::Protobuf::TypeError do
      m.repeated_int32 = []  # needs RepeatedField
    end

    assert_raises Google::Protobuf::TypeError do
      m.repeated_int32.push "hello"
    end

    assert_raises Google::Protobuf::TypeError do
      m.repeated_msg.push proto_module::TestMessage.new
    end
  end

  def test_string_encoding
    m = proto_module::TestMessage.new

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
    assert_raises(FrozenError) { m.optional_string.encode!('ASCII-8BIT') }
  end

  def test_rptfield_int32
    l = Google::Protobuf::RepeatedField.new(:int32)
    assert_equal 0, l.count
    l = Google::Protobuf::RepeatedField.new(:int32, [1, 2, 3])
    assert_equal 3, l.count
    assert_equal [1, 2, 3], l
    assert_equal l, [1, 2, 3]
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
    assert_raises Google::Protobuf::TypeError do
      m = proto_module::TestMessage.new
      l.push m
    end

    m = proto_module::TestMessage.new
    m.repeated_int32 = l
    assert_equal [5, 2, 3, 4, 7, 8], m.repeated_int32
    assert_equal m.repeated_int32.object_id, l.object_id
    l.push 42
    assert_equal 42, m.repeated_int32.pop
    l3 = l + l.dup
    assert_equal l3.count, l.count * 2
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
    m = proto_module::TestMessage.new
    assert_empty m.repeated_string
    m.repeated_string << 'ok'
    m.repeated_string.push('ok2')
    assert_equal ['ok', 'ok2'], m.repeated_string
    m.repeated_string += ['ok3']
    assert_equal ['ok', 'ok2', 'ok3'], m.repeated_string
  end

  def test_rptfield_msg
    l = Google::Protobuf::RepeatedField.new(:message, proto_module::TestMessage)
    l.push proto_module::TestMessage.new
    assert_equal 1, l.count
    assert_raises Google::Protobuf::TypeError do
      l.push proto_module::TestMessage2.new
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
    new_msg = proto_module::TestMessage.new(:optional_int32 => 200)
    l4 = l + [new_msg]
    assert_equal 2, l4.count
    new_msg.optional_int32 = 1000
    assert_equal 1000, l4[1].optional_int32
  end

  def test_rptfield_enum
    l = Google::Protobuf::RepeatedField.new(:enum, proto_module::TestEnum)
    l.push :A
    l.push :B
    l.push :C
    l.push :v0
    assert_equal 4, l.count
    assert_raises RangeError do
      l.push :D
    end
    assert_equal :A, l[0]
    assert_equal :v0, l[3]
    l.push 5
    assert_equal 5, l[4]
  end

  def test_rptfield_initialize
    assert_raises ArgumentError do
      Google::Protobuf::RepeatedField.new
    end
    assert_raises ArgumentError do
      Google::Protobuf::RepeatedField.new(:message)
    end
    assert_raises ArgumentError do
      Google::Protobuf::RepeatedField.new([1, 2, 3])
    end
    assert_raises ArgumentError do
      Google::Protobuf::RepeatedField.new(:message, [proto_module::TestMessage2.new])
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
    assert m.has_key?("asdf")
    refute m.has_key?("qwerty")
    assert_equal 2, m.length
    m2 = m.dup
    assert_equal m, m2
    refute_equal 0, m.hash
    assert_equal m.hash, m2.hash

    collected = {}
    m.each { |k,v| collected[v] = k }
    assert_equal({ 42 => "jkl;", 1 => "asdf" }, collected)
    assert_equal 1, m.delete("asdf")
    refute m.has_key?("asdf")
    assert_nil m["asdf"]
    refute m.has_key?("asdf")

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

  def test_b_8385
    m1 = Google::Protobuf::Map.new(:string, :string)
    m2 = Google::Protobuf::Map.new(:string, :string)

    assert_equal m1, m2

    m1["counter"] = "a"
    m2["counter"] = "aa"

    refute_equal m1, m2
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
    assert_raises Encoding::UndefinedConversionError do
      bytestring = ["FFFF"].pack("H*")
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
    m = Google::Protobuf::Map.new(:string, :message, proto_module::TestMessage)
    m["asdf"] = proto_module::TestMessage.new
    assert_raises Google::Protobuf::TypeError do
      m["jkl;"] = proto_module::TestMessage2.new
    end

    m = Google::Protobuf::Map.new(
      :string, :message, proto_module::TestMessage,
      { "a" => proto_module::TestMessage.new(:optional_int32 => 42),
        "b" => proto_module::TestMessage.new(:optional_int32 => 84) })
    assert_equal 2, m.length
    assert_equal [42, 84], m.values.map{|msg| msg.optional_int32}.sort
    m = Google::Protobuf::Map.new(:string, :enum, proto_module::TestEnum,
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
      :string, :message, proto_module::TestMessage,
      { "a" => proto_module::TestMessage.new(:optional_int32 => 42),
        "b" => proto_module::TestMessage.new(:optional_int32 => 84) })

    m2 = m.dup
    assert_equal m.to_h, m2.to_h
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

  def test_oneof_descriptors
    d = proto_module::OneofMessage.descriptor
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
    d = proto_module::OneofMessage.new
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
    d.c = proto_module::TestMessage2.new(:foo => 100)
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
    d2 = proto_module::OneofMessage.decode(proto_module::OneofMessage.encode(d))
    assert_equal d, d2
    encoded_field_a = proto_module::OneofMessage.encode(proto_module::OneofMessage.new(:a => "string"))
    encoded_field_b = proto_module::OneofMessage.encode(proto_module::OneofMessage.new(:b => 1000))
    encoded_field_c = proto_module::OneofMessage.encode(
      proto_module::OneofMessage.new(:c => proto_module::TestMessage2.new(:foo => 1)))
    encoded_field_d = proto_module::OneofMessage.encode(proto_module::OneofMessage.new(:d => :B))

    d3 = proto_module::OneofMessage.decode(
      encoded_field_c + encoded_field_a + encoded_field_b + encoded_field_d)
    assert_empty d3.a
    assert_equal 0, d3.b
    assert_nil d3.c
    assert_equal :B, d3.d
    d4 = proto_module::OneofMessage.decode(
      encoded_field_c + encoded_field_a + encoded_field_b + encoded_field_d +
      encoded_field_c)
    assert_empty d4.a
    assert_equal 0, d4.b
    assert_equal 1, d4.c.foo
    assert_equal :Default, d4.d
    d5 = proto_module::OneofMessage.new(:a => "hello")
    assert_equal "hello", d5.a
    d5.a = nil
    assert_empty d5.a
    assert_empty proto_module::OneofMessage.encode(d5)
    assert_nil d5.my_oneof
  end

  def test_enum_field
    m = proto_module::TestMessage.new
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
    m = proto_module::TestMessage.new
    m.optional_string = "hello"
    m.optional_int32 = 42
    tm1 = proto_module::TestMessage2.new(:foo => 100)
    tm2 = proto_module::TestMessage2.new(:foo => 200)
    m.repeated_msg.push tm1
    assert_equal m.repeated_msg[-1], tm1
    m.repeated_msg.push tm2
    assert_equal m.repeated_msg[-1], tm2
    m2 = m.dup
    assert_equal m, m2
    m.optional_int32 += 1
    refute_equal m, m2
    assert_equal m.repeated_msg[0], m2.repeated_msg[0]
    assert_equal m.repeated_msg[0].object_id, m2.repeated_msg[0].object_id
  end

  def test_deep_copy
    m = proto_module::TestMessage.new(:optional_int32 => 42,
                                      :repeated_msg => [proto_module::TestMessage2.new(:foo => 100)])
    m2 = Google::Protobuf.deep_copy(m)
    assert_equal m, m2
    assert_equal m.repeated_msg, m2.repeated_msg
    refute_same m.repeated_msg, m2.repeated_msg
    refute_same m.repeated_msg[0], m2.repeated_msg[0]
  end

  def test_sub_message_deep_copy # regression test for issue 12505
    m = proto_module::Foo.new(bar: proto_module::Bar.new(msg: "Hello World"))
    m2 = Google::Protobuf.deep_copy(m)
    assert_equal m, m2
    refute_same m, m2
    assert_equal m.bar, m2.bar
    refute_same m.bar, m2.bar
  end

  def test_message_eq
    m = proto_module::TestMessage.new(:optional_int32 => 42,
                                      :repeated_int32 => [1, 2, 3])
    m2 = proto_module::TestMessage.new(:optional_int32 => 43,
                                       :repeated_int32 => [1, 2, 3])
    refute_equal m, m2
    refute_equal proto_module::TestMessage.new, proto_module::TestMessage2.new
  end

  def test_enum_lookup
    assert_equal 1, proto_module::TestEnum::A
    assert_equal 2, proto_module::TestEnum::B
    assert_equal 3, proto_module::TestEnum::C
    assert_equal 4, proto_module::TestEnum::V0
    assert_equal :A, proto_module::TestEnum::lookup(1)
    assert_equal :B, proto_module::TestEnum::lookup(2)
    assert_equal :C, proto_module::TestEnum::lookup(3)
    assert_equal :v0, proto_module::TestEnum::lookup(4)
    assert_equal 1, proto_module::TestEnum::resolve(:A)
    assert_equal 2, proto_module::TestEnum::resolve(:B)
    assert_equal 3, proto_module::TestEnum::resolve(:C)
    assert_equal 4, proto_module::TestEnum::resolve(:v0)
  end

  def test_enum_const_get_helpers
    m = proto_module::TestMessage.new
    assert_equal proto_module::TestEnum::Default, m.optional_enum_const
    assert_equal proto_module::TestEnum.const_get(:Default), m.optional_enum_const

    m = proto_module::TestMessage.new({optional_enum: proto_module::TestEnum::A})
    assert_equal proto_module::TestEnum::A, m.optional_enum_const
    assert_equal proto_module::TestEnum.const_get(:A), m.optional_enum_const

    m = proto_module::TestMessage.new({optional_enum: proto_module::TestEnum::B})
    assert_equal proto_module::TestEnum::B, m.optional_enum_const
    assert_equal proto_module::TestEnum.const_get(:B), m.optional_enum_const

    m = proto_module::TestMessage.new({optional_enum: proto_module::TestEnum::C})
    assert_equal proto_module::TestEnum::C, m.optional_enum_const
    assert_equal proto_module::TestEnum.const_get(:C), m.optional_enum_const

    m = proto_module::TestMessage2.new({foo: 2})
    assert_equal 2, m.foo
    assert_raises(NoMethodError) { m.foo_ }
    assert_raises(NoMethodError) { m.foo_X }
    assert_raises(NoMethodError) { m.foo_XX }
    assert_raises(NoMethodError) { m.foo_XXX }
    assert_raises(NoMethodError) { m.foo_XXXX }
    assert_raises(NoMethodError) { m.foo_XXXXX }
    assert_raises(NoMethodError) { m.foo_XXXXXX }

    m = proto_module::Enumer.new({optional_enum: :B})
    assert_equal :B, m.optional_enum
    assert_raises(NoMethodError) { m.optional_enum_ }
    assert_raises(NoMethodError) { m.optional_enum_X }
    assert_raises(NoMethodError) { m.optional_enum_XX }
    assert_raises(NoMethodError) { m.optional_enum_XXX }
    assert_raises(NoMethodError) { m.optional_enum_XXXX }
    assert_raises(NoMethodError) { m.optional_enum_XXXXX }
    assert_raises(NoMethodError) { m.optional_enum_XXXXXX }
  end

  def test_enum_getter
    m = proto_module::Enumer.new(:optional_enum => :B, :repeated_enum => [:A, :C])

    assert_equal :B, m.optional_enum
    assert_equal 2, m.optional_enum_const
    assert_equal proto_module::TestEnum::B, m.optional_enum_const
    assert_equal [:A, :C], m.repeated_enum
    assert_equal [1, 3], m.repeated_enum_const
    assert_equal [proto_module::TestEnum::A, proto_module::TestEnum::C], m.repeated_enum_const
  end

  def test_enum_getter_oneof
    m = proto_module::Enumer.new(:const => :C)

    assert_equal :C, m.const
    assert_equal 3, m.const_const
    assert_equal proto_module::TestEnum::C, m.const_const
  end

  def test_enum_getter_only_enums
    m = proto_module::Enumer.new(:optional_enum => :B, :a_const => 'thing')

    assert_equal 'thing', m.a_const
    assert_equal :B, m.optional_enum

    assert_raises(NoMethodError) { m.a }
    assert_raises(NoMethodError) { m.a_const_const }
  end

  def test_repeated_push
    m = proto_module::TestMessage.new

    m.repeated_string += ['one']
    m.repeated_string += %w[two three]
    assert_equal %w[one two three], m.repeated_string

    m.repeated_string.push( *['four', 'five'] )
    assert_equal %w[one two three four five], m.repeated_string

    m.repeated_string.push 'six', 'seven'
    assert_equal %w[one two three four five six seven], m.repeated_string

    m = proto_module::TestMessage.new

    m.repeated_msg += [proto_module::TestMessage2.new(:foo => 1), proto_module::TestMessage2.new(:foo => 2)]
    m.repeated_msg += [proto_module::TestMessage2.new(:foo => 3)]
    m.repeated_msg.push proto_module::TestMessage2.new(:foo => 4), proto_module::TestMessage2.new(:foo => 5)
    assert_equal [1, 2, 3, 4, 5], m.repeated_msg.map {|x| x.foo}
  end

  def test_parse_serialize
    m = proto_module::TestMessage.new(:optional_int32 => 42,
                                      :optional_string => "hello world",
                                      :optional_enum => :B,
                                      :repeated_string => ["a", "b", "c"],
                                      :repeated_int32 => [42, 43, 44],
                                      :repeated_enum => [:A, :B, :C],
                                      :repeated_msg => [proto_module::TestMessage2.new(:foo => 1),
                                                        proto_module::TestMessage2.new(:foo => 2)])
    if proto_module.class.name == "BasicTest"
      # For proto3 we can add an unknown enum value safely.
      m.repeated_enum << 100
    end

    data = proto_module::TestMessage.encode m
    m2 = proto_module::TestMessage.decode data

    assert_equal m, m2

    data = Google::Protobuf.encode m
    m2 = Google::Protobuf.decode(proto_module::TestMessage, data)
    assert_equal m, m2
  end

  def test_encode_decode_helpers
    m = proto_module::TestMessage.new(:optional_string => 'foo', :repeated_string => ['bar1', 'bar2'])
    assert_equal 'foo', m.optional_string
    assert_equal ['bar1', 'bar2'], m.repeated_string

    json = m.to_json
    m2 = proto_module::TestMessage.decode_json(json)
    assert_equal 'foo', m2.optional_string
    assert_equal ['bar1', 'bar2'], m2.repeated_string
    if RUBY_PLATFORM != "java"
      assert m2.optional_string.frozen?
      assert m2.repeated_string[0].frozen?
    end

    proto = m.to_proto
    m2 = proto_module::TestMessage.decode(proto)
    assert_equal 'foo', m2.optional_string
    assert_equal ['bar1', 'bar2'], m2.repeated_string
  end

  def test_protobuf_encode_decode_helpers
    m = proto_module::TestMessage.new(:optional_string => 'foo', :repeated_string => ['bar1', 'bar2'])
    encoded_msg = Google::Protobuf.encode(m)
    assert_equal m.to_proto, encoded_msg

    decoded_msg = Google::Protobuf.decode(proto_module::TestMessage, encoded_msg)
    assert_equal proto_module::TestMessage.decode(m.to_proto), decoded_msg
  end

  def test_protobuf_encode_decode_json_helpers
    m = proto_module::TestMessage.new(:optional_string => 'foo', :repeated_string => ['bar1', 'bar2'])
    encoded_msg = Google::Protobuf.encode_json(m)
    assert_equal m.to_json, encoded_msg

    decoded_msg = Google::Protobuf.decode_json(proto_module::TestMessage, encoded_msg)
    assert_equal proto_module::TestMessage.decode_json(m.to_json), decoded_msg

    assert_equal [m].to_json, Google::Protobuf.encode_json([m])
    assert_equal proto_module::TestMessage.decode_json([m.to_json].first), decoded_msg
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
    m = proto_module::Recursive1.new(:foo => proto_module::Recursive2.new(:foo => proto_module::Recursive1.new))
    assert_equal proto_module::Recursive2.descriptor, proto_module::Recursive1.descriptor.lookup("foo").subtype
    assert_equal proto_module::Recursive1.descriptor, proto_module::Recursive2.descriptor.lookup("foo").subtype
    serialized = proto_module::Recursive1.encode(m)
    m2 = proto_module::Recursive1.decode(serialized)
    assert_equal m, m2
  end

  def test_serialize_cycle
    m = proto_module::Recursive1.new(:foo => proto_module::Recursive2.new)
    m.foo.foo = m
    assert_raises RuntimeError do
      proto_module::Recursive1.encode(m)
    end
  end

  def test_bad_field_names
    m = proto_module::BadFieldNames.new(:dup => 1, :class => 2)
    m2 = m.dup
    assert_equal m, m2
    assert_equal 1, m['dup']
    assert_equal 2, m['class']
    m['dup'] = 3
    assert_equal 3, m['dup']
  end

  def test_int_ranges
    m = proto_module::TestMessage.new

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
    m = proto_module::TestMessage.new
    m.optional_int32 = 42
    m.optional_int64 = 0x100000000
    m.optional_string = "hello world"
    10.times do m.repeated_msg.push proto_module::TestMessage2.new(:foo => 42) end
    10.times do m.repeated_string.push "hello world" end

    data = proto_module::TestMessage.encode(m)

    10_000.times do
      m = proto_module::TestMessage.decode(data)
      data_new = proto_module::TestMessage.encode(m)
      assert_equal data, data_new
      data = data_new
    end
  end

  def test_reflection
    m = proto_module::TestMessage.new(:optional_int32 => 1234)
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
    assert_equal proto_module::TestMessage2.descriptor, optional_msg.subtype
    optional_msg.set(m, optional_msg.subtype.msgclass.new)

    assert_equal proto_module::TestMessage, msgdef.msgclass
    optional_enum = msgdef.lookup "optional_enum"
    assert_equal proto_module::TestEnum.descriptor, optional_enum.subtype
    assert_instance_of Google::Protobuf::EnumDescriptor, optional_enum.subtype
    optional_enum.subtype.each do |k, v|
      # set with integer, check resolution to symbolic name
      optional_enum.set(m, v)
      assert_equal optional_enum.get(m), k
    end
  end

  def test_json
    m = proto_module::TestMessage.new(:optional_int32 => 1234,
                                      :optional_int64 => -0x1_0000_0000,
                                      :optional_uint32 => 0x8000_0000,
                                      :optional_uint64 => 0xffff_ffff_ffff_ffff,
                                      :optional_bool => true,
                                      :optional_float => 1.0,
                                      :optional_double => -1e100,
                                      :optional_string => "Test string",
                                      :optional_bytes => ["FFFFFFFF"].pack('H*'),
                                      :optional_msg => proto_module::TestMessage2.new(:foo => 42),
                                      :repeated_int32 => [1, 2, 3, 4],
                                      :repeated_string => ["a", "b", "c"],
                                      :repeated_bool => [true, false, true, false],
                                      :repeated_msg => [proto_module::TestMessage2.new(:foo => 1),
                                                        proto_module::TestMessage2.new(:foo => 2)])

    json_text = proto_module::TestMessage.encode_json(m)
    m2 = proto_module::TestMessage.decode_json(json_text)
    assert_equal m, m2

    # Crash case from GitHub issue 283.
    bar = proto_module::Bar.new(msg: "bar")
    baz1 = proto_module::Baz.new(msg: "baz")
    baz2 = proto_module::Baz.new(msg: "quux")
    proto_module::Foo.encode_json(proto_module::Foo.new)
    proto_module::Foo.encode_json(proto_module::Foo.new(bar: bar))
    proto_module::Foo.encode_json(proto_module::Foo.new(bar: bar, baz: [baz1, baz2]))
  end

  def test_json_empty
    assert_equal '{}', proto_module::TestMessage.encode_json(proto_module::TestMessage.new)
  end

  def test_json_emit_defaults
    # TODO: Fix JSON in JRuby version.
    return if RUBY_PLATFORM == "java"
    m = proto_module::TestMessage.new

    expected = {
      repeatedInt32: [],
      repeatedInt64: [],
      repeatedUint32: [],
      repeatedUint64: [],
      repeatedBool: [],
      repeatedFloat: [],
      repeatedDouble: [],
      repeatedString: [],
      repeatedBytes: [],
      repeatedMsg: [],
      repeatedEnum: []
    }

    actual = proto_module::TestMessage.encode_json(m, :emit_defaults => true)

    assert_equal expected, JSON.parse(actual, :symbolize_names => true)
  end

  def test_json_emit_defaults_submsg
    # TODO: Fix JSON in JRuby version.
    return if RUBY_PLATFORM == "java"
    m = proto_module::TestMessage.new(optional_msg: proto_module::TestMessage2.new)

    expected = {
      optionalMsg: {},
      repeatedInt32: [],
      repeatedInt64: [],
      repeatedUint32: [],
      repeatedUint64: [],
      repeatedBool: [],
      repeatedFloat: [],
      repeatedDouble: [],
      repeatedString: [],
      repeatedBytes: [],
      repeatedMsg: [],
      repeatedEnum: []
    }

    actual = proto_module::TestMessage.encode_json(m, :emit_defaults => true)

    assert_equal expected, JSON.parse(actual, :symbolize_names => true)
  end

  def test_json_emit_defaults_repeated_submsg
    # TODO: Fix JSON in JRuby version.
    return if RUBY_PLATFORM == "java"
    m = proto_module::TestMessage.new(repeated_msg: [proto_module::TestMessage2.new])

    expected = {
      repeatedInt32: [],
      repeatedInt64: [],
      repeatedUint32: [],
      repeatedUint64: [],
      repeatedBool: [],
      repeatedFloat: [],
      repeatedDouble: [],
      repeatedString: [],
      repeatedBytes: [],
      repeatedMsg: [{}],
      repeatedEnum: []
    }

    actual = proto_module::TestMessage.encode_json(m, :emit_defaults => true)

    assert_equal expected, JSON.parse(actual, :symbolize_names => true)
  end

  def value_from_ruby(value)
    ret = Google::Protobuf::Value.new
    case value
    when String
      ret.string_value = value
    when Google::Protobuf::Struct
      ret.struct_value = value
    when Hash
      ret.struct_value = struct_from_ruby(value)
    when Google::Protobuf::ListValue
      ret.list_value = value
    when Array
      ret.list_value = list_from_ruby(value)
    else
      @log.error "Unknown type: #{value.class}"
      raise Google::Protobuf::Error, "Unknown type: #{value.class}"
    end
    ret
  end

  def list_from_ruby(arr)
    ret = Google::Protobuf::ListValue.new
    arr.each do |v|
      ret.values << value_from_ruby(v)
    end
    ret
  end

  def struct_from_ruby(hash)
    ret = Google::Protobuf::Struct.new
    hash.each do |k, v|
      ret.fields[k] ||= value_from_ruby(v)
    end
    ret
  end

  def test_deep_json
    # will not overflow
    json = '{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":'\
           '{"a":{"a":{"a":{"a":{}}}}}}}}}}}}}}}}'

    struct = struct_from_ruby(JSON.parse(json))
    assert_equal json, struct.to_json

    encoded = proto_module::MyRepeatedStruct.encode(
      proto_module::MyRepeatedStruct.new(structs: [proto_module::MyStruct.new(struct: struct)]))
    assert_equal json, proto_module::MyRepeatedStruct.decode(encoded).structs[0].struct.to_json

    # will overflow
    json = '{"a":{"a":{"a":[{"a":{"a":[{"a":[{"a":{"a":[{"a":[{"a":'\
           '{"a":[{"a":[{"a":{"a":{"a":[{"a":"a"}]}}}]}]}}]}]}}]}]}}]}}}'

    struct = struct_from_ruby(JSON.parse(json))
    assert_equal json, struct.to_json

    assert_raises(RuntimeError, "Recursion limit exceeded during encoding") do
      struct = Google::Protobuf::Struct.new
      struct.fields["foobar"] = Google::Protobuf::Value.new(struct_value: struct)
      Google::Protobuf::Struct.encode(struct)
    end
  end

  def test_comparison_with_arbitrary_object
    refute_nil proto_module::TestMessage.new
  end

  def test_wrappers_set_to_default
    run_asserts = ->(m) {
      assert_equal 0.0, m.double.value
      assert_equal 0.0, m.float.value
      assert_equal 0, m.int32.value
      assert_equal 0, m.int64.value
      assert_equal 0, m.uint32.value
      assert_equal 0, m.uint64.value
      refute m.bool.value
      assert_empty m.string.value
      assert_empty m.bytes.value
    }

    m = proto_module::Wrapper.new(
      double: Google::Protobuf::DoubleValue.new(value: 0.0),
      float: Google::Protobuf::FloatValue.new(value: 0.0),
      int32: Google::Protobuf::Int32Value.new(value: 0),
      int64: Google::Protobuf::Int64Value.new(value: 0),
      uint32: Google::Protobuf::UInt32Value.new(value: 0),
      uint64: Google::Protobuf::UInt64Value.new(value: 0),
      bool: Google::Protobuf::BoolValue.new(value: false),
      string: Google::Protobuf::StringValue.new(value: ""),
      bytes: Google::Protobuf::BytesValue.new(value: ''),
    )

    run_asserts.call(m)
    m2 = proto_module::Wrapper.decode(m.to_proto)
    run_asserts.call(m2)
    m3 = proto_module::Wrapper.decode_json(m.to_json)
    run_asserts.call(m3)
  end

  def test_wrapper_getters
    run_asserts = ->(m) {
      assert_equal 2.0, m.double_as_value
      assert_equal 2.0, m.double.value
      assert_equal 2.0, m.double_as_value

      assert_equal 4.0, m.float_as_value
      assert_equal 4.0, m.float.value
      assert_equal 4.0, m.float_as_value

      assert_equal 3, m.int32_as_value
      assert_equal 3, m.int32.value
      assert_equal 3, m.int32_as_value

      assert_equal 4, m.int64_as_value
      assert_equal 4, m.int64.value
      assert_equal 4, m.int64_as_value

      assert_equal 5, m.uint32_as_value
      assert_equal 5, m.uint32.value
      assert_equal 5, m.uint32_as_value

      assert_equal 6, m.uint64_as_value
      assert_equal 6, m.uint64.value
      assert_equal 6, m.uint64_as_value

      assert m.bool_as_value
      assert m.bool.value
      assert m.bool_as_value
      assert_equal "st\nr", m.string_as_value
      assert_equal "st\nr", m.string.value
      assert_equal "st\nr", m.string_as_value

      assert_equal 'fun', m.bytes_as_value
      assert_equal 'fun', m.bytes.value
      assert_equal 'fun', m.bytes_as_value
    }

    m = proto_module::Wrapper.new(
      double: Google::Protobuf::DoubleValue.new(value: 2.0),
      float: Google::Protobuf::FloatValue.new(value: 4.0),
      int32: Google::Protobuf::Int32Value.new(value: 3),
      int64: Google::Protobuf::Int64Value.new(value: 4),
      uint32: Google::Protobuf::UInt32Value.new(value: 5),
      uint64: Google::Protobuf::UInt64Value.new(value: 6),
      bool: Google::Protobuf::BoolValue.new(value: true),
      string: Google::Protobuf::StringValue.new(value: "st\nr"),
      bytes: Google::Protobuf::BytesValue.new(value: 'fun'),
      real_string: '100'
    )

    run_asserts.call(m)
    serialized = proto_module::Wrapper::encode(m)
    m2 = proto_module::Wrapper::decode(serialized)
    run_asserts.call(m2)

    # Test the case where we are serializing directly from the parsed form
    # (before anything lazy is materialized).
    m3 = proto_module::Wrapper::decode(serialized)
    serialized2 = proto_module::Wrapper::encode(m3)
    m4 = proto_module::Wrapper::decode(serialized2)
    run_asserts.call(m4)

    # Test that the lazy form compares equal to the expanded form.
    m5 = proto_module::Wrapper::decode(serialized2)
    assert_equal m5, m

    serialized_json = proto_module::Wrapper::encode_json(m)
    m6 = proto_module::Wrapper::decode_json(serialized_json)
    assert_equal m6, m
  end

  def test_repeated_wrappers
    run_asserts = ->(m) {
      assert_equal 2.0, m.repeated_double[0].value
      assert_equal 4.0, m.repeated_float[0].value
      assert_equal 3, m.repeated_int32[0].value
      assert_equal 4, m.repeated_int64[0].value
      assert_equal 5, m.repeated_uint32[0].value
      assert_equal 6, m.repeated_uint64[0].value
      assert m.repeated_bool[0].value
      assert_equal 'str', m.repeated_string[0].value
      assert_equal 'fun', m.repeated_bytes[0].value
    }

    m = proto_module::Wrapper.new(
      repeated_double: [Google::Protobuf::DoubleValue.new(value: 2.0)],
      repeated_float: [Google::Protobuf::FloatValue.new(value: 4.0)],
      repeated_int32: [Google::Protobuf::Int32Value.new(value: 3)],
      repeated_int64: [Google::Protobuf::Int64Value.new(value: 4)],
      repeated_uint32: [Google::Protobuf::UInt32Value.new(value: 5)],
      repeated_uint64: [Google::Protobuf::UInt64Value.new(value: 6)],
      repeated_bool: [Google::Protobuf::BoolValue.new(value: true)],
      repeated_string: [Google::Protobuf::StringValue.new(value: 'str')],
      repeated_bytes: [Google::Protobuf::BytesValue.new(value: 'fun')],
    )

    run_asserts.call(m)
    serialized = proto_module::Wrapper::encode(m)
    m2 = proto_module::Wrapper::decode(serialized)
    run_asserts.call(m2)

    # Test the case where we are serializing directly from the parsed form
    # (before anything lazy is materialized).
    m3 = proto_module::Wrapper::decode(serialized)
    serialized2 = proto_module::Wrapper::encode(m3)
    m4 = proto_module::Wrapper::decode(serialized2)
    run_asserts.call(m4)

    # Test that the lazy form compares equal to the expanded form.
    m5 = proto_module::Wrapper::decode(serialized2)
    assert_equal m5, m

    # Test JSON.
    serialized_json = proto_module::Wrapper::encode_json(m5)
    m6 = proto_module::Wrapper::decode_json(serialized_json)
    run_asserts.call(m6)
    assert_equal m6, m
  end

  def test_oneof_wrappers
    run_test = ->(m) {
      serialized = proto_module::Wrapper::encode(m)
      m2 = proto_module::Wrapper::decode(serialized)

      # Encode directly from lazy form.
      serialized2 = proto_module::Wrapper::encode(m2)

      assert_equal m, m2
      assert_equal serialized, serialized2

      serialized_json = proto_module::Wrapper::encode_json(m)
      m3 = proto_module::Wrapper::decode_json(serialized_json)
      assert_equal m, m3
    }

    m = proto_module::Wrapper.new()

    run_test.call(m)
    m.oneof_double_as_value = 2.0
    run_test.call(m)
    m.oneof_float_as_value = 4.0
    run_test.call(m)
    m.oneof_int32_as_value = 3
    run_test.call(m)
    m.oneof_int64_as_value = 5
    run_test.call(m)
    m.oneof_uint32_as_value = 6
    run_test.call(m)
    m.oneof_uint64_as_value = 7
    run_test.call(m)
    m.oneof_string_as_value = 'str'
    run_test.call(m)
    m.oneof_bytes_as_value = 'fun'
    run_test.call(m)
  end

  def test_top_level_wrappers
    # We don't expect anyone to do this, but we should also make sure it does
    # the right thing.
    run_test = ->(klass, val) {
      m = klass.new(value: val)
      serialized = klass::encode(m)
      m2 = klass::decode(serialized)

      # Encode directly from lazy form.
      serialized2 = klass::encode(m2)

      assert_equal m, m2
      assert_equal serialized, serialized2

      serialized_json = klass::encode_json(m)

      # This is nonsensical to do and does not work.  There is no good reason
      # to parse a wrapper type directly.
      assert_raises(RuntimeError) { klass::decode_json(serialized_json) }
    }

    run_test.call(Google::Protobuf::DoubleValue, 2.0)
    run_test.call(Google::Protobuf::FloatValue, 4.0)
    run_test.call(Google::Protobuf::Int32Value, 3)
    run_test.call(Google::Protobuf::Int64Value, 4)
    run_test.call(Google::Protobuf::UInt32Value, 5)
    run_test.call(Google::Protobuf::UInt64Value, 6)
    run_test.call(Google::Protobuf::BoolValue, true)
    run_test.call(Google::Protobuf::StringValue, 'str')
    run_test.call(Google::Protobuf::BytesValue, 'fun')
  end

  def test_wrapper_setters_as_value
    run_asserts = ->(m) {
      m.double_as_value = 4.8
      assert_equal 4.8, m.double_as_value
      assert_equal Google::Protobuf::DoubleValue.new(value: 4.8), m.double
      m.float_as_value = 2.4
      assert_in_delta 2.4, m.float_as_value
      assert_in_delta Google::Protobuf::FloatValue.new(value: 2.4).value, m.float.value
      m.int32_as_value = 5
      assert_equal 5, m.int32_as_value
      assert_equal Google::Protobuf::Int32Value.new(value: 5), m.int32
      m.int64_as_value = 15
      assert_equal 15, m.int64_as_value
      assert_equal Google::Protobuf::Int64Value.new(value: 15), m.int64
      m.uint32_as_value = 50
      assert_equal 50, m.uint32_as_value
      assert_equal Google::Protobuf::UInt32Value.new(value: 50), m.uint32
      m.uint64_as_value = 500
      assert_equal 500, m.uint64_as_value
      assert_equal Google::Protobuf::UInt64Value.new(value: 500), m.uint64
      m.bool_as_value = false
      refute m.bool_as_value
      assert_equal Google::Protobuf::BoolValue.new(value: false), m.bool
      m.string_as_value = 'xy'
      assert_equal 'xy', m.string_as_value
      assert_equal Google::Protobuf::StringValue.new(value: 'xy'), m.string
      m.bytes_as_value = '123'
      assert_equal '123', m.bytes_as_value
      assert_equal Google::Protobuf::BytesValue.new(value: '123'), m.bytes

      m.double_as_value = nil
      assert_nil m.double
      assert_nil m.double_as_value
      m.float_as_value = nil
      assert_nil m.float
      assert_nil m.float_as_value
      m.int32_as_value = nil
      assert_nil m.int32
      assert_nil m.int32_as_value
      m.int64_as_value = nil
      assert_nil m.int64
      assert_nil m.int64_as_value
      m.uint32_as_value = nil
      assert_nil m.uint32
      assert_nil m.uint32_as_value
      m.uint64_as_value = nil
      assert_nil m.uint64
      assert_nil m.uint64_as_value
      m.bool_as_value = nil
      assert_nil m.bool
      assert_nil m.bool_as_value
      m.string_as_value = nil
      assert_nil m.string
      assert_nil m.string_as_value
      m.bytes_as_value = nil
      assert_nil m.bytes
      assert_nil m.bytes_as_value
    }

    m2 = proto_module::Wrapper.new(
      double: Google::Protobuf::DoubleValue.new(value: 2.0),
      float: Google::Protobuf::FloatValue.new(value: 4.0),
      int32: Google::Protobuf::Int32Value.new(value: 3),
      int64: Google::Protobuf::Int64Value.new(value: 4),
      uint32: Google::Protobuf::UInt32Value.new(value: 5),
      uint64: Google::Protobuf::UInt64Value.new(value: 6),
      bool: Google::Protobuf::BoolValue.new(value: true),
      string: Google::Protobuf::StringValue.new(value: 'str'),
      bytes: Google::Protobuf::BytesValue.new(value: 'fun'),
      real_string: '100'
    )

    run_asserts.call(m2)

    serialized = proto_module::Wrapper::encode(m2)
    m3 = proto_module::Wrapper::decode(serialized)
    run_asserts.call(m3)
  end

  def test_wrapper_setters
    run_asserts = ->(m) {
      m.double = Google::Protobuf::DoubleValue.new(value: 4.8)
      assert_equal 4.8, m.double_as_value
      assert_equal Google::Protobuf::DoubleValue.new(value: 4.8), m.double
      m.float = Google::Protobuf::FloatValue.new(value: 2.4)
      assert_in_delta 2.4, m.float_as_value
      assert_in_delta Google::Protobuf::FloatValue.new(value: 2.4).value, m.float.value
      m.int32 = Google::Protobuf::Int32Value.new(value: 5)
      assert_equal 5, m.int32_as_value
      assert_equal Google::Protobuf::Int32Value.new(value: 5), m.int32
      m.int64 = Google::Protobuf::Int64Value.new(value: 15)
      assert_equal 15, m.int64_as_value
      assert_equal Google::Protobuf::Int64Value.new(value: 15), m.int64
      m.uint32 = Google::Protobuf::UInt32Value.new(value: 50)
      assert_equal 50, m.uint32_as_value
      assert_equal Google::Protobuf::UInt32Value.new(value: 50), m.uint32
      m.uint64 = Google::Protobuf::UInt64Value.new(value: 500)
      assert_equal 500, m.uint64_as_value
      assert_equal Google::Protobuf::UInt64Value.new(value: 500), m.uint64
      m.bool = Google::Protobuf::BoolValue.new(value: false)
      refute m.bool_as_value
      assert_equal Google::Protobuf::BoolValue.new(value: false), m.bool
      m.string = Google::Protobuf::StringValue.new(value: 'xy')
      assert_equal 'xy', m.string_as_value
      assert_equal Google::Protobuf::StringValue.new(value: 'xy'), m.string
      m.bytes = Google::Protobuf::BytesValue.new(value: '123')
      assert_equal '123', m.bytes_as_value
      assert_equal Google::Protobuf::BytesValue.new(value: '123'), m.bytes

      m.double = nil
      assert_nil m.double
      assert_nil m.double_as_value
      m.float = nil
      assert_nil m.float
      assert_nil m.float_as_value
      m.int32 = nil
      assert_nil m.int32
      assert_nil m.int32_as_value
      m.int64 = nil
      assert_nil m.int64
      assert_nil m.int64_as_value
      m.uint32 = nil
      assert_nil m.uint32
      assert_nil m.uint32_as_value
      m.uint64 = nil
      assert_nil m.uint64
      assert_nil m.uint64_as_value
      m.bool = nil
      assert_nil m.bool
      assert_nil m.bool_as_value
      m.string = nil
      assert_nil m.string
      assert_nil m.string_as_value
      m.bytes = nil
      assert_nil m.bytes
      assert_nil m.bytes_as_value
    }

    m = proto_module::Wrapper.new
    run_asserts.call(m)

    m2 = proto_module::Wrapper.new(
      double: Google::Protobuf::DoubleValue.new(value: 2.0),
      float: Google::Protobuf::FloatValue.new(value: 4.0),
      int32: Google::Protobuf::Int32Value.new(value: 3),
      int64: Google::Protobuf::Int64Value.new(value: 4),
      uint32: Google::Protobuf::UInt32Value.new(value: 5),
      uint64: Google::Protobuf::UInt64Value.new(value: 6),
      bool: Google::Protobuf::BoolValue.new(value: true),
      string: Google::Protobuf::StringValue.new(value: 'str'),
      bytes: Google::Protobuf::BytesValue.new(value: 'fun'),
      real_string: '100'
    )

    run_asserts.call(m2)

    serialized = proto_module::Wrapper::encode(m2)
    m3 = proto_module::Wrapper::decode(serialized)
    run_asserts.call(m3)
  end

  def test_wrappers_only
    m = proto_module::Wrapper.new(real_string: 'hi', string_in_oneof: 'there')

    assert_raises(NoMethodError) { m.real_string_as_value }
    assert_raises(NoMethodError) { m.as_value }
    assert_raises(NoMethodError) { m._as_value }
    assert_raises(NoMethodError) { m.string_in_oneof_as_value }

    m = proto_module::Wrapper.new
    m.string_as_value = 'you'
    assert_equal 'you', m.string.value
    assert_equal 'you', m.string_as_value
    assert_raises(NoMethodError) { m.string_ }
    assert_raises(NoMethodError) { m.string_X }
    assert_raises(NoMethodError) { m.string_XX }
    assert_raises(NoMethodError) { m.string_XXX }
    assert_raises(NoMethodError) { m.string_XXXX }
    assert_raises(NoMethodError) { m.string_XXXXX }
    assert_raises(NoMethodError) { m.string_XXXXXX }
    assert_raises(NoMethodError) { m.string_XXXXXXX }
    assert_raises(NoMethodError) { m.string_XXXXXXXX }
    assert_raises(NoMethodError) { m.string_XXXXXXXXX }
    assert_raises(NoMethodError) { m.string_XXXXXXXXXX }
  end

  def test_converts_time
    m = proto_module::TimeMessage.new

    m.timestamp = Google::Protobuf::Timestamp.new(seconds: 5, nanos: 6)
    assert_kind_of Google::Protobuf::Timestamp, m.timestamp
    assert_equal 5, m.timestamp.seconds
    assert_equal 6, m.timestamp.nanos

    m.timestamp = Time.at(9466, 123456.789)
    assert_equal Google::Protobuf::Timestamp.new(seconds: 9466, nanos: 123456789), m.timestamp

    m = proto_module::TimeMessage.new(timestamp: Time.at(1))
    assert_equal Google::Protobuf::Timestamp.new(seconds: 1, nanos: 0), m.timestamp

    assert_raises(Google::Protobuf::TypeError) { m.timestamp = 2 }
    assert_raises(Google::Protobuf::TypeError) { m.timestamp = 2.4 }
    assert_raises(Google::Protobuf::TypeError) { m.timestamp = '4' }
    assert_raises(Google::Protobuf::TypeError) { m.timestamp = proto_module::TimeMessage.new }

    def test_time(year, month, day)
      str = ("\"%04d-%02d-%02dT00:00:00.000+00:00\"" % [year, month, day])
      t = Google::Protobuf::Timestamp.decode_json(str)
      time = Time.new(year, month, day, 0, 0, 0, "+00:00")
      assert_equal t.seconds, time.to_i
    end

    (1970..2010).each do |year|
      test_time(year, 2, 28)
      test_time(year, 3, 01)
    end
  end

  def test_converts_duration
    m = proto_module::TimeMessage.new

    m.duration = Google::Protobuf::Duration.new(seconds: 2, nanos: 22)
    assert_kind_of Google::Protobuf::Duration, m.duration
    assert_equal 2, m.duration.seconds
    assert_equal 22, m.duration.nanos

    m.duration = 10.5
    assert_equal Google::Protobuf::Duration.new(seconds: 10, nanos: 500_000_000), m.duration

    m.duration = 200
    assert_equal Google::Protobuf::Duration.new(seconds: 200, nanos: 0), m.duration

    m.duration = Rational(3, 2)
    assert_equal Google::Protobuf::Duration.new(seconds: 1, nanos: 500_000_000), m.duration

    m.duration = BigDecimal("5")
    assert_equal Google::Protobuf::Duration.new(seconds: 5, nanos: 0), m.duration

    m = proto_module::TimeMessage.new(duration: 1.1)
    assert_equal Google::Protobuf::Duration.new(seconds: 1, nanos: 100_000_000), m.duration

    m = proto_module::TimeMessage.new(duration: 123.321)
    assert_equal Google::Protobuf::Duration.new(seconds: 123, nanos: 321_000_000), m.duration

    m = proto_module::TimeMessage.new(duration: -123.321)
    assert_equal Google::Protobuf::Duration.new(seconds: -123, nanos: -321_000_000), m.duration

    assert_raises(Google::Protobuf::TypeError) { m.duration = '2' }
    assert_raises(Google::Protobuf::TypeError) { m.duration = proto_module::TimeMessage.new }
  end

  def test_freeze
    m = proto_module::TestMessage.new
    m.optional_int32 = 10
    m.freeze

    frozen_error = assert_raises(FrozenError) { m.optional_int32 = 20 }
    assert_match "can't modify frozen #{proto_module}::TestMessage", frozen_error.message
    assert_equal 10, m.optional_int32
    assert m.frozen?
    assert_raises(FrozenError) { m.optional_int64 = 2 }
    assert_raises(FrozenError) { m.optional_uint32 = 3 }
    assert_raises(FrozenError) { m.optional_uint64 = 4 }
    assert_raises(FrozenError) { m.optional_bool = true }
    assert_raises(FrozenError) { m.optional_float = 6.0 }
    assert_raises(FrozenError) { m.optional_double = 7.0 }
    assert_raises(FrozenError) { m.optional_string = '8' }
    assert_raises(FrozenError) { m.optional_bytes = nil }
    assert_raises(FrozenError) { m.optional_msg = proto_module::TestMessage2.new }
    assert_raises(FrozenError) { m.optional_enum = :A }
    assert_raises(FrozenError) { m.repeated_int32 = 1 }
    assert_raises(FrozenError) { m.repeated_int64 = 2 }
    assert_raises(FrozenError) { m.repeated_uint32 = 3 }
    assert_raises(FrozenError) { m.repeated_uint64 = 4 }
    assert_raises(FrozenError) { m.repeated_bool = true }
    assert_raises(FrozenError) { m.repeated_float = 6.0 }
    assert_raises(FrozenError) { m.repeated_double = 7.0 }
    assert_raises(FrozenError) { m.repeated_string = '8' }
    assert_raises(FrozenError) { m.repeated_bytes = nil }
    assert_raises(FrozenError) { m.repeated_msg = proto_module::TestMessage2.new }
    assert_raises(FrozenError) { m.repeated_enum = :A }
  end

  def test_eq
    m1 = proto_module::TestMessage.new(:optional_string => 'foo', :repeated_string => ['bar1', 'bar2'])
    m2 = proto_module::TestMessage.new(:optional_string => 'foo', :repeated_string => ['bar1', 'bar2'])

    h = {}
    h[m1] = :yes

    assert_equal m1, m2
    assert m1.eql?(m2)
    assert_equal m1.hash, m2.hash
    assert_equal :yes, h[m1]
    assert_equal :yes, h[m2]
    m1.optional_int32 = 2

    refute_equal m1, m2
    refute m1.eql?(m2)
    refute_equal m1.hash, m2.hash
    assert_nil h[m2]
  end

  def cruby_or_jruby_9_3_or_higher?
    # https://github.com/jruby/jruby/issues/6818 was fixed in JRuby 9.3.0.0
    match = RUBY_PLATFORM == "java" &&
      JRUBY_VERSION.match(/^(\d+)\.(\d+)\.\d+\.\d+$/)
    match && (match[1].to_i > 9 || (match[1].to_i == 9 && match[2].to_i >= 3))
  end

  def test_object_gc
    m = proto_module::TestMessage.new(optional_msg: proto_module::TestMessage2.new)
    m.optional_msg
    # https://github.com/jruby/jruby/issues/6818 was fixed in JRuby 9.3.0.0
    GC.start(full_mark: true, immediate_sweep: true) if cruby_or_jruby_9_3_or_higher?
    m.optional_msg.inspect
  end

  def test_object_gc_freeze
    m = proto_module::TestMessage.new
    m.repeated_float.freeze
    # https://github.com/jruby/jruby/issues/6818 was fixed in JRuby 9.3.0.0
    GC.start(full_mark: true) if cruby_or_jruby_9_3_or_higher?

    # Make sure we remember that the object is frozen.
    # The wrapper object contains this information, so we need to ensure that
    # the previous GC did not collect it.
    assert m.repeated_float.frozen?

    # https://github.com/jruby/jruby/issues/6818 was fixed in JRuby 9.3.0.0
    GC.start(full_mark: true, immediate_sweep: true) if cruby_or_jruby_9_3_or_higher?
    assert m.repeated_float.frozen?
  end

  def test_optional_fields_respond_to? # regression test for issue 9202
    msg = proto_module::TestMessage.new
    assert msg.respond_to? :optional_int32=
    msg.optional_int32 = 42

    assert msg.respond_to? :optional_int32
    assert_equal 42, msg.optional_int32

    assert msg.respond_to? :clear_optional_int32
    msg.clear_optional_int32
    assert_equal 0, msg.optional_int32

    assert msg.respond_to? :has_optional_int32?
    refute msg.has_optional_int32?

    refute_respond_to msg, :optional_int32_as_value=
    assert_raises NoMethodError do
      msg.optional_int32_as_value = 42
    end

    refute_respond_to msg, :optional_int32_as_value
    assert_raises NoMethodError do
      msg.optional_int32_as_value
    end

    assert msg.respond_to? :optional_enum_const
    assert_equal 0, msg.optional_enum_const

    refute_respond_to msg, :foo
    assert_raises NoMethodError do
      msg.foo
    end

    refute_respond_to msg, :foo_const
    assert_raises NoMethodError do
      msg.foo_const
    end

    refute_respond_to msg, :optional_int32_const
    assert_raises NoMethodError do
      msg.optional_int32_const
    end
  end

  def test_oneof_fields_respond_to? # regression test for issue 9202
    msg = proto_module::OneofMessage.new

    # names of the elements of a oneof and the oneof itself are valid actions.
    assert msg.respond_to? :my_oneof
    assert_nil msg.my_oneof
    assert msg.respond_to? :a
    assert_empty msg.a
    assert msg.respond_to? :b
    assert_equal 0, msg.b
    assert msg.respond_to? :c
    assert_nil msg.c
    assert msg.respond_to? :d
    assert_equal :Default, msg.d

    # `clear` prefix actions work on elements of a oneof and the oneof itself.
    assert msg.respond_to? :clear_my_oneof
    msg.clear_my_oneof
    # Repeatedly clearing a oneof used to cause a NoMethodError under JRuby
    msg.clear_my_oneof
    assert msg.respond_to? :clear_a
    msg.clear_a
    assert msg.respond_to? :clear_b
    msg.clear_b
    assert msg.respond_to? :clear_c
    msg.clear_c
    assert msg.respond_to? :clear_d
    msg.clear_d

    # `=` suffix actions should work on elements of a oneof but not the oneof itself.
    refute_respond_to msg, :my_oneof=
    error = assert_raises RuntimeError do
      msg.my_oneof = nil
    end
    assert_equal "Oneof accessors are read-only.", error.message
    assert msg.respond_to? :a=
    msg.a = "foo"
    assert msg.respond_to? :b=
    msg.b = 42
    assert msg.respond_to? :c=
    msg.c = proto_module::TestMessage2.new
    assert msg.respond_to? :d=
    msg.d = :Default

    # `has_` prefix + "?" suffix actions work for oneofs fields.
    assert msg.respond_to? :has_my_oneof?
    assert msg.has_my_oneof?

    # `_as_value` suffix actions should only work for wrapped fields.
    refute_respond_to msg, :my_oneof_as_value
    assert_raises NoMethodError do
      msg.my_oneof_as_value
    end
    refute_respond_to msg, :a_as_value
    assert_raises NoMethodError do
      msg.a_as_value
    end
    refute_respond_to msg, :b_as_value
    assert_raises NoMethodError do
      msg.b_as_value
    end
    refute_respond_to msg, :c_as_value
    assert_raises NoMethodError do
      msg.c_as_value
    end
    refute_respond_to msg, :d_as_value
    assert_raises NoMethodError do
      msg.d_as_value
    end

    # `_as_value=` suffix actions should only work for wrapped fields.
    refute_respond_to msg, :my_oneof_as_value=
    assert_raises NoMethodError do
      msg.my_oneof_as_value = :boom
    end
    refute_respond_to msg, :a_as_value=
    assert_raises NoMethodError do
      msg.a_as_value = ""
    end
    refute_respond_to msg, :b_as_value=
    assert_raises NoMethodError do
      msg.b_as_value = 42
    end
    refute_respond_to msg, :c_as_value=
    assert_raises NoMethodError do
      msg.c_as_value = proto_module::TestMessage2.new
    end
    refute_respond_to msg, :d_as_value=
    assert_raises NoMethodError do
      msg.d_as_value = :Default
    end

    # `_const` suffix actions should only work for enum fields.
    refute_respond_to msg, :my_oneof_const
    assert_raises NoMethodError do
      msg.my_oneof_const
    end
    refute_respond_to msg, :a_const
    assert_raises NoMethodError do
      msg.a_const
    end
    refute_respond_to msg, :b_const
    assert_raises NoMethodError do
      msg.b_const
    end
    refute_respond_to msg, :c_const
    assert_raises NoMethodError do
      msg.c_const
    end
    assert msg.respond_to? :d_const
    assert_equal 0, msg.d_const
  end

  def test_wrapped_fields_respond_to? # regression test for issue 9202
    msg = proto_module::Wrapper.new
    assert_respond_to msg, :double_as_value=
    msg.double_as_value = 42
    assert_respond_to msg, :double_as_value
    assert_equal 42, msg.double_as_value
    assert_equal Google::Protobuf::DoubleValue.new(value: 42), msg.double
  end
end
