use std::ptr::NonNull;
use std::slice;

mod opaque_pointee;
use opaque_pointee::opaque_pointee;

mod arena;
pub use arena::{upb_Arena, Arena, RawArena};

opaque_pointee!(upb_Message);
pub type RawMessage = NonNull<upb_Message>;

opaque_pointee!(upb_MiniTable);
pub type RawMiniTable = NonNull<upb_MiniTable>;

opaque_pointee!(upb_ExtensionRegistry);
pub type RawExtensionRegistry = NonNull<upb_ExtensionRegistry>;

opaque_pointee!(upb_Map);
pub type RawMap = NonNull<upb_Map>;

opaque_pointee!(upb_Array);
pub type RawArray = NonNull<upb_Array>;

extern "C" {
    pub fn upb_Message_DeepCopy(
        dst: RawMessage,
        src: RawMessage,
        mini_table: *const upb_MiniTable,
        arena: RawArena,
    );

    pub fn upb_Message_DeepClone(
        m: RawMessage,
        mini_table: *const upb_MiniTable,
        arena: RawArena,
    ) -> Option<RawMessage>;
}

// LINT.IfChange(encode_status)
#[repr(C)]
#[derive(PartialEq, Eq, Copy, Clone)]
pub enum EncodeStatus {
    Ok = 0,
    OutOfMemory = 1,
    MaxDepthExceeded = 2,
    MissingRequired = 3,
}
// LINT.ThenChange()

// LINT.IfChange(decode_status)
#[repr(C)]
#[derive(PartialEq, Eq, Copy, Clone)]
pub enum DecodeStatus {
    Ok = 0,
    Malformed = 1,
    OutOfMemory = 2,
    BadUtf8 = 3,
    MaxDepthExceeded = 4,
    MissingRequired = 5,
    UnlinkedSubMessage = 6,
}
// LINT.ThenChange()

extern "C" {
    pub fn upb_Encode(
        msg: RawMessage,
        mini_table: *const upb_MiniTable,
        options: i32,
        arena: RawArena,
        buf: *mut *mut u8,
        buf_size: *mut usize,
    ) -> EncodeStatus;

    pub fn upb_Decode(
        buf: *const u8,
        buf_size: usize,
        msg: RawMessage,
        mini_table: *const upb_MiniTable,
        extreg: *const upb_ExtensionRegistry,
        options: i32,
        arena: RawArena,
    ) -> DecodeStatus;
}

/// ABI compatible struct with upb_StringView.
///
/// Note that this has semantics similar to `std::string_view` in C++ and
/// `&[u8]` in Rust, but is not ABI-compatible with either.
///
/// If `len` is 0, then `ptr` is allowed to be either null or dangling. C++
/// considers a dangling 0-len `std::string_view` to be invalid, and Rust
/// considers a `&[u8]` with a null data pointer to be invalid.
#[repr(C)]
#[derive(Copy, Clone)]
pub struct StringView {
    /// Pointer to the first byte.
    /// Borrows the memory.
    pub ptr: *const u8,

    /// Length of the `[u8]` pointed to by `ptr`.
    pub len: usize,
}

impl StringView {
    /// Unsafely dereference this slice.
    ///
    /// # Safety
    /// - `self.ptr` must be dereferencable and immutable for `self.len` bytes
    ///   for the lifetime `'a`. It can be null or dangling if `self.len == 0`.
    pub unsafe fn as_ref<'a>(self) -> &'a [u8] {
        if self.ptr.is_null() {
            assert_eq!(self.len, 0, "Non-empty slice with null data pointer");
            &[]
        } else {
            // SAFETY:
            // - `ptr` is non-null
            // - `ptr` is valid for `len` bytes as promised by the caller.
            unsafe { slice::from_raw_parts(self.ptr, self.len) }
        }
    }
}

impl From<&[u8]> for StringView {
    fn from(slice: &[u8]) -> Self {
        Self { ptr: slice.as_ptr(), len: slice.len() }
    }
}

// Transcribed from google3/third_party/upb/upb/message/value.h
#[repr(C)]
#[derive(Clone, Copy)]
pub union upb_MessageValue {
    pub bool_val: bool,
    pub float_val: std::ffi::c_float,
    pub double_val: std::ffi::c_double,
    pub uint32_val: u32,
    pub int32_val: i32,
    pub uint64_val: u64,
    pub int64_val: i64,
    // TODO: Replace this `RawMessage` with the const type.
    pub array_val: Option<RawArray>,
    pub map_val: Option<RawMap>,
    pub msg_val: Option<RawMessage>,
    pub str_val: StringView,

    tagged_msg_val: *const std::ffi::c_void,
}

