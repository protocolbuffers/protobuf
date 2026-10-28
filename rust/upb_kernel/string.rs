// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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

#[derive(Copy, Clone)]
#[doc(hidden)]
pub struct InnerProtoCord<'msg>(&'msg [u8]);

impl<'msg> InnerProtoCord<'msg> {
    pub const fn empty() -> Self {
        Self(b"")
    }

    pub const fn from_slice(bytes: &'msg [u8]) -> Self {
        Self(bytes)
    }

    pub const fn as_slice(&self) -> &'msg [u8] {
        self.0
    }

    pub fn len(&self) -> usize {
        self.0.len()
    }

    pub fn try_flat(&self) -> Option<&'msg [u8]> {
        Some(self.0)
    }

    pub fn chunks(&self) -> InnerProtoCordChunks<'msg> {
        InnerProtoCordChunks(if self.0.is_empty() { None } else { Some(self.0) })
    }

    pub fn copy_to_slice(&self, dst: &mut [u8]) {
        assert_eq!(
            self.0.len(),
            dst.len(),
            "destination slice length ({}) does not match cord length ({})",
            dst.len(),
            self.0.len()
        );
        dst.copy_from_slice(self.0);
    }

    pub fn to_vec(&self) -> Vec<u8> {
        self.0.to_vec()
    }

    pub fn eq_cord(&self, other: &InnerProtoCord<'_>) -> bool {
        self.0 == other.0
    }

    pub fn eq_bytes(&self, other: &[u8]) -> bool {
        self.0 == other
    }

    pub fn to_owned_cord(&self) -> InnerOwnedProtoCord {
        InnerOwnedProtoCord::Vec(self.0.to_vec())
    }
}

#[derive(Clone)]
#[doc(hidden)]
pub struct InnerProtoCordChunks<'msg>(Option<&'msg [u8]>);

impl<'msg> Iterator for InnerProtoCordChunks<'msg> {
    type Item = &'msg [u8];

    fn next(&mut self) -> Option<Self::Item> {
        self.0.take()
    }
}

#[derive(Clone)]
#[doc(hidden)]
pub enum InnerOwnedProtoCord {
    Static(&'static [u8]),
    Vec(Vec<u8>),
}

impl InnerOwnedProtoCord {
    pub const fn new() -> Self {
        Self::Static(b"")
    }

    pub const fn from_static(bytes: &'static [u8]) -> Self {
        Self::Static(bytes)
    }

    pub fn from_input(val: impl IntoProtoCord) -> Self {
        val.with_cord_value(Private, |value| match value {
            ProtoCordValue::BorrowedSlice(s) => Self::Vec(s.to_vec()),
            ProtoCordValue::BorrowedCord(c) => Self::Vec(c.inner.0.to_vec()),
            ProtoCordValue::OwnedCord(o) => o.into_inner(Private),
            ProtoCordValue::OwnedVec(v) => Self::Vec(v),
            ProtoCordValue::OwnedProtoString(s) => Self::Vec(s.as_bytes().to_vec()),
        })
    }

    pub fn as_view(&self) -> InnerProtoCord<'_> {
        match self {
            Self::Static(s) => InnerProtoCord(s),
            Self::Vec(v) => InnerProtoCord(v.as_slice()),
        }
    }

    pub fn into_vec(self) -> Vec<u8> {
        match self {
            Self::Static(s) => s.to_vec(),
            Self::Vec(v) => v,
        }
    }
}
