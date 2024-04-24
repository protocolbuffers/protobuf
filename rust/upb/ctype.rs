// Transcribed from google3/third_party/upb/upb/base/descriptor_constants.h
#[repr(C)]
#[allow(dead_code)]
#[derive(PartialEq, Eq, Clone, Copy, Debug)]
pub enum CType {
    Bool = 1,
    Float = 2,
    Int32 = 3,
    UInt32 = 4,
    Enum = 5,
    Message = 6,
    Double = 7,
    Int64 = 8,
    UInt64 = 9,
    String = 10,
    Bytes = 11,
}
