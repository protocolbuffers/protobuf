use super::*;

impl From<&ProtoStr> for PtrAndLen {
    fn from(s: &ProtoStr) -> Self {
        let bytes = s.as_bytes();
        Self { ptr: bytes.as_ptr(), len: bytes.len() }
    }
}

/// Kernel-specific owned `string` and `bytes` field type.
#[doc(hidden)]
pub struct InnerProtoString(OwnedArenaBox<[u8]>);

impl InnerProtoString {
    pub(crate) fn as_bytes(&self) -> &[u8] {
        &self.0
    }

    #[doc(hidden)]
    pub fn into_raw_parts(self) -> (PtrAndLen, Arena) {
        let (data_ptr, arena) = self.0.into_parts();
        (unsafe { data_ptr.as_ref().into() }, arena)
    }
}

impl From<&[u8]> for InnerProtoString {
    fn from(val: &[u8]) -> InnerProtoString {
        let arena = Arena::new();
        let in_arena_copy = arena.copy_slice_in(val).unwrap();
        // SAFETY:
        // - `in_arena_copy` is valid slice that will live for `arena`'s lifetime and
        //   this is the only reference in the program to it.
        // - `in_arena_copy` is a pointer into an allocation on `arena`
        InnerProtoString(unsafe { OwnedArenaBox::new(Into::into(in_arena_copy), arena) })
    }
}
