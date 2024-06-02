const TAG_TYPE_BITS: u32 = 3; // Number of bits used to hold type info in a proto tag.
const TAG_TYPE_MASK: u32 = (1 << TAG_TYPE_BITS) - 1; // 0x7

// These numbers identify the wire type of a protocol buffer value.
//  We use the least-significant TAG_TYPE_BITS bits of the varint-encoded
// tag-and-type to store one of these WIRETYPE_* constants.
// These values must match WireType enum in
// //third_party/protobuf/wire_format.h.
const _WIRETYPE_VARINT: i64 = 0;
const _WIRETYPE_FIXED64: i64 = 1;
const _WIRETYPE_LENGTH_DELIMITED: i64 = 2;
const _WIRETYPE_START_GROUP: i64 = 3;
const _WIRETYPE_END_GROUP: i64 = 4;
const _WIRETYPE_FIXED32: i64 = 5;
const WIRETYPE_MAX: u32 = 5;

// Bounds for various integer types.
pub const INT32_MAX: i64 = (1 << 31) - 1;
pub const INT32_MIN: i64 = -(1 << 31);
pub const UINT32_MAX: u64 = (1 << 32) - 1;

pub const INT64_MAX: i128 = (1 << 63) - 1;
pub const INT64_MIN: i128 = -(1 << 63);
pub const UINT64_MAX: u128 = (1 << 64) - 1;

pub struct MockMessage {
    pub byte_size: u32,
}

pub fn pack_tag(field_number: i32, wire_type: i32) -> Result<u32, errors::DecodeError> {
    //   Returns an unsigned 32-bit integer that encodes the field number and
    //   wire type information in standard protocol message wire format.

    //   Args:
    //     field_number: Expected to be an integer in the range [1, 1 << 29)
    //     wire_type: One of the WIRETYPE_* constants.

    if field_number <= 0 || field_number >= 1 << 29 {
        return Err(errors::DecodeError::new("Not in range for valid field number"));
    }

    if wire_type < 0 || wire_type > WIRETYPE_MAX as i32 {
        Err(errors::DecodeError::new("Unknown wire type"))
    } else {
        // Conversion should have no problem, as this can only pass with positive
        // integers
        Ok(((field_number << TAG_TYPE_BITS) | wire_type) as u32)
    }
}

pub fn unpack_tag(tag: u32) -> (i32, i32) {
    // The inverse of pack_tag().  Given an unsigned 32-bit number,
    //   returns a (field_number, wire_type) tuple.
    ((tag >> TAG_TYPE_BITS) as i32, (tag & TAG_TYPE_MASK) as i32)
}

pub fn zig_zag_encode(value: i64) -> u64 {
    //   ZigZag Transform:  Encodes signed integers so that they can be
    //   effectively used with varint encoding.  See wire_format.h for
    //   more details.
    if value >= 0 { (value << 1) as u64 } else { ((value << 1) ^ (!0)) as u64 }
}

pub fn zig_zag_decode(value: u64) -> i64 {
    // Inverse of zig_zag_encode().
    let odd = value & 0x1;
    if odd == 0 { (value >> 1) as i64 } else { ((value >> 1) ^ (!0)) as i64 }
}

pub fn int32_byte_size(field_number: i32, int32: i32) -> u32 {
    int64_byte_size(field_number, int32.into())
}

pub fn int32_byte_size_no_tag(int32: i32) -> u32 {
    var_uint64_byte_size_no_tag(int32 as u64)
}

pub fn int64_byte_size(field_number: i32, int64: i64) -> u32 {
    // Have to convert to uint before calling uint64_byte_size.
    uint64_byte_size(field_number, int64 as u64)
}

pub fn uint32_byte_size(field_number: i32, uint32: u32) -> u32 {
    uint64_byte_size(field_number, uint32.into())
}

pub fn uint64_byte_size(field_number: i32, uint64: u64) -> u32 {
    tag_byte_size(field_number) + var_uint64_byte_size_no_tag(uint64)
}

pub fn sint32_byte_size(field_number: i32, int32: i32) -> u32 {
    uint32_byte_size(field_number, zig_zag_encode(int32.into()) as u32)
}

