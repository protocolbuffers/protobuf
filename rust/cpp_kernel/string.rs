// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::*;

unsafe extern "C" {
    pub fn proto2_rust_cpp_new_string(src: PtrAndLen) -> CppStdString;
    pub fn proto2_rust_cpp_delete_string(src: CppStdString);
    pub fn proto2_rust_cpp_string_to_view(src: CppStdString) -> PtrAndLen;

    fn proto2_rust_cord_len(cord: RawCord) -> usize;
    fn proto2_rust_cord_try_flat(cord: RawCord, out: &mut PtrAndLen) -> bool;
    fn proto2_rust_cord_copy_to_bytes(cord: RawCord, dst: *mut u8);
    fn proto2_rust_cord_eq(lhs: RawCord, rhs: RawCord) -> bool;
    fn proto2_rust_cord_eq_bytes(lhs: RawCord, rhs: PtrAndLen) -> bool;

    fn proto2_rust_cord_iter_new(cord: RawCord) -> RawCordChunkIter;
    fn proto2_rust_cord_iter_next(iter: RawCordChunkIter, out: &mut PtrAndLen) -> bool;
    fn proto2_rust_cord_iter_clone(iter: RawCordChunkIter) -> RawCordChunkIter;
    fn proto2_rust_cord_iter_delete(iter: RawCordChunkIter);

    fn proto2_rust_cord_new_empty() -> RawCord;
    fn proto2_rust_cord_new_from_input(input: RawCordInput) -> RawCord;
    fn proto2_rust_cord_clone(cord: RawCord) -> RawCord;
    fn proto2_rust_cord_swap(lhs: RawCord, rhs: RawCord);
    fn proto2_rust_cord_delete(cord: RawCord);
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

#[cold]
#[inline(never)]
#[track_caller]
fn null_slice_with_nonzero_len(len: usize) -> ! {
    panic!("Non-empty slice with null data pointer (len: {len})");
}

impl PtrAndLen {
    /// Unsafely dereference this slice.
    ///
    /// # Safety
    /// - `self.ptr` must be dereferenceable and immutable for `self.len` bytes for the lifetime
    ///   `'a`. It can be null or dangling if `self.len == 0`.
    pub unsafe fn as_ref<'a>(self) -> &'a [u8] {
        if self.ptr.is_null() {
            if self.len != 0 {
                null_slice_with_nonzero_len(self.len);
            }
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
    /// - `data` must have been allocated by the Rust global allocator with a size of `len` and
    ///   align of 1.
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
            // - The capacity (`len` in this case) is the size the pointer was allocated with.
            // - The allocated size is no more than isize::MAX, because the protobuf serializer will
            //   refuse to serialize a message if the output would exceed 2^31 - 1 bytes.
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
///   * It owns its data but does not drop it. Immediately turn it into a `String` by calling
///     `.into()` on it.
///   * `.data` points to a valid UTF-8 string that has been allocated with the Rust allocator and
///     is 1-byte aligned.
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
        //  - `value.data` contains valid UTF-8 bytes as promised by `RustStringRawParts`.
        //  - `value.data` has been allocated with the Rust allocator and is 1-byte aligned as
        //    promised by `RustStringRawParts`.
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

/// Converts a `PtrAndLen` borrowed from C++ into a `&ProtoStr`.
///
/// # Safety
/// - `val` must satisfy the safety requirements of [`PtrAndLen::as_ref`] for the lifetime `'msg`.
pub unsafe fn ptrlen_to_str<'msg>(val: PtrAndLen) -> &'msg ProtoStr {
    // SAFETY: the caller upholds the requirements of `PtrAndLen::as_ref`.
    ProtoStr::from_utf8_unchecked(unsafe { val.as_ref() })
}

pub fn protostr_into_cppstdstring(val: ProtoString) -> CppStdString {
    val.into_inner(Private).into_raw()
}

pub fn protobytes_into_cppstdstring(val: ProtoBytes) -> CppStdString {
    val.into_inner(Private).into_raw()
}

/// Converts a `PtrAndLen` borrowed from C++ into a `&[u8]`.
///
/// # Safety
/// - `val` must satisfy the safety requirements of [`PtrAndLen::as_ref`] for the lifetime `'msg`.
pub unsafe fn ptrlen_to_bytes<'msg>(val: PtrAndLen) -> &'msg [u8] {
    // SAFETY: the caller upholds the requirements of `PtrAndLen::as_ref`.
    unsafe { val.as_ref() }
}

#[repr(u8)]
#[derive(Copy, Clone, Debug)]
#[doc(hidden)]
pub enum RawCordInputKind {
    BorrowedSlice = 0,
    StaticSlice = 1,
    BorrowedCord = 2,
    OwnedCord = 3,
    RustVec = 4,
    CppStdString = 5,
}

#[repr(C)]
#[derive(Copy, Clone)]
#[doc(hidden)]
pub struct RawCordInput {
    pub kind: RawCordInputKind,
    pub ptr: *const c_void,
    pub len: usize,
    pub cap: usize,
}

/// Converts a `ProtoCordValue<'a>` into an FFI `RawCordInput`.
///
/// # Safety
/// - The returned `RawCordInput` must be consumed at most once (and exactly once for owned
///   variants) by `proto2::rust::TakeCordInput` before lifetime `'a` ends.
#[doc(hidden)]
pub unsafe fn raw_cord_input_from_value(value: ProtoCordValue<'_>) -> RawCordInput {
    match value {
        ProtoCordValue::BorrowedSlice(slice) => RawCordInput {
            kind: RawCordInputKind::BorrowedSlice,
            ptr: slice.as_ptr().cast(),
            len: slice.len(),
            cap: 0,
        },
        ProtoCordValue::BorrowedCord(cord) => match cord.inner {
            InnerProtoCord::Slice(slice) => RawCordInput {
                kind: RawCordInputKind::BorrowedSlice,
                ptr: slice.as_ptr().cast(),
                len: slice.len(),
                cap: 0,
            },
            InnerProtoCord::Cord(raw, _) => RawCordInput {
                kind: RawCordInputKind::BorrowedCord,
                ptr: raw.as_ptr().cast(),
                len: 0,
                cap: 0,
            },
        },
        ProtoCordValue::OwnedCord(owned) => {
            let raw = owned.into_inner(Private).into_raw();
            RawCordInput {
                kind: RawCordInputKind::OwnedCord,
                ptr: raw.as_ptr().cast(),
                len: 0,
                cap: 0,
            }
        }
        ProtoCordValue::OwnedVec(vec) => {
            let mut vec = ManuallyDrop::new(vec);
            RawCordInput {
                kind: RawCordInputKind::RustVec,
                ptr: vec.as_mut_ptr().cast(),
                len: vec.len(),
                cap: vec.capacity(),
            }
        }
        ProtoCordValue::OwnedProtoString(inner) => {
            let raw = inner.into_raw();
            RawCordInput {
                kind: RawCordInputKind::CppStdString,
                ptr: raw.as_ptr().cast(),
                len: 0,
                cap: 0,
            }
        }
    }
}

/// Helper used by generated `set_<field>_cord` methods on the C++ kernel.
///
/// # Safety
/// - `f` must consume the `RawCordInput` exactly once via `proto2::rust::TakeCordInput` before
///   returning and must not retain any borrowed pointers past `f`.
#[doc(hidden)]
pub unsafe fn with_raw_cord_input(val: impl IntoProtoCord, f: impl FnOnce(RawCordInput)) {
    val.with_cord_value(Private, |value| {
        // SAFETY: `f` consumes `raw_input` once via `TakeCordInput` while `value`'s borrow is live,
        // as guaranteed by the caller of `with_raw_cord_input`.
        let raw_input = unsafe { raw_cord_input_from_value(value) };
        f(raw_input);
    });
}

#[derive(Copy, Clone)]
#[doc(hidden)]
pub enum InnerProtoCord<'msg> {
    Slice(&'msg [u8]),
    Cord(RawCord, PhantomData<&'msg [u8]>),
}

// SAFETY: `absl::Cord` uses atomic reference counting on shared tree nodes and is safe to read
// concurrently across threads (`Send + Sync` when immutably borrowed).
unsafe impl Send for InnerProtoCord<'_> {}
// SAFETY: See `Send` safety comment above.
unsafe impl Sync for InnerProtoCord<'_> {}

impl<'msg> InnerProtoCord<'msg> {
    pub const fn empty() -> Self {
        Self::Slice(b"")
    }

    pub const fn from_slice(bytes: &'msg [u8]) -> Self {
        Self::Slice(bytes)
    }

    /// # Safety
    /// - `raw` must point to a valid `absl::Cord` that remains alive and unmutated for `'msg`.
    pub unsafe fn from_raw(raw: RawCord) -> Self {
        Self::Cord(raw, PhantomData)
    }

    pub fn as_raw(&self) -> Option<RawCord> {
        match *self {
            Self::Slice(_) => None,
            Self::Cord(raw, _) => Some(raw),
        }
    }

    pub fn len(&self) -> usize {
        match *self {
            Self::Slice(s) => s.len(),
            // SAFETY: `raw` is a valid `const absl::Cord*` for `'msg`.
            Self::Cord(raw, _) => unsafe { proto2_rust_cord_len(raw) },
        }
    }

    pub fn try_flat(&self) -> Option<&'msg [u8]> {
        match *self {
            Self::Slice(s) => Some(s),
            Self::Cord(raw, _) => {
                let mut out = PtrAndLen { ptr: ptr::null(), len: 0 };
                // SAFETY: `raw` is a valid `const absl::Cord*` for `'msg`.
                if unsafe { proto2_rust_cord_try_flat(raw, &mut out) } {
                    // SAFETY: `out` borrows from `raw` which lives and is immutable for `'msg`.
                    Some(unsafe { out.as_ref() })
                } else {
                    None
                }
            }
        }
    }

    pub fn chunks(&self) -> InnerProtoCordChunks<'msg> {
        if let Some(flat) = self.try_flat() {
            return InnerProtoCordChunks::Flat(if flat.is_empty() { None } else { Some(flat) });
        }
        match *self {
            Self::Slice(s) => InnerProtoCordChunks::Flat(if s.is_empty() { None } else { Some(s) }),
            Self::Cord(raw, _) => {
                // SAFETY: `raw` is a valid `const absl::Cord*` for `'msg`.
                let iter = unsafe { proto2_rust_cord_iter_new(raw) };
                InnerProtoCordChunks::Tree { iter, _phantom: PhantomData }
            }
        }
    }

    pub fn copy_to_slice(&self, dst: &mut [u8]) {
        let len = self.len();
        assert_eq!(
            len,
            dst.len(),
            "destination slice length ({}) does not match cord length ({})",
            dst.len(),
            len
        );
        if len == 0 {
            return;
        }
        match *self {
            Self::Slice(s) => dst.copy_from_slice(s),
            Self::Cord(raw, _) => {
                // SAFETY: `raw` has length `len == dst.len() > 0`, and `dst` is writable for `len`
                // bytes.
                unsafe { proto2_rust_cord_copy_to_bytes(raw, dst.as_mut_ptr()) };
            }
        }
    }

    pub fn to_vec(&self) -> Vec<u8> {
        match *self {
            Self::Slice(s) => s.to_vec(),
            Self::Cord(raw, _) => {
                let len = self.len();
                if len == 0 {
                    return Vec::new();
                }
                let mut vec = Vec::with_capacity(len);
                // SAFETY: `raw` has `len` bytes and `vec` has capacity >= `len`.
                unsafe {
                    proto2_rust_cord_copy_to_bytes(raw, vec.as_mut_ptr());
                    vec.set_len(len);
                }
                vec
            }
        }
    }

    pub fn eq_cord(&self, other: &InnerProtoCord<'_>) -> bool {
        match (*self, *other) {
            (InnerProtoCord::Slice(a), InnerProtoCord::Slice(b)) => a == b,
            (InnerProtoCord::Cord(a, _), InnerProtoCord::Cord(b, _)) => {
                // SAFETY: `a` and `b` are valid `const absl::Cord*` pointers.
                unsafe { proto2_rust_cord_eq(a, b) }
            }
            (InnerProtoCord::Cord(a, _), InnerProtoCord::Slice(b))
            | (InnerProtoCord::Slice(b), InnerProtoCord::Cord(a, _)) => {
                // SAFETY: `a` is a valid `const absl::Cord*` and `b` is a valid byte slice.
                unsafe { proto2_rust_cord_eq_bytes(a, b.into()) }
            }
        }
    }

    pub fn eq_bytes(&self, other: &[u8]) -> bool {
        match *self {
            Self::Slice(a) => a == other,
            Self::Cord(raw, _) => {
                // SAFETY: `raw` is a valid `const absl::Cord*` and `other` is a valid byte slice.
                unsafe { proto2_rust_cord_eq_bytes(raw, other.into()) }
            }
        }
    }

    pub fn to_owned_cord(&self) -> InnerOwnedProtoCord {
        match *self {
            Self::Slice(s) => {
                let input = RawCordInput {
                    kind: RawCordInputKind::BorrowedSlice,
                    ptr: s.as_ptr().cast(),
                    len: s.len(),
                    cap: 0,
                };
                // SAFETY: `s` is valid for the duration of `proto2_rust_cord_new_from_input`.
                InnerOwnedProtoCord { raw: unsafe { proto2_rust_cord_new_from_input(input) } }
            }
            Self::Cord(raw, _) => {
                // SAFETY: `raw` is a valid `const absl::Cord*`.
                InnerOwnedProtoCord { raw: unsafe { proto2_rust_cord_clone(raw) } }
            }
        }
    }
}

#[doc(hidden)]
pub enum InnerProtoCordChunks<'msg> {
    Flat(Option<&'msg [u8]>),
    Tree { iter: RawCordChunkIter, _phantom: PhantomData<&'msg [u8]> },
}

// SAFETY: `Cord::ChunkIterator` only reads immutable `absl::Cord` nodes live for `'msg`.
unsafe impl Send for InnerProtoCordChunks<'_> {}
// SAFETY: `Cord::ChunkIterator` has no interior mutability when shared by `&self`.
unsafe impl Sync for InnerProtoCordChunks<'_> {}

impl Clone for InnerProtoCordChunks<'_> {
    fn clone(&self) -> Self {
        match *self {
            Self::Flat(opt) => Self::Flat(opt),
            Self::Tree { iter, _phantom } => {
                // SAFETY: `iter` is a valid `CordChunkIter*`.
                let cloned = unsafe { proto2_rust_cord_iter_clone(iter) };
                Self::Tree { iter: cloned, _phantom }
            }
        }
    }
}

impl Drop for InnerProtoCordChunks<'_> {
    fn drop(&mut self) {
        if let Self::Tree { iter, .. } = *self {
            // SAFETY: `iter` was allocated by `proto2_rust_cord_iter_new` or `clone` and is
            // exclusively owned by `self`.
            unsafe { proto2_rust_cord_iter_delete(iter) };
        }
    }
}

impl<'msg> Iterator for InnerProtoCordChunks<'msg> {
    type Item = &'msg [u8];

    fn next(&mut self) -> Option<Self::Item> {
        match self {
            Self::Flat(opt) => opt.take(),
            Self::Tree { iter, .. } => {
                let mut out = PtrAndLen { ptr: ptr::null(), len: 0 };
                // SAFETY: `iter` is a valid `CordChunkIter*` for `'msg`.
                if unsafe { proto2_rust_cord_iter_next(*iter, &mut out) } {
                    // SAFETY: `out` points to a chunk of the `absl::Cord` valid for `'msg`.
                    Some(unsafe { out.as_ref() })
                } else {
                    None
                }
            }
        }
    }
}

#[doc(hidden)]
pub struct InnerOwnedProtoCord {
    raw: RawCord,
}

// SAFETY: `InnerOwnedProtoCord` uniquely owns its heap-allocated `absl::Cord`, and `absl::Cord`'s
// internal reference counts are atomic.
unsafe impl Send for InnerOwnedProtoCord {}
// SAFETY: Shared references `&InnerOwnedProtoCord` only perform `const` operations on `absl::Cord`.
unsafe impl Sync for InnerOwnedProtoCord {}

impl InnerOwnedProtoCord {
    pub fn new() -> Self {
        // SAFETY: Allocates an empty `absl::Cord` on the C++ heap.
        Self { raw: unsafe { proto2_rust_cord_new_empty() } }
    }

    pub fn from_static(bytes: &'static [u8]) -> Self {
        let input = RawCordInput {
            kind: RawCordInputKind::StaticSlice,
            ptr: bytes.as_ptr().cast(),
            len: bytes.len(),
            cap: 0,
        };
        // SAFETY: `bytes` has `'static` lifetime.
        Self { raw: unsafe { proto2_rust_cord_new_from_input(input) } }
    }

    pub fn from_input(val: impl IntoProtoCord) -> Self {
        val.with_cord_value(Private, |value| match value {
            ProtoCordValue::OwnedCord(owned) => owned.into_inner(Private),
            other => {
                // SAFETY: `raw_input` is consumed immediately once by
                // `proto2_rust_cord_new_from_input` while `other`'s borrow is live.
                let raw_input = unsafe { raw_cord_input_from_value(other) };
                // SAFETY: `raw_input` is valid.
                Self { raw: unsafe { proto2_rust_cord_new_from_input(raw_input) } }
            }
        })
    }

    /// # Safety
    /// - `raw` must be a valid heap-allocated `absl::Cord*` allocated with `new absl::Cord`,
    ///   transferring exclusive ownership to `Self`.
    pub unsafe fn from_raw(raw: RawCord) -> Self {
        Self { raw }
    }

    pub fn raw(&self) -> RawCord {
        self.raw
    }

    pub fn into_raw(self) -> RawCord {
        let me = ManuallyDrop::new(self);
        me.raw
    }

    pub fn as_view(&self) -> InnerProtoCord<'_> {
        InnerProtoCord::Cord(self.raw, PhantomData)
    }

    pub fn into_vec(self) -> Vec<u8> {
        self.as_view().to_vec()
    }

    /// # Safety
    /// - `other` must point to a valid, mutable `absl::Cord` that does not alias `self.raw`.
    pub unsafe fn swap_raw(&mut self, other: RawCord) {
        // SAFETY: Both `self.raw` and `other` are valid distinct `absl::Cord*` pointers.
        unsafe { proto2_rust_cord_swap(self.raw, other) };
    }
}

impl Clone for InnerOwnedProtoCord {
    fn clone(&self) -> Self {
        // SAFETY: `self.raw` points to a valid `absl::Cord`.
        Self { raw: unsafe { proto2_rust_cord_clone(self.raw) } }
    }
}

impl Drop for InnerOwnedProtoCord {
    fn drop(&mut self) {
        // SAFETY: `self.raw` points to an `absl::Cord` exclusively owned by `self`.
        unsafe { proto2_rust_cord_delete(self.raw) };
    }
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
