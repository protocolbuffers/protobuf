class TestMsg
  pool = Google::Protobuf::DescriptorPool.new
  pool.build do
    add_message "Message" do
      optional :optional_int32,  :int32,        1
      optional :optional_int64,  :int64,        2
      optional :optional_uint32, :uint32,       3
      optional :optional_uint64, :uint64,       4
      optional :optional_bool,   :bool,         5
      optional :optional_float,  :float,        6
      optional :optional_double, :double,       7
      optional :optional_string, :string,       8
      optional :optional_bytes,  :bytes,        9
      optional :optional_msg,    :message,      10, "Message2"
      optional :optional_enum,   :enum,         11, "Enum"

      repeated :repeated_int32,  :int32,        12
      repeated :repeated_int64,  :int64,        13
      repeated :repeated_uint32, :uint32,       14
      repeated :repeated_uint64, :uint64,       15
      repeated :repeated_bool,   :bool,         16
      repeated :repeated_float,  :float,        17
      repeated :repeated_double, :double,       18
      repeated :repeated_string, :string,       19
      repeated :repeated_bytes,  :bytes,        20
      repeated :repeated_msg,    :message,      21, "Message2"
      repeated :repeated_enum,   :enum,         22, "Enum"

      map      :map_string_int32, :string, :int32,   23
      map      :map_string_msg,   :string, :message, 24, "Message2"
    end

    add_message "Message2" do
      optional :foo, :int32, 1
    end

    add_enum "Enum" do
      value :Default, 0
      value :A, 1
      value :B, 2
      value :C, 3
    end
  end

  Message = pool.lookup("Message").msgclass
  Message2 = pool.lookup("Message2").msgclass
  Enum = pool.lookup("Enum").enummodule
end
