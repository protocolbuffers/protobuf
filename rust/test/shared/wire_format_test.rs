use googletest::prelude::*;

#[test]
fn test_pack_tag() {
    let field_number: i32 = 0xabc;
    let tag_type: i32 = 2;
    let tag: u32 = ((field_number << 3) | tag_type).try_into().unwrap();
    assert_that!(tag, eq(wire_format::pack_tag(field_number, tag_type).unwrap()));

    // Wire type too high
    match wire_format::pack_tag(field_number, 6) {
        Ok(_result) => panic!("Pack tag is supposed to fail with an unknown wire type"),
        Err(error) => assert_eq!(errors::DecodeError { ..error.clone() }, error),
    }

    // Wire type too low
    match wire_format::pack_tag(field_number, -1) {
        Ok(_result) => panic!("Pack tag is supposed to fail with an unknown wire type"),
        Err(error) => assert_eq!(errors::DecodeError { ..error.clone() }, error),
    }
    // Field number too high
    match wire_format::pack_tag(1 << 29, 0) {
        Ok(_result) => {
            panic!("Pack tag is supposed to fail for greater than max field number")
        }
        Err(error) => assert_eq!(errors::DecodeError { ..error.clone() }, error),
    }
    // Field number too low
    match wire_format::pack_tag(0, 0) {
        Ok(_result) => panic!("Pack tag is supposed to fail for less than min field number"),
        Err(error) => assert_eq!(errors::DecodeError { ..error.clone() }, error),
    }
}

#[test]
fn test_unpack_tag() {
    // Test field numbers that will require various varint sizes.

    let expected_field_numbers = vec![1, 15, 16, 2047, 2048];
    for expected_field_number in &expected_field_numbers {
        for expected_wire_type in 1..6 {
            let (field_number, wire_type) = wire_format::unpack_tag(
                wire_format::pack_tag(*expected_field_number, expected_wire_type).unwrap(),
            );
            assert_that!(*expected_field_number, eq(field_number));
            assert_that!(expected_wire_type, eq(wire_type));
        }
    }
}

#[test]
fn test_zig_zag_encode() {
    let z = wire_format::zig_zag_encode;
    assert_that!(0, eq(z(0)));
    assert_that!(1, eq(z(-1)));
    assert_that!(2, eq(z(1)));
    assert_that!(3, eq(z(-2)));
    assert_that!(4, eq(z(2)));
    assert_that!(0xfffffffe, eq(z(0x7fffffff)));
    assert_that!(0xffffffff, eq(z(-0x80000000)));
    assert_that!(0xfffffffffffffffe, eq(z(0x7fffffffffffffff)));
    assert_that!(0xffffffffffffffff, eq(z(-0x8000000000000000)))
}

#[test]
fn test_zig_zag_decode() {
    let z = wire_format::zig_zag_decode;
    assert_that!(0, eq(z(0)));
    assert_that!(-1, eq(z(1)));
    assert_that!(1, eq(z(2)));
    assert_that!(-2, eq(z(3)));
    assert_that!(2, eq(z(4)));
    assert_that!(0x7fffffff, eq(z(0xfffffffe)));
    assert_that!(-0x80000000, eq(z(0xffffffff)));
    assert_that!(0x7fffffffffffffff, eq(z(0xfffffffffffffffe)));
    assert_that!(-0x8000000000000000, eq(z(0xffffffffffffffff)))
}

enum Functions {
    Int32,
    Int64,
    UInt32,
    UInt64,
    SInt32,
    SInt64,
    Fixed32,
    Fixed64,
    SFixed32,
    SFixed64,
    Float,
    Double,
    Bool,
    Enum,
}

struct Value {
    floating: f64,
    integer: i128,
}

impl Functions {
    fn call(&self, field_number: i32, value: &Value) -> u32 {
        match self {
            Functions::Int32 => wire_format::int32_byte_size(field_number, value.integer as i32),
            Functions::Int64 => wire_format::int64_byte_size(field_number, value.integer as i64),
            Functions::UInt32 => wire_format::uint32_byte_size(field_number, value.integer as u32),
            Functions::UInt64 => wire_format::uint64_byte_size(field_number, value.integer as u64),
            Functions::SInt32 => wire_format::sint32_byte_size(field_number, value.integer as i32),
            Functions::SInt64 => wire_format::sint64_byte_size(field_number, value.integer as i64),
            Functions::Fixed32 => {
                wire_format::fixed32_byte_size(field_number, value.integer as u32)
            }
            Functions::Fixed64 => {
                wire_format::fixed64_byte_size(field_number, value.integer as u64)
            }
            Functions::SFixed32 => {
                wire_format::sfixed32_byte_size(field_number, value.integer as i32)
            }
            Functions::SFixed64 => {
                wire_format::sfixed64_byte_size(field_number, value.integer as i64)
            }
            Functions::Float => wire_format::float_byte_size(field_number, value.floating as f32),
            Functions::Double => wire_format::double_byte_size(field_number, value.floating),
            Functions::Bool => {
                if value.integer == 1 {
                    wire_format::bool_byte_size(field_number, true)
                } else {
                    wire_format::bool_byte_size(field_number, false)
                }
            }
            Functions::Enum => wire_format::enum_byte_size(field_number, value.integer as u32),
        }
    }
}

