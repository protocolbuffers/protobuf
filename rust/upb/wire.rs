use crate::{upb_ExtensionRegistry, upb_MiniTable, Arena, OwnedArenaBox, RawArena, RawMessage};
use std::ptr::NonNull;

// LINT.IfChange(encode_status)
#[repr(C)]
#[derive(PartialEq, Eq, Copy, Clone, Debug)]
pub enum EncodeStatus {
    Ok = 0,
    OutOfMemory = 1,
    MaxDepthExceeded = 2,
    MissingRequired = 3,
}
// LINT.ThenChange()

// LINT.IfChange(decode_status)
#[repr(C)]
#[derive(PartialEq, Eq, Copy, Clone, Debug)]
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

/// If Err, then EncodeStatus != Ok.
///
/// SAFETY:
/// - `msg` must be associated with `mini_table`.
pub unsafe fn encode(
    msg: RawMessage,
    mini_table: *const upb_MiniTable,
) -> Result<OwnedArenaBox<[u8]>, EncodeStatus> {
    let arena = Arena::new();
    let mut buf: *mut u8 = std::ptr::null_mut();
    let mut len = 0usize;

    // SAFETY:
    // - `mini_table` is the one associated with `msg`.
    // - `buf` and `buf_size` are legally writable.
    let status = upb_Encode(msg, mini_table, 0, arena.raw(), &mut buf, &mut len);

    if status == EncodeStatus::Ok {
        assert!(!buf.is_null()); // EncodeStatus Ok should never return NULL data, even for len=0.
        // SAFETY: upb guarantees that `buf` is valid to read for `len`.
        let slice = NonNull::new_unchecked(std::ptr::slice_from_raw_parts_mut(buf, len));
        Ok(OwnedArenaBox::new(slice, arena))
    } else {
        Err(status)
    }
}

/// Decodes into the provided message (merge semantics). If Err, then
/// DecodeStatus != Ok.
///
/// SAFETY:
/// - `msg` must be mutable.
/// - `msg` must be associated with `mini_table`.
pub unsafe fn decode(
    buf: &[u8],
    msg: RawMessage,
    mini_table: *const upb_MiniTable,
    arena: &Arena,
) -> Result<(), DecodeStatus> {
    let len = buf.len();
    let buf = buf.as_ptr();
    // SAFETY:
    // - `mini_table` is the one associated with `msg`
    // - `buf` is legally readable for at least `buf_size` bytes.
    // - `extreg` is null.
    let status = upb_Decode(buf, len, msg, mini_table, std::ptr::null(), 0, arena.raw());
    match status {
        DecodeStatus::Ok => Ok(()),
        _ => Err(status),
    }
}

extern "C" {
    // SAFETY:
    // - `mini_table` is the one associated with `msg`
    // - `buf` and `buf_size` are legally writable.
    pub fn upb_Encode(
        msg: RawMessage,
        mini_table: *const upb_MiniTable,
        options: i32,
        arena: RawArena,
        buf: *mut *mut u8,
        buf_size: *mut usize,
    ) -> EncodeStatus;

    // SAFETY:
    // - `mini_table` is the one associated with `msg`
    // - `buf` is legally readable for at least `buf_size` bytes.
    // - `extreg` is either null or points at a valid upb_ExtensionRegistry.
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