impl upb_MessageValue {
    pub fn zeroed() -> Self {
        // SAFETY: zero bytes is a valid representation for at least one value in the
        // union (actually valid for all of them).
        unsafe { std::mem::zeroed() }
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub union upb_MutableMessageValue {
    pub array: Option<RawArray>,
    pub map: Option<RawMap>,
    pub msg: Option<RawMessage>,
}

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

extern "C" {
    pub fn upb_Array_New(a: RawArena, r#type: CType) -> RawArray;
    pub fn upb_Array_Size(arr: RawArray) -> usize;
    pub fn upb_Array_Set(arr: RawArray, i: usize, val: upb_MessageValue);
    pub fn upb_Array_Get(arr: RawArray, i: usize) -> upb_MessageValue;
    pub fn upb_Array_Append(arr: RawArray, val: upb_MessageValue, arena: RawArena) -> bool;
    pub fn upb_Array_Resize(arr: RawArray, size: usize, arena: RawArena) -> bool;
    pub fn upb_Array_MutableDataPtr(arr: RawArray) -> *mut std::ffi::c_void;
    pub fn upb_Array_DataPtr(arr: RawArray) -> *const std::ffi::c_void;
    pub fn upb_Array_GetMutable(arr: RawArray, i: usize) -> upb_MutableMessageValue;
}

#[repr(C)]
#[allow(dead_code)]
#[derive(PartialEq, Eq, Clone, Copy, Debug)]
pub enum MapInsertStatus {
    Inserted = 0,
    Replaced = 1,
    OutOfMemory = 2,
}

extern "C" {
    pub fn upb_Map_New(arena: RawArena, key_type: CType, value_type: CType) -> RawMap;
    pub fn upb_Map_Size(map: RawMap) -> usize;
    pub fn upb_Map_Insert(
        map: RawMap,
        key: upb_MessageValue,
        value: upb_MessageValue,
        arena: RawArena,
    ) -> MapInsertStatus;
    pub fn upb_Map_Get(map: RawMap, key: upb_MessageValue, value: *mut upb_MessageValue) -> bool;
    pub fn upb_Map_Delete(
        map: RawMap,
        key: upb_MessageValue,
        removed_value: *mut upb_MessageValue,
    ) -> bool;
    pub fn upb_Map_Clear(map: RawMap);

    pub static __rust_proto_kUpb_Map_Begin: usize;

    pub fn upb_Map_Next(
        map: RawMap,
        key: *mut upb_MessageValue,
        value: *mut upb_MessageValue,
        iter: &mut usize,
    ) -> bool;
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn array_ffi_test() {
        // SAFETY: FFI unit test uses C API under expected patterns.
        unsafe {
            let arena = Arena::new();
            let raw_arena = arena.raw();
            let array = upb_Array_New(raw_arena, CType::Float);

            assert!(upb_Array_Append(array, upb_MessageValue { float_val: 7.0 }, raw_arena));
            assert!(upb_Array_Append(array, upb_MessageValue { float_val: 42.0 }, raw_arena));
            assert_eq!(upb_Array_Size(array), 2);
            assert!(matches!(upb_Array_Get(array, 1), upb_MessageValue { float_val: 42.0 }));

            assert!(upb_Array_Resize(array, 3, raw_arena));
            assert_eq!(upb_Array_Size(array), 3);
            assert!(matches!(upb_Array_Get(array, 2), upb_MessageValue { float_val: 0.0 }));
        }
    }

    #[test]
    fn map_ffi_test() {
        // SAFETY: FFI unit test uses C API under expected patterns.
        unsafe {
            let arena = Arena::new();
            let raw_arena = arena.raw();
            let map = upb_Map_New(raw_arena, CType::Bool, CType::Double);
            assert_eq!(upb_Map_Size(map), 0);
            assert_eq!(
                upb_Map_Insert(
                    map,
                    upb_MessageValue { bool_val: true },
                    upb_MessageValue { double_val: 2.0 },
                    raw_arena
                ),
                MapInsertStatus::Inserted
            );
            assert_eq!(
                upb_Map_Insert(
                    map,
                    upb_MessageValue { bool_val: true },
                    upb_MessageValue { double_val: 3.0 },
                    raw_arena,
                ),
                MapInsertStatus::Replaced,
            );
            assert_eq!(upb_Map_Size(map), 1);
            upb_Map_Clear(map);
            assert_eq!(upb_Map_Size(map), 0);
            assert_eq!(
                upb_Map_Insert(
                    map,
                    upb_MessageValue { bool_val: true },
                    upb_MessageValue { double_val: 4.0 },
                    raw_arena
                ),
                MapInsertStatus::Inserted
            );

            let mut out = upb_MessageValue::zeroed();
            assert!(upb_Map_Get(map, upb_MessageValue { bool_val: true }, &mut out));
            assert!(matches!(out, upb_MessageValue { double_val: 4.0 }));
        }
    }
}
