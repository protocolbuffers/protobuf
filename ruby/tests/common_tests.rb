# Defines tests which are common between proto2 and proto3 syntax.
#
# Requires that the proto messages are exactly the same in proto2 and proto3 syntax
# and that the including class should define a 'proto_module' method which returns
# the enclosing module of the proto message classes.
module CommonTests
  def test_defaults
    m = proto_module::TestMessage.new
    assert m.optional_int32 == 0
    assert m.optional_int64 == 0
    assert m.optional_uint32 == 0
    assert m.optional_uint64 == 0
    assert m.optional_bool == false
    assert m.optional_float == 0.0
    assert m.optional_double == 0.0
    assert m.optional_string == ""
    assert m.optional_bytes == ""
    assert m.optional_msg == nil
    assert m.optional_enum == :Default
  end

  def test_setters
    m = proto_module::TestMessage.new
    m.optional_int32 = -42
    assert m.optional_int32 == -42
    m.optional_int64 = -0x1_0000_0000
    assert m.optional_int64 == -0x1_0000_0000
    m.optional_uint32 = 0x9000_0000
    assert m.optional_uint32 == 0x9000_0000
    m.optional_uint64 = 0x9000_0000_0000_0000
    assert m.optional_uint64 == 0x9000_0000_0000_0000
    m.optional_bool = true
    assert m.optional_bool == true
    m.optional_float = 0.5
    assert m.optional_float == 0.5
    m.optional_double = 0.5
    assert m.optional_double == 0.5
    m.optional_string = "hello"
    assert m.optional_string == "hello"
    m.optional_string = :hello
    assert m.optional_string == "hello"
    m.optional_bytes = "world".encode!('ASCII-8BIT')
    assert m.optional_bytes == "world"
    m.optional_msg = proto_module::TestMessage2.new(:foo => 42)
    assert m.optional_msg == proto_module::TestMessage2.new(:foo => 42)
    m.optional_msg = nil
    assert m.optional_msg == nil
    m.optional_enum = :C
    assert m.optional_enum == :C
    m.optional_enum = 'C'
    assert m.optional_enum == :C
  end

  def test_ctor_args
    m = proto_module::TestMessage.new(:optional_int32 => -42,
                                      :optional_msg => proto_module::TestMessage2.new,
                                      :optional_enum => :C,
                                      :repeated_string => ["hello", "there", "world"])
    assert m.optional_int32 == -42
    assert m.optional_msg.class == proto_module::TestMessage2
    assert m.repeated_string.length == 3
    assert m.optional_enum == :C
    assert m.repeated_string[0] == "hello"
    assert m.repeated_string[1] == "there"
    assert m.repeated_string[2] == "world"
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
    assert_equal "", m.optional_string
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

    assert_not_nil m.child_msg
    assert_not_nil m.child_msg.sub_child
    assert_equal m.child_msg.sub_child.optional_int32, 1

    assert_not_nil m.repeated_msg
    assert_equal 1, m.repeated_msg.length
    assert_equal 3, m.repeated_msg.first.sub_child.optional_int32
  end

  def test_inspect
    m = proto_module::TestMessage.new(
      :optional_int32 => -42,
      :optional_enum => :A,
      :optional_msg => proto_module::TestMessage2.new,
      :repeated_string => ["hello", "there", "world"])
    expected = "<#{proto_module}::TestMessage: optional_int32: -42, optional_int64: 0, optional_uint32: 0, optional_uint64: 0, optional_bool: false, optional_float: 0.0, optional_double: 0.0, optional_string: \"\", optional_bytes: \"\", optional_msg: <#{proto_module}::TestMessage2: foo: 0>, optional_enum: :A, repeated_int32: [], repeated_int64: [], repeated_uint32: [], repeated_uint64: [], repeated_bool: [], repeated_float: [], repeated_double: [], repeated_string: [\"hello\", \"there\", \"world\"], repeated_bytes: [], repeated_msg: [], repeated_enum: []>"
    assert_equal expected, m.inspect

    m = proto_module::OneofMessage.new(:b => -42)
    expected = "<#{proto_module}::OneofMessage: a: \"\", b: -42, c: nil, d: :Default>"
    assert_equal expected, m.inspect
  end

  def test_hash
    m1 = proto_module::TestMessage.new(:optional_int32 => 42)
    m2 = proto_module::TestMessage.new(:optional_int32 => 102, repeated_string: ['please', 'work', 'ok?'])
    m3 = proto_module::TestMessage.new(:optional_int32 => 102, repeated_string: ['please', 'work', 'ok?'])
    assert m1.hash != 0
    assert m2.hash != 0
    assert m3.hash != 0
    # relying on the randomness here -- if hash function changes and we are
    # unlucky enough to get a collision, then change the values above.
    assert m1.hash != m2.hash
    assert_equal m2.hash, m3.hash
  end

  def test_unknown_field_errors
    e = assert_raise NoMethodError do
      proto_module::TestMessage.new.hello
    end
    assert_match(/hello/, e.message)

    e = assert_raise NoMethodError do
      proto_module::TestMessage.new.hello = "world"
    end
    assert_match(/hello/, e.message)
  end

  def test_type_errors
    m = proto_module::TestMessage.new
    e = assert_raise Google::Protobuf::TypeError do
      m.optional_int32 = "hello"
    end

    # Google::Protobuf::TypeError should inherit from TypeError for backwards compatibility
    # TODO: This can be removed when we can safely migrate to Google::Protobuf::TypeError
    assert e.is_a?(::TypeError)

    assert_raise Google::Protobuf::TypeError do
      m.optional_string = 42
    end
    assert_raise Google::Protobuf::TypeError do
      m.optional_string = nil
    end
    assert_raise Google::Protobuf::TypeError do
      m.optional_bool = 42
    end
    assert_raise Google::Protobuf::TypeError do
      m.optional_msg = proto_module::TestMessage.new  # expects TestMessage2
    end

    assert_raise Google::Protobuf::TypeError do
      m.repeated_int32 = []  # needs RepeatedField
    end

    assert_raise Google::Protobuf::TypeError do
      m.repeated_int32.push "hello"
    end

    assert_raise Google::Protobuf::TypeError do
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

    assert_raise Encoding::UndefinedConversionError do
      m.optional_bytes = "Test string UTF-8 \u0100".encode!('UTF-8')
    end

    assert_raise Encoding::UndefinedConversionError do
      m.optional_string = ["FFFF"].pack('H*')
    end

    # "Ordinary" use case.
    m.optional_bytes = ["FFFF"].pack('H*')
    m.optional_string = "\u0100"

    # strings are immutable so we can't do this, but serialize should catch it.
    m.optional_string = "asdf".encode!('UTF-8')
    # Ruby 2.5 changed to raise FrozenError. However, assert_raise don't
    # accept subclass.
    ok = false
    begin
      m.optional_string.encode!('ASCII-8BIT')
    rescue RuntimeError
      ok = true
    end
    assert ok
  end

  def test_rptfield_int32
    l = Google::Protobuf::RepeatedField.new(:int32)
    assert l.count == 0
    l = Google::Protobuf::RepeatedField.new(:int32, [1, 2, 3])
    assert l.count == 3
    assert_equal [1, 2, 3], l
    assert_equal l, [1, 2, 3]
    l.push 4
    assert l == [1, 2, 3, 4]
    dst_list = []
    l.each { |val| dst_list.push val }
    assert dst_list == [1, 2, 3, 4]
    assert l.to_a == [1, 2, 3, 4]
    assert l[0] == 1
    assert l[3] == 4
    l[0] = 5
    assert l == [5, 2, 3, 4]

    l2 = l.dup
    assert l == l2
    assert l.object_id != l2.object_id
    l2.push 6
    assert l.count == 4
    assert l2.count == 5

    assert l.inspect == '[5, 2, 3, 4]'

    l.concat([7, 8, 9])
    assert l == [5, 2, 3, 4, 7, 8, 9]
    assert l.pop == 9
    assert l == [5, 2, 3, 4, 7, 8]

    assert_raise Google::Protobuf::TypeError do
      m = proto_module::TestMessage.new
      l.push m
    end

    m = proto_module::TestMessage.new
    m.repeated_int32 = l
    assert m.repeated_int32 == [5, 2, 3, 4, 7, 8]
    assert m.repeated_int32.object_id == l.object_id
    l.push 42
    assert m.repeated_int32.pop == 42

    l3 = l + l.dup
    assert l3.count == l.count * 2
    l.count.times do |i|
      assert l3[i] == l[i]
      assert l3[l.count + i] == l[i]
    end

    l.clear
    assert l.count == 0
    l += [1, 2, 3, 4]
    l.replace([5, 6, 7, 8])
    assert l == [5, 6, 7, 8]

    l4 = Google::Protobuf::RepeatedField.new(:int32)
    l4[5] = 42
    assert l4 == [0, 0, 0, 0, 0, 42]

    l4 << 100
    assert l4 == [0, 0, 0, 0, 0, 42, 100]
    l4 << 101 << 102
    assert l4 == [0, 0, 0, 0, 0, 42, 100, 101, 102]
  end

  def test_parent_rptfield
    #make sure we set the RepeatedField and can add to it
    m = proto_module::TestMessage.new
    assert m.repeated_string == []
    m.repeated_string << 'ok'
    m.repeated_string.push('ok2')
    assert m.repeated_string == ['ok', 'ok2']
    m.repeated_string += ['ok3']
    assert m.repeated_string == ['ok', 'ok2', 'ok3']
  end

  def test_rptfield_msg
    l = Google::Protobuf::RepeatedField.new(:message, proto_module::TestMessage)
    l.push proto_module::TestMessage.new
    assert l.count == 1
    assert_raise Google::Protobuf::TypeError do
      l.push proto_module::TestMessage2.new
    end
    assert_raise Google::Protobuf::TypeError do
      l.push 42
    end

    l2 = l.dup
    assert l2[0] == l[0]
    assert l2[0].object_id == l[0].object_id

    l2 = Google::Protobuf.deep_copy(l)
    assert l2[0] == l[0]
    assert l2[0].object_id != l[0].object_id

    l3 = l + l2
    assert l3.count == 2
    assert l3[0] == l[0]
    assert l3[1] == l2[0]
    l3[0].optional_int32 = 1000
    assert l[0].optional_int32 == 1000

    new_msg = proto_module::TestMessage.new(:optional_int32 => 200)
    l4 = l + [new_msg]
    assert l4.count == 2
    new_msg.optional_int32 = 1000
    assert l4[1].optional_int32 == 1000
  end

  def test_rptfield_enum
    l = Google::Protobuf::RepeatedField.new(:enum, proto_module::TestEnum)
    l.push :A
    l.push :B
    l.push :C
    assert l.count == 3
    assert_raise RangeError do
      l.push :D
    end
    assert l[0] == :A

    l.push 4
    assert l[3] == 4
  end

  def test_rptfield_initialize
    assert_raise ArgumentError do
      Google::Protobuf::RepeatedField.new
    end
    assert_raise ArgumentError do
      Google::Protobuf::RepeatedField.new(:message)
    end
    assert_raise ArgumentError do
      Google::Protobuf::RepeatedField.new([1, 2, 3])
    end
    assert_raise ArgumentError do
      Google::Protobuf::RepeatedField.new(:message, [proto_module::TestMessage2.new])
    end
  end

  def test_rptfield_array_ducktyping
    l = Google::Protobuf::RepeatedField.new(:int32)
    length_methods = %w(count length size)
    length_methods.each do |lm|
      assert l.send(lm)  == 0
    end
    # out of bounds returns a nil
    assert l[0] == nil
    assert l[1] == nil
    assert l[-1] == nil
    l.push 4
    length_methods.each do |lm|
      assert l.send(lm) == 1
    end
    assert l[0] == 4
    assert l[1] == nil
    assert l[-1] == 4
    assert l[-2] == nil

    l.push 2
    length_methods.each do |lm|
      assert l.send(lm) == 2
    end
    assert l[0] == 4
    assert l[1] == 2
    assert l[2] == nil
    assert l[-1] == 2
    assert l[-2] == 4
    assert l[-3] == nil

    #adding out of scope will backfill with empty objects
  end

  def test_map_basic
    # allowed key types:
    # :int32, :int64, :uint32, :uint64, :bool, :string, :bytes.

    m = Google::Protobuf::Map.new(:string, :int32)
    m["asdf"] = 1
    assert m["asdf"] == 1
    m["jkl;"] = 42
    assert m == { "jkl;" => 42, "asdf" => 1 }
    assert m.has_key?("asdf")
    assert !m.has_key?("qwerty")
    assert m.length == 2

    m2 = m.dup
    assert_equal m, m2
    assert m.hash != 0
    assert_equal m.hash, m2.hash

    collected = {}
    m.each { |k,v| collected[v] = k }
    assert collected == { 42 => "jkl;", 1 => "asdf" }

    assert m.delete("asdf") == 1
    assert !m.has_key?("asdf")
    assert m["asdf"] == nil
    assert !m.has_key?("asdf")

    # We only assert on inspect value when there is one map entry because the
    # order in which elements appear is unspecified (depends on the internal
    # hash function). We don't want a brittle test.
    assert m.inspect == "{\"jkl;\"=>42}"

    assert m.keys == ["jkl;"]
    assert m.values == [42]

    m.clear
    assert m.length == 0
    assert m == {}

    assert_raise TypeError do
      m[1] = 1
    end
    assert_raise RangeError do
      m["asdf"] = 0x1_0000_0000
    end
  end

  def test_map_ctor
    m = Google::Protobuf::Map.new(:string, :int32,
                                  {"a" => 1, "b" => 2, "c" => 3})
    assert m == {"a" => 1, "c" => 3, "b" => 2}
  end

  def test_map_keytypes
    m = Google::Protobuf::Map.new(:int32, :int32)
    m[1] = 42
    m[-1] = 42
    assert_raise RangeError do
      m[0x8000_0000] = 1
    end
    assert_raise Google::Protobuf::TypeError do
      m["asdf"] = 1
    end

    m = Google::Protobuf::Map.new(:int64, :int32)
    m[0x1000_0000_0000_0000] = 1
    assert_raise RangeError do
      m[0x1_0000_0000_0000_0000] = 1
    end
    assert_raise Google::Protobuf::TypeError do
      m["asdf"] = 1
    end

    m = Google::Protobuf::Map.new(:uint32, :int32)
    m[0x8000_0000] = 1
    assert_raise RangeError do
      m[0x1_0000_0000] = 1
    end
    assert_raise RangeError do
      m[-1] = 1
    end

    m = Google::Protobuf::Map.new(:uint64, :int32)
    m[0x8000_0000_0000_0000] = 1
    assert_raise RangeError do
      m[0x1_0000_0000_0000_0000] = 1
    end
    assert_raise RangeError do
      m[-1] = 1
    end

    m = Google::Protobuf::Map.new(:bool, :int32)
    m[true] = 1
    m[false] = 2
    assert_raise Google::Protobuf::TypeError do
      m[1] = 1
    end
    assert_raise Google::Protobuf::TypeError do
      m["asdf"] = 1
    end

    m = Google::Protobuf::Map.new(:string, :int32)
    m["asdf"] = 1
    assert_raise TypeError do
      m[1] = 1
    end
    assert_raise Encoding::UndefinedConversionError do
      bytestring = ["FFFF"].pack("H*")
      m[bytestring] = 1
    end

    m = Google::Protobuf::Map.new(:bytes, :int32)
    bytestring = ["FFFF"].pack("H*")
    m[bytestring] = 1
    # Allowed -- we will automatically convert to ASCII-8BIT.
    m["asdf"] = 1
    assert_raise TypeError do
      m[1] = 1
    end
  end

  def test_map_msg_enum_valuetypes
    m = Google::Protobuf::Map.new(:string, :message, proto_module::TestMessage)
    m["asdf"] = proto_module::TestMessage.new
    assert_raise Google::Protobuf::TypeError do
      m["jkl;"] = proto_module::TestMessage2.new
    end

    m = Google::Protobuf::Map.new(
      :string, :message, proto_module::TestMessage,
      { "a" => proto_module::TestMessage.new(:optional_int32 => 42),
        "b" => proto_module::TestMessage.new(:optional_int32 => 84) })
    assert m.length == 2
    assert m.values.map{|msg| msg.optional_int32}.sort == [42, 84]

    m = Google::Protobuf::Map.new(:string, :enum, proto_module::TestEnum,
                                  { "x" => :A, "y" => :B, "z" => :C })
    assert m.length == 3
    assert m["z"] == :C
    m["z"] = 2
    assert m["z"] == :B
    m["z"] = 4
    assert m["z"] == 4
    assert_raise RangeError do
      m["z"] = :Z
    end
    assert_raise RangeError do
      m["z"] = "z"
    end
  end

  def test_map_dup_deep_copy
    m = Google::Protobuf::Map.new(
      :string, :message, proto_module::TestMessage,
      { "a" => proto_module::TestMessage.new(:optional_int32 => 42),
        "b" => proto_module::TestMessage.new(:optional_int32 => 84) })

    m2 = m.dup
    assert m == m2
    assert m.object_id != m2.object_id
    assert m["a"].object_id == m2["a"].object_id
    assert m["b"].object_id == m2["b"].object_id

    m2 = Google::Protobuf.deep_copy(m)
    assert m == m2
    assert m.object_id != m2.object_id
    assert m["a"].object_id != m2["a"].object_id
    assert m["b"].object_id != m2["b"].object_id
  end

  def test_oneof_descriptors
    d = proto_module::OneofMessage.descriptor
    o = d.lookup_oneof("my_oneof")
    assert o != nil
    assert o.class == Google::Protobuf::OneofDescriptor
    assert o.name == "my_oneof"
    oneof_count = 0
    d.each_oneof{ |oneof|
      oneof_count += 1
      assert oneof == o
    }
    assert oneof_count == 1
    assert o.count == 4
    field_names = o.map{|f| f.name}.sort
    assert field_names == ["a", "b", "c", "d"]
  end

  def test_oneof
    d = proto_module::OneofMessage.new
    assert d.a == ""
    assert d.b == 0
    assert d.c == nil
    assert d.d == :Default
    assert d.my_oneof == nil

    d.a = "hi"
    assert d.a == "hi"
    assert d.b == 0
    assert d.c == nil
    assert d.d == :Default
    assert d.my_oneof == :a

    d.b = 42
    assert d.a == ""
    assert d.b == 42
    assert d.c == nil
    assert d.d == :Default
    assert d.my_oneof == :b

    d.c = proto_module::TestMessage2.new(:foo => 100)
    assert d.a == ""
    assert d.b == 0
    assert d.c.foo == 100
    assert d.d == :Default
    assert d.my_oneof == :c

    d.d = :C
    assert d.a == ""
    assert d.b == 0
    assert d.c == nil
    assert d.d == :C
    assert d.my_oneof == :d

    d2 = proto_module::OneofMessage.decode(proto_module::OneofMessage.encode(d))
    assert d2 == d

    encoded_field_a = proto_module::OneofMessage.encode(proto_module::OneofMessage.new(:a => "string"))
    encoded_field_b = proto_module::OneofMessage.encode(proto_module::OneofMessage.new(:b => 1000))
    encoded_field_c = proto_module::OneofMessage.encode(
      proto_module::OneofMessage.new(:c => proto_module::TestMessage2.new(:foo => 1)))
    encoded_field_d = proto_module::OneofMessage.encode(proto_module::OneofMessage.new(:d => :B))

    d3 = proto_module::OneofMessage.decode(
      encoded_field_c + encoded_field_a + encoded_field_b + encoded_field_d)
    assert d3.a == ""
    assert d3.b == 0
    assert d3.c == nil
    assert d3.d == :B

    d4 = proto_module::OneofMessage.decode(
      encoded_field_c + encoded_field_a + encoded_field_b + encoded_field_d +
      encoded_field_c)
    assert d4.a == ""
    assert d4.b == 0
    assert d4.c.foo == 1
    assert d4.d == :Default

    d5 = proto_module::OneofMessage.new(:a => "hello")
    assert d5.a == "hello"
    d5.a = nil
    assert d5.a == ""
    assert proto_module::OneofMessage.encode(d5) == ''
    assert d5.my_oneof == nil
  end

  def test_enum_field
    m = proto_module::TestMessage.new
    assert m.optional_enum == :Default
    m.optional_enum = :A
    assert m.optional_enum == :A
    assert_raise RangeError do
      m.optional_enum = :ASDF
    end
    m.optional_enum = 1
    assert m.optional_enum == :A
    m.optional_enum = 100
    assert m.optional_enum == 100
  end

  def test_dup
    m = proto_module::TestMessage.new
    m.optional_string = "hello"
    m.optional_int32 = 42
    tm1 = proto_module::TestMessage2.new(:foo => 100)
    tm2 = proto_module::TestMessage2.new(:foo => 200)
    m.repeated_msg.push tm1
    assert m.repeated_msg[-1] == tm1
    m.repeated_msg.push tm2
    assert m.repeated_msg[-1] == tm2
    m2 = m.dup
    assert m == m2
    m.optional_int32 += 1
    assert m != m2
    assert m.repeated_msg[0] == m2.repeated_msg[0]
    assert m.repeated_msg[0].object_id == m2.repeated_msg[0].object_id
  end

  def test_deep_copy
    m = proto_module::TestMessage.new(:optional_int32 => 42,
                                      :repeated_msg => [proto_module::TestMessage2.new(:foo => 100)])
    m2 = Google::Protobuf.deep_copy(m)
    assert m == m2
    assert m.repeated_msg == m2.repeated_msg
    assert m.repeated_msg.object_id != m2.repeated_msg.object_id
    assert m.repeated_msg[0].object_id != m2.repeated_msg[0].object_id
  end

  def test_eq
    m = proto_module::TestMessage.new(:optional_int32 => 42,
                                      :repeated_int32 => [1, 2, 3])
    m2 = proto_module::TestMessage.new(:optional_int32 => 43,
                                       :repeated_int32 => [1, 2, 3])
    assert m != m2
  end

  def test_enum_lookup
    assert proto_module::TestEnum::A == 1
    assert proto_module::TestEnum::B == 2
    assert proto_module::TestEnum::C == 3

    assert proto_module::TestEnum::lookup(1) == :A
    assert proto_module::TestEnum::lookup(2) == :B
    assert proto_module::TestEnum::lookup(3) == :C

    assert proto_module::TestEnum::resolve(:A) == 1
    assert proto_module::TestEnum::resolve(:B) == 2
    assert proto_module::TestEnum::resolve(:C) == 3
  end

  def test_parse_serialize
    m = proto_module::TestMessage.new(:optional_int32 => 42,
                                      :optional_string => "hello world",
                                      :optional_enum => :B,
                                      :repeated_string => ["a", "b", "c"],
                                      :repeated_int32 => [42, 43, 44],
                                      :repeated_enum => [:A, :B, :C, 100],
                                      :repeated_msg => [proto_module::TestMessage2.new(:foo => 1),
                                                        proto_module::TestMessage2.new(:foo => 2)])
    data = proto_module::TestMessage.encode m
    m2 = proto_module::TestMessage.decode data
    assert m == m2

    data = Google::Protobuf.encode m
    m2 = Google::Protobuf.decode(proto_module::TestMessage, data)
    assert m == m2
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
  end

  def test_def_errors
    s = Google::Protobuf::DescriptorPool.new
    assert_raise Google::Protobuf::TypeError do
      s.build do
        # enum with no default (integer value 0)
        add_enum "MyEnum" do
          value :A, 1
        end
      end
    end
    assert_raise Google::Protobuf::TypeError do
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
    assert proto_module::Recursive1.descriptor.lookup("foo").subtype ==
           proto_module::Recursive2.descriptor
    assert proto_module::Recursive2.descriptor.lookup("foo").subtype ==
           proto_module::Recursive1.descriptor

    serialized = proto_module::Recursive1.encode(m)
    m2 = proto_module::Recursive1.decode(serialized)
    assert m == m2
  end

  def test_serialize_cycle
    m = proto_module::Recursive1.new(:foo => proto_module::Recursive2.new)
    m.foo.foo = m
    assert_raise RuntimeError do
      proto_module::Recursive1.encode(m)
    end
  end

  def test_bad_field_names
    m = proto_module::BadFieldNames.new(:dup => 1, :class => 2)
    m2 = m.dup
    assert m == m2
    assert m['dup'] == 1
    assert m['class'] == 2
    m['dup'] = 3
    assert m['dup'] == 3
    m['a.b'] = 4
    assert m['a.b'] == 4
  end

  def test_int_ranges
    m = proto_module::TestMessage.new

    m.optional_int32 = 0
    m.optional_int32 = -0x8000_0000
    m.optional_int32 = +0x7fff_ffff
    m.optional_int32 = 1.0
    m.optional_int32 = -1.0
    m.optional_int32 = 2e9
    assert_raise RangeError do
      m.optional_int32 = -0x8000_0001
    end
    assert_raise RangeError do
      m.optional_int32 = +0x8000_0000
    end
    assert_raise RangeError do
      m.optional_int32 = +0x1000_0000_0000_0000_0000_0000 # force Bignum
    end
    assert_raise RangeError do
      m.optional_int32 = 1e12
    end
    assert_raise RangeError do
      m.optional_int32 = 1.5
    end

    m.optional_uint32 = 0
    m.optional_uint32 = +0xffff_ffff
    m.optional_uint32 = 1.0
    m.optional_uint32 = 4e9
    assert_raise RangeError do
      m.optional_uint32 = -1
    end
    assert_raise RangeError do
      m.optional_uint32 = -1.5
    end
    assert_raise RangeError do
      m.optional_uint32 = -1.5e12
    end
    assert_raise RangeError do
      m.optional_uint32 = -0x1000_0000_0000_0000
    end
    assert_raise RangeError do
      m.optional_uint32 = +0x1_0000_0000
    end
    assert_raise RangeError do
      m.optional_uint32 = +0x1000_0000_0000_0000_0000_0000 # force Bignum
    end
    assert_raise RangeError do
      m.optional_uint32 = 1e12
    end
    assert_raise RangeError do
      m.optional_uint32 = 1.5
    end

    m.optional_int64 = 0
    m.optional_int64 = -0x8000_0000_0000_0000
    m.optional_int64 = +0x7fff_ffff_ffff_ffff
    m.optional_int64 = 1.0
    m.optional_int64 = -1.0
    m.optional_int64 = 8e18
    m.optional_int64 = -8e18
    assert_raise RangeError do
      m.optional_int64 = -0x8000_0000_0000_0001
    end
    assert_raise RangeError do
      m.optional_int64 = +0x8000_0000_0000_0000
    end
    assert_raise RangeError do
      m.optional_int64 = +0x1000_0000_0000_0000_0000_0000 # force Bignum
    end
    assert_raise RangeError do
      m.optional_int64 = 1e50
    end
    assert_raise RangeError do
      m.optional_int64 = 1.5
    end

    m.optional_uint64 = 0
    m.optional_uint64 = +0xffff_ffff_ffff_ffff
    m.optional_uint64 = 1.0
    m.optional_uint64 = 16e18
    assert_raise RangeError do
      m.optional_uint64 = -1
    end
    assert_raise RangeError do
      m.optional_uint64 = -1.5
    end
    assert_raise RangeError do
      m.optional_uint64 = -1.5e12
    end
    assert_raise RangeError do
      m.optional_uint64 = -0x1_0000_0000_0000_0000
    end
    assert_raise RangeError do
      m.optional_uint64 = +0x1_0000_0000_0000_0000
    end
    assert_raise RangeError do
      m.optional_uint64 = +0x1000_0000_0000_0000_0000_0000 # force Bignum
    end
    assert_raise RangeError do
      m.optional_uint64 = 1e50
    end
    assert_raise RangeError do
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
      assert data_new == data
      data = data_new
    end
  end

  def test_reflection
    m = proto_module::TestMessage.new(:optional_int32 => 1234)
    msgdef = m.class.descriptor
    assert msgdef.class == Google::Protobuf::Descriptor
    assert msgdef.any? {|field| field.name == "optional_int32"}
    optional_int32 = msgdef.lookup "optional_int32"
    assert optional_int32.class == Google::Protobuf::FieldDescriptor
    assert optional_int32 != nil
    assert optional_int32.name == "optional_int32"
    assert optional_int32.type == :int32
    optional_int32.set(m, 5678)
    assert m.optional_int32 == 5678
    m.optional_int32 = 1000
    assert optional_int32.get(m) == 1000

    optional_msg = msgdef.lookup "optional_msg"
    assert optional_msg.subtype == proto_module::TestMessage2.descriptor

    optional_msg.set(m, optional_msg.subtype.msgclass.new)

    assert msgdef.msgclass == proto_module::TestMessage

    optional_enum = msgdef.lookup "optional_enum"
    assert optional_enum.subtype == proto_module::TestEnum.descriptor
    assert optional_enum.subtype.class == Google::Protobuf::EnumDescriptor
    optional_enum.subtype.each do |k, v|
      # set with integer, check resolution to symbolic name
      optional_enum.set(m, v)
      assert optional_enum.get(m) == k
    end
  end

  def test_json
    # TODO: Fix JSON in JRuby version.
    return if RUBY_PLATFORM == "java"
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
    puts m.inspect
    puts m2.inspect
    assert m == m2

    # Crash case from GitHub issue 283.
    bar = proto_module::Bar.new(msg: "bar")
    baz1 = proto_module::Baz.new(msg: "baz")
    baz2 = proto_module::Baz.new(msg: "quux")
    proto_module::Foo.encode_json(proto_module::Foo.new)
    proto_module::Foo.encode_json(proto_module::Foo.new(bar: bar))
    proto_module::Foo.encode_json(proto_module::Foo.new(bar: bar, baz: [baz1, baz2]))
  end

  def test_json_empty
    assert proto_module::TestMessage.encode_json(proto_module::TestMessage.new) == '{}'
  end

  def test_json_emit_defaults
    # TODO: Fix JSON in JRuby version.
    return if RUBY_PLATFORM == "java"
    m = proto_module::TestMessage.new

    expected = {
      optionalInt32: 0,
      optionalInt64: 0,
      optionalUint32: 0,
      optionalUint64: 0,
      optionalBool: false,
      optionalFloat: 0,
      optionalDouble: 0,
      optionalString: "",
      optionalBytes: "",
      optionalEnum: "Default",
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

    assert JSON.parse(actual, :symbolize_names => true) == expected
  end

  def test_json_emit_defaults_submsg
    # TODO: Fix JSON in JRuby version.
    return if RUBY_PLATFORM == "java"
    m = proto_module::TestMessage.new(optional_msg: proto_module::TestMessage2.new)

    expected = {
      optionalInt32: 0,
      optionalInt64: 0,
      optionalUint32: 0,
      optionalUint64: 0,
      optionalBool: false,
      optionalFloat: 0,
      optionalDouble: 0,
      optionalString: "",
      optionalBytes: "",
      optionalMsg: {foo: 0},
      optionalEnum: "Default",
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

    assert JSON.parse(actual, :symbolize_names => true) == expected
  end

  def test_json_emit_defaults_repeated_submsg
    # TODO: Fix JSON in JRuby version.
    return if RUBY_PLATFORM == "java"
    m = proto_module::TestMessage.new(repeated_msg: [proto_module::TestMessage2.new])

    expected = {
      optionalInt32: 0,
      optionalInt64: 0,
      optionalUint32: 0,
      optionalUint64: 0,
      optionalBool: false,
      optionalFloat: 0,
      optionalDouble: 0,
      optionalString: "",
      optionalBytes: "",
      optionalEnum: "Default",
      repeatedInt32: [],
      repeatedInt64: [],
      repeatedUint32: [],
      repeatedUint64: [],
      repeatedBool: [],
      repeatedFloat: [],
      repeatedDouble: [],
      repeatedString: [],
      repeatedBytes: [],
      repeatedMsg: [{foo: 0}],
      repeatedEnum: []
    }

    actual = proto_module::TestMessage.encode_json(m, :emit_defaults => true)

    assert JSON.parse(actual, :symbolize_names => true) == expected
  end

  def test_comparison_with_arbitrary_object
    assert proto_module::TestMessage.new != nil
  end
end
