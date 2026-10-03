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
        let arena = Arena::new_sized(val.len());
        let in_arena_copy = arena.copy_slice_in(val).unwrap();
        // SAFETY:
        // - `in_arena_copy` is valid slice that will live for `arena`'s lifetime and this is the
        //   only reference in the program to it.
        // - `in_arena_copy` is a pointer into an allocation on `arena`
        InnerProtoString(unsafe { OwnedArenaBox::new(Into::into(in_arena_copy), arena) })
    }
}

impl<'a> crate::proxied::ProxiedInArena<'a, ProtoString> {
    pub(crate) fn into_view(self, parent_arena: &'a Arena) -> PtrAndLen {
        match self {
            Self::Owned(s) => {
                let (view, arena) = s.into_inner(Private).into_raw_parts();
                parent_arena.fuse(&arena);
                view
            }
            Self::Borrowed(view) => view.into(),
        }
    }
}

impl<'a> crate::proxied::ProxiedInArena<'a, ProtoBytes> {
    pub(crate) fn into_view(self, parent_arena: &'a Arena) -> PtrAndLen {
        match self {
            Self::Owned(s) => {
                let (view, arena) = s.into_inner(Private).into_raw_parts();
                parent_arena.fuse(&arena);
                view
            }
            Self::Borrowed(view) => view.into(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::proxied::ProxiedInArena;
    use googletest::prelude::*;

    #[gtest]
    fn test_into_proxied_in_arena_borrows_from_target_arena() {
        let arena = Arena::new();
        let s: ProxiedInArena<'_, ProtoString> =
            "hello world".into_proxied_in_arena(Private, &arena);
        let ProxiedInArena::Borrowed(s_view) = s else {
            panic!("expected Borrowed variant");
        };
        assert_that!(s_view, eq("hello world"));

        let b: ProxiedInArena<'_, ProtoBytes> =
            vec![1u8, 2, 3, 4].into_proxied_in_arena(Private, &arena);
        let ProxiedInArena::Borrowed(b_view) = b else {
            panic!("expected Borrowed variant");
        };
        assert_that!(b_view, eq(&[1u8, 2, 3, 4][..]));
    }

    #[gtest]
    fn test_into_proxied_in_arena_preserves_owned_proto_string_for_fusing() {
        let arena = Arena::new();
        let prebuilt = ProtoString::from("already owned");
        let s: ProxiedInArena<'_, ProtoString> = prebuilt.into_proxied_in_arena(Private, &arena);
        assert_that!(matches!(s, ProxiedInArena::Owned(_)), eq(true));
        let view = s.into_view(&arena);
        assert_that!(unsafe { view.as_ref() }, eq(b"already owned"));
    }
}
