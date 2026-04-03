use super::*;

unsafe extern "C" {
    pub fn proto2_rust_cpp_new_string(src: PtrAndLen) -> CppStdString;
    pub fn proto2_rust_cpp_delete_string(src: CppStdString);
    pub fn proto2_rust_cpp_string_to_view(src: CppStdString) -> PtrAndLen;
}

/// Kernel-specific owned `string` and `bytes` field type.
#[derive(Debug)]
#[doc(hidden)]
pub struct InnerProtoString {
    owned_ptr: CppStdString,
}

impl Drop for InnerProtoString {
    fn drop(&mut self) {
        // SAFETY: `self.owned_ptr` points to a valid std::string object.
        unsafe {
            proto2_rust_cpp_delete_string(self.owned_ptr);
        }
    }
}

impl InnerProtoString {
    pub(crate) fn as_bytes(&self) -> &[u8] {
        // SAFETY: `self.owned_ptr` points to a valid std::string object.
        unsafe { proto2_rust_cpp_string_to_view(self.owned_ptr).as_ref() }
    }

    pub fn into_raw(self) -> CppStdString {
        let s = ManuallyDrop::new(self);
        s.owned_ptr
    }

    /// # Safety
    ///  - `src` points to a valid CppStdString.
    pub unsafe fn from_raw(src: CppStdString) -> InnerProtoString {
        InnerProtoString { owned_ptr: src }
    }
}

impl From<&[u8]> for InnerProtoString {
    fn from(val: &[u8]) -> Self {
        // SAFETY: `val` is valid byte slice.
        let owned_ptr: CppStdString = unsafe { proto2_rust_cpp_new_string(val.into()) };
        InnerProtoString { owned_ptr }
    }
}

/// Represents an ABI-stable version of `NonNull<[u8]>`/`string_view` (a
/// borrowed slice of bytes) for FFI use only.
///
/// Has semantics similar to `std::string_view` in C++ and `&[u8]` in Rust,
/// but is not ABI-compatible with either.
///
/// If `len` is 0, then `ptr` can be null or dangling. C++ considers a dangling
/// 0-len `std::string_view` to be invalid, and Rust considers a `&[u8]` with a
/// null data pointer to be invalid.
#[repr(C)]
#[derive(Copy, Clone)]
#[doc(hidden)]
pub struct PtrAndLen {
    /// Pointer to the first byte.
    /// Borrows the memory.
    pub ptr: *const u8,

    /// Length of the `[u8]` pointed to by `ptr`.
    pub len: usize,
}

impl PtrAndLen {
    /// Unsafely dereference this slice.
    ///
    /// # Safety
    /// - `self.ptr` must be dereferenceable and immutable for `self.len` bytes
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

impl From<&[u8]> for PtrAndLen {
    fn from(slice: &[u8]) -> Self {
        Self { ptr: slice.as_ptr(), len: slice.len() }
    }
}

impl From<&ProtoStr> for PtrAndLen {
    fn from(s: &ProtoStr) -> Self {
        let bytes = s.as_bytes();
        Self { ptr: bytes.as_ptr(), len: bytes.len() }
    }
}

/// Serialized Protobuf wire format data. It's typically produced by
/// `<Message>.serialize()`.
///
/// This struct is ABI-compatible with the equivalent struct on the C++ side. It
/// owns (and drops) its data.
#[repr(C)]
#[doc(hidden)]
pub struct SerializedData {
    /// Owns the memory.
    data: NonNull<u8>,
    len: usize,
}

impl SerializedData {
    pub fn new() -> Self {
        Self { data: NonNull::dangling(), len: 0 }
    }

    /// Constructs owned serialized data from raw components.
    ///
    /// # Safety
    /// - `data` must be readable for `len` bytes.
    /// - `data` must be an owned pointer and valid until deallocated.
    /// - `data` must have been allocated by the Rust global allocator with a
    ///   size of `len` and align of 1.
    pub unsafe fn from_raw_parts(data: NonNull<u8>, len: usize) -> Self {
        Self { data, len }
    }

    /// Gets a raw slice pointer.
    pub fn as_ptr(&self) -> *const [u8] {
        ptr::slice_from_raw_parts(self.data.as_ptr(), self.len)
    }

    /// Gets a mutable raw slice pointer.
    fn as_mut_ptr(&mut self) -> *mut [u8] {
        ptr::slice_from_raw_parts_mut(self.data.as_ptr(), self.len)
    }