pub fn sint64_byte_size(field_number: i32, int64: i64) -> u32 {
    uint64_byte_size(field_number, zig_zag_encode(int64))
}

pub fn fixed32_byte_size(field_number: i32, _fixed32: u32) -> u32 {
    tag_byte_size(field_number) + 4
}

pub fn fixed64_byte_size(field_number: i32, _fixed64: u64) -> u32 {
    tag_byte_size(field_number) + 8
}

pub fn sfixed32_byte_size(field_number: i32, _sfixed32: i32) -> u32 {
    tag_byte_size(field_number) + 4
}

pub fn sfixed64_byte_size(field_number: i32, _sfixed64: i64) -> u32 {
    tag_byte_size(field_number) + 8
}

pub fn float_byte_size(field_number: i32, _flt: f32) -> u32 {
    tag_byte_size(field_number) + 4
}

pub fn double_byte_size(field_number: i32, _dbl: f64) -> u32 {
    tag_byte_size(field_number) + 8
}

pub fn bool_byte_size(field_number: i32, _b: bool) -> u32 {
    tag_byte_size(field_number) + 1
}

pub fn enum_byte_size(field_number: i32, e: u32) -> u32 {
    uint32_byte_size(field_number, e)
}

pub fn string_byte_size(field_number: i32, s: &str) -> u32 {
    bytes_byte_size(field_number, s)
}

pub fn bytes_byte_size(field_number: i32, b: &str) -> u32 {
    tag_byte_size(field_number)
        + var_uint64_byte_size_no_tag(b.to_string().len() as u64)
        + b.to_string().len() as u32
}

pub fn group_byte_size(field_number: i32, message: &MockMessage) -> u32 {
    2 * tag_byte_size(field_number) + message.byte_size
}

pub fn message_byte_size(field_number: i32, message: &MockMessage) -> u32 {
    tag_byte_size(field_number)
        + var_uint64_byte_size_no_tag(message.byte_size.into())
        + message.byte_size
}

pub fn message_set_item_byte_size(field_number: i32, message: &MockMessage) -> u32 {
    //   First compute the sizes of the tags.
    //   There are 2 tags for the beginning and ending of the repeated group, that
    //   is field number 1, one with field number 2 (type_id) and one with field
    //   number 3 (message).
    let mut total_size = 2 * tag_byte_size(1) + tag_byte_size(2) + tag_byte_size(3);

    // Add the number of bytes for type_id.
    total_size += var_uint64_byte_size_no_tag(field_number as u64);

    let message_size = message.byte_size;

    // The number of bytes for encoding the length of the message.
    total_size += var_uint64_byte_size_no_tag(message_size.into());

    // The size of the message.
    total_size += message_size;
    total_size
}

pub fn tag_byte_size(field_number: i32) -> u32 {
    // Returns the bytes required to serialize a tag with this field number.
    // Just pass in type 0, since the type won't affect the tag+type size.
    var_uint64_byte_size_no_tag(pack_tag(field_number, 0).unwrap().into())
}

// Private helper function for the *_byte_size() functions above.

pub fn var_uint64_byte_size_no_tag(uint64: u64) -> u32 {
    //   Returns the number of bytes required to serialize a single varint
    //   using boundary value comparisons. (unrolled loop optimization -WPierce)
    //   uint64 must be unsigned.
    if uint64 <= 0x7f {
        return 1;
    }
    if uint64 <= 0x3fff {
        return 2;
    }
    if uint64 <= 0x1fffff {
        return 3;
    }
    if uint64 <= 0xfffffff {
        return 4;
    }
    if uint64 <= 0x7ffffffff {
        return 5;
    }
    if uint64 <= 0x3ffffffffff {
        return 6;
    }
    if uint64 <= 0x1ffffffffffff {
        return 7;
    }
    if uint64 <= 0xffffffffffffff {
        return 8;
    }
    if uint64 <= 0x7fffffffffffffff {
        return 9;
    }
    // No need to verify if 'uint64' is greater than UINT64_MAX as this function
    // cannot accept anything bigger
    10
}