#[test]
fn test_byte_size_functions() {
    // Test all numeric *_byte_size() functions.
    let numeric_args: Vec<(Functions, Value, u32)> = vec![
        // int32_byte_size()
        (Functions::Int32, Value { integer: 0, floating: 0.0 }, 1),
        (Functions::Int32, Value { integer: 127, floating: 0.0 }, 1),
        (Functions::Int32, Value { integer: 128, floating: 0.0 }, 2),
        (Functions::Int32, Value { integer: -1, floating: 0.0 }, 10),
        // int64_byte_size()
        (Functions::Int64, Value { integer: 0, floating: 0.0 }, 1),
        (Functions::Int64, Value { integer: 127, floating: 0.0 }, 1),
        (Functions::Int64, Value { integer: 128, floating: 0.0 }, 2),
        (Functions::Int64, Value { integer: -1, floating: 0.0 }, 10),
        // uint32_byte_size()
        (Functions::UInt32, Value { integer: 0, floating: 0.0 }, 1),
        (Functions::UInt32, Value { integer: 127, floating: 0.0 }, 1),
        (Functions::UInt32, Value { integer: 128, floating: 0.0 }, 2),
        (Functions::UInt32, Value { integer: u32::MAX as i128, floating: 0.0 }, 5),
        // uint64_byte_size()
        (Functions::UInt64, Value { integer: 0, floating: 0.0 }, 1),
        (Functions::UInt64, Value { integer: 127, floating: 0.0 }, 1),
        (Functions::UInt64, Value { integer: 128, floating: 0.0 }, 2),
        (Functions::UInt64, Value { integer: u64::MAX as i128, floating: 0.0 }, 10),
        // sint32_byte_size()
        (Functions::SInt32, Value { integer: 0, floating: 0.0 }, 1),
        (Functions::SInt32, Value { integer: -1, floating: 0.0 }, 1),
        (Functions::SInt32, Value { integer: 1, floating: 0.0 }, 1),
        (Functions::SInt32, Value { integer: -63, floating: 0.0 }, 1),
        (Functions::SInt32, Value { integer: 63, floating: 0.0 }, 1),
        (Functions::SInt32, Value { integer: -64, floating: 0.0 }, 1),
        (Functions::SInt32, Value { integer: 64, floating: 0.0 }, 2),
        // sint64_byte_size()
        (Functions::SInt64, Value { integer: 0, floating: 0.0 }, 1),
        (Functions::SInt64, Value { integer: -1, floating: 0.0 }, 1),
        (Functions::SInt64, Value { integer: 1, floating: 0.0 }, 1),
        (Functions::SInt64, Value { integer: -63, floating: 0.0 }, 1),
        (Functions::SInt64, Value { integer: 63, floating: 0.0 }, 1),
        (Functions::SInt64, Value { integer: -64, floating: 0.0 }, 1),
        (Functions::SInt64, Value { integer: 64, floating: 0.0 }, 2),
        // fixed32_byte_size().
        (Functions::Fixed32, Value { integer: 0, floating: 0.0 }, 4),
        (Functions::Fixed32, Value { integer: u32::MAX as i128, floating: 0.0 }, 4),
        // fixed64_byte_size().
        (Functions::Fixed64, Value { integer: 0, floating: 0.0 }, 8),
        (Functions::Fixed64, Value { integer: u64::MAX as i128, floating: 0.0 }, 8),
        // sfixed32_byte_size().
        (Functions::SFixed32, Value { integer: 0, floating: 0.0 }, 4),
        (Functions::SFixed32, Value { integer: i32::MIN as i128, floating: 0.0 }, 4),
        (Functions::SFixed32, Value { integer: i32::MAX as i128, floating: 0.0 }, 4),
        // sfixed64_byte_size().
        (Functions::SFixed64, Value { integer: 0, floating: 0.0 }, 8),
        (Functions::SFixed64, Value { integer: i64::MIN as i128, floating: 0.0 }, 8),
        (Functions::SFixed64, Value { integer: i64::MAX as i128, floating: 0.0 }, 8),
        // float_byte_size().
        (Functions::Float, Value { integer: 0, floating: 0.0 }, 4),
        (Functions::Float, Value { integer: 0, floating: 1000000000.0 }, 4),
        (Functions::Float, Value { integer: 0, floating: -1000000000.0 }, 4),
        // double_byte_size().
        (Functions::Double, Value { integer: 0, floating: 0.0 }, 8),
        (Functions::Double, Value { integer: 0, floating: 1000000000.0 }, 8),
        (Functions::Double, Value { integer: 0, floating: -1000000000.0 }, 8),
        // bool_byte_size().
        (Functions::Bool, Value { integer: 0, floating: 0.0 }, 1),
        (Functions::Bool, Value { integer: 1, floating: 0.0 }, 1),
        // enum_byte_size().
        (Functions::Enum, Value { integer: 0, floating: 0.0 }, 1),
        (Functions::Enum, Value { integer: 127, floating: 0.0 }, 1),
        (Functions::Enum, Value { integer: 128, floating: 0.0 }, 2),
        (Functions::Enum, Value { integer: u32::MAX as i128, floating: 0.0 }, 5),
    ];

    for args in numeric_args {
        let byte_size_fn = args.0;
        let value = args.1;
        let expected_value_size = args.2;

        // Use field numbers that cause various byte sizes for the tag information.
        let field_tag: Vec<(i32, u32)> = vec![(15, 1), (16, 2), (2047, 2), (2048, 3)];

        for arg in field_tag {
            let field_number = arg.0;
            let tag_bytes = arg.1;
            let expected_size = expected_value_size + tag_bytes;
            let actual_size = byte_size_fn.call(field_number, &value);
            assert_that!(expected_size, eq(actual_size));
        }
    }

    // 1 byte for tag, 1 byte for length, 3 bytes for contents.
    assert_that!(5, eq(wire_format::string_byte_size(10, "abc")));
    assert_that!(5, eq(wire_format::bytes_byte_size(10, "abc")));
    // 2 bytes for tag, 1 byte for length, 3 bytes for contents.
    assert_that!(6, eq(wire_format::string_byte_size(16, "abc")));
    assert_that!(6, eq(wire_format::bytes_byte_size(16, "abc")));
    // 2 bytes for tag, 2 bytes for length, 128 bytes for contents.
    assert_that!(132, eq(wire_format::string_byte_size(16, &"a".repeat(128))));
    assert_that!(132, eq(wire_format::bytes_byte_size(16, &"a".repeat(128))));

    // Test UTF-8 string byte size calculation.
    // 1 byte for tag, 1 byte for length, 8 bytes for content.
    let utf8_bytes = [0xd0, 0xa2, 0xd0, 0xb5, 0xd1, 0x81, 0xd1, 0x82];
    assert_that!(
        10,
        eq(wire_format::string_byte_size(5, std::str::from_utf8(&utf8_bytes).unwrap()))
    );

    let mut mock_message = wire_format::MockMessage { byte_size: 10 };
    let message_byte_size: u32 = 10;

    // Test groups.
    // (2 * 1) bytes for begin and end tags, plus message_byte_size.
    assert_that!(2 + message_byte_size, eq(wire_format::group_byte_size(1, &mock_message)));
    // (2 * 2) bytes for begin and end tags, plus message_byte_size.
    assert_that!(4 + message_byte_size, eq(wire_format::group_byte_size(16, &mock_message)));

    // Test messages.
    // 1 byte for tag, plus 1 byte for length, plus contents.
    assert_that!(2 + mock_message.byte_size, eq(wire_format::message_byte_size(1, &mock_message)));
    // 2 bytes for tag, plus 1 byte for length, plus contents.
    assert_that!(3 + mock_message.byte_size, eq(wire_format::message_byte_size(16, &mock_message)));
    // 2 bytes for tag, plus 2 bytes for length, plus contents.
    mock_message.byte_size = 128;
    assert_that!(4 + mock_message.byte_size, eq(wire_format::message_byte_size(16, &mock_message)));

    // Test message set item byte size.
    // 4 bytes for tags, plus 1 byte for length, plus 1 byte for type_id,
    // plus contents.
    mock_message.byte_size = 10;
    assert_that!(
        mock_message.byte_size + 6,
        eq(wire_format::message_set_item_byte_size(1, &mock_message))
    );

    // 4 bytes for tags, plus 2 bytes for length, plus 1 byte for type_id,
    // plus contents.
    mock_message.byte_size = 128;
    assert_that!(
        mock_message.byte_size + 7,
        eq(wire_format::message_set_item_byte_size(1, &mock_message))
    );

    // 4 bytes for tags, plus 2 bytes for length, plus 2 byte for type_id,
    // plus contents.
    assert_that!(
        mock_message.byte_size + 8,
        eq(wire_format::message_set_item_byte_size(128, &mock_message))
    );
}
