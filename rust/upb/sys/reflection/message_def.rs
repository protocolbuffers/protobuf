// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::field_def::RawFieldDef;
use super::opaque_pointee::opaque_pointee;
use core::ffi::c_int;
use core::ptr::NonNull;

opaque_pointee!(upb_MessageDef);
pub type RawMessageDef = NonNull<upb_MessageDef>;

unsafe extern "C" {
    /// Returns the number of fields of the message.
    ///
    /// # Safety
    /// - `m` must be a valid pointer to a `upb_MessageDef` whose `upb_DefPool` has not been freed.
    pub fn upb_MessageDef_FieldCount(m: RawMessageDef) -> c_int;

    /// Returns the field at index `i` of the message.
    ///
    /// # Safety
    /// - `m` must be a valid pointer to a `upb_MessageDef` whose `upb_DefPool` has not been freed.
    /// - `i` must be in `0..upb_MessageDef_FieldCount(m)`.
    pub fn upb_MessageDef_Field(m: RawMessageDef, i: c_int) -> RawFieldDef;

    /// Returns the number of extension ranges the message declares.
    ///
    /// # Safety
    /// - `m` must be a valid pointer to a `upb_MessageDef` whose `upb_DefPool` has not been freed.
    pub fn upb_MessageDef_ExtensionRangeCount(m: RawMessageDef) -> c_int;
}

#[cfg(test)]
mod tests {
    use googletest::gtest;

    #[gtest]
    fn assert_message_def_linked() {
        use super::super::test_helpers::assert_linked;
        assert_linked!(super::upb_MessageDef_FieldCount);
        assert_linked!(super::upb_MessageDef_Field);
        assert_linked!(super::upb_MessageDef_ExtensionRangeCount);
    }
}