    /// Converts into a Vec<u8>.
    pub fn into_vec(self) -> Vec<u8> {
        // We need to prevent self from being dropped, because we are going to transfer
        // ownership of self.data to the Vec<u8>.
        let s = ManuallyDrop::new(self);

        unsafe {
            // SAFETY:
            // - `data` was allocated by the Rust global allocator.
            // - `data` was allocated with an alignment of 1 for u8.
            // - The allocated size was `len`.
            // - The length and capacity are equal.
            // - All `len` bytes are initialized.
            // - The capacity (`len` in this case) is the size the pointer was allocated
            //   with.
            // - The allocated size is no more than isize::MAX, because the protobuf
            //   serializer will refuse to serialize a message if the output would exceed
            //   2^31 - 1 bytes.
            Vec::<u8>::from_raw_parts(s.data.as_ptr(), s.len, s.len)
        }
    }
}

impl Deref for SerializedData {
    type Target = [u8];
    fn deref(&self) -> &Self::Target {
        // SAFETY: `data` is valid for `len` bytes until deallocated as promised by
        // `from_raw_parts`.
        unsafe { &*self.as_ptr() }
    }
}

impl Drop for SerializedData {
    fn drop(&mut self) {
        // SAFETY: `data` was allocated by the Rust global allocator with a
        // size of `len` and align of 1 as promised by `from_raw_parts`.
        unsafe { drop(Box::from_raw(self.as_mut_ptr())) }
    }
}

impl fmt::Debug for SerializedData {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        fmt::Debug::fmt(self.deref(), f)
    }
}

/// A type to transfer an owned Rust string across the FFI boundary:
///   * This struct is ABI-compatible with the equivalent C struct.
///   * It owns its data but does not drop it. Immediately turn it into a
///     `String` by calling `.into()` on it.
///   * `.data` points to a valid UTF-8 string that has been allocated with the
///     Rust allocator and is 1-byte aligned.
///   * `.data` contains exactly `.len` bytes.
///   * The empty string is represented as `.data.is_null() == true`.
#[repr(C)]
#[doc(hidden)]
pub struct RustStringRawParts {
    data: *const u8,
    len: usize,
}

impl From<RustStringRawParts> for String {
    fn from(value: RustStringRawParts) -> Self {
        if value.data.is_null() {
            // Handle the case where the string is empty.
            return String::new();
        }
        // SAFETY:
        //  - `value.data` contains valid UTF-8 bytes as promised by
        //    `RustStringRawParts`.
        //  - `value.data` has been allocated with the Rust allocator and is 1-byte
        //    aligned as promised by `RustStringRawParts`.
        //  - `value.data` contains and is allocated for exactly `value.len` bytes.
        unsafe { String::from_raw_parts(value.data as *mut u8, value.len, value.len) }
    }
}

unsafe extern "C" {
    fn proto2_rust_utf8_debug_string(raw: RawMessage) -> RustStringRawParts;
}

pub fn debug_string(raw: RawMessage, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    // SAFETY:
    // - `raw` is a valid protobuf message.
    let dbg_str: String = unsafe { proto2_rust_utf8_debug_string(raw) }.into();
    write!(f, "{dbg_str}")
}

pub fn str_to_ptrlen<'msg>(val: impl Into<&'msg ProtoStr>) -> PtrAndLen {
    val.into().as_bytes().into()
}

// Warning: this function is unsound on its own! `val.as_ref()` must be safe to
// call.
pub fn ptrlen_to_str<'msg>(val: PtrAndLen) -> &'msg ProtoStr {
    ProtoStr::from_utf8_unchecked(unsafe { val.as_ref() })
}

pub fn protostr_into_cppstdstring(val: ProtoString) -> CppStdString {
    val.into_inner(Private).into_raw()
}

pub fn protobytes_into_cppstdstring(val: ProtoBytes) -> CppStdString {
    val.into_inner(Private).into_raw()
}

// Warning: this function is unsound on its own! `val.as_ref()` must be safe to
// call.
pub fn ptrlen_to_bytes<'msg>(val: PtrAndLen) -> &'msg [u8] {
    unsafe { val.as_ref() }
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::prelude::*;

    // We need to allocate the byte array so SerializedData can own it and
    // deallocate it in its drop. This function makes it easier to do so for our
    // tests.
    fn allocate_byte_array(content: &'static [u8]) -> (*mut u8, usize) {
        let content: &mut [u8] = Box::leak(content.into());
        (content.as_mut_ptr(), content.len())
    }

    #[gtest]
    fn test_serialized_data_roundtrip() {
        let (ptr, len) = allocate_byte_array(b"Hello world");
        let serialized_data = SerializedData { data: NonNull::new(ptr).unwrap(), len };
        assert_that!(&*serialized_data, eq(b"Hello world"));
    }

    #[gtest]
    fn test_empty_string() {
        let empty_str: String = RustStringRawParts { data: std::ptr::null(), len: 0 }.into();
        assert_that!(empty_str, eq(""));
    }
}
