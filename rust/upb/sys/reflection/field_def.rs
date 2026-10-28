// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::message_def::RawMessageDef;
use super::opaque_pointee::opaque_pointee;
use core::ptr::NonNull;

opaque_pointee!(upb_FieldDef);
pub type RawFieldDef = NonNull<upb_FieldDef>;

unsafe extern "C" {
    /// Returns the message type of a message or group field, which for a map field is its entry
    /// message, and `None` for any other field.
    ///
    /// # Safety
    /// - `f` must be a valid pointer to a `upb_FieldDef` whose `upb_DefPool` has not been freed.
    pub fn upb_FieldDef_MessageSubDef(f: RawFieldDef) -> Option<RawMessageDef>;
}

#[cfg(test)]
mod tests {
    use googletest::gtest;

    #[gtest]
    fn assert_field_def_linked() {
        use super::super::test_helpers::assert_linked;
        assert_linked!(super::upb_FieldDef_MessageSubDef);
    }
}
