// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#[path = "mini_table/enum.rs"]
pub mod r#enum;
pub mod field;
pub mod message;

pub use r#enum::MiniTableEnum;
pub use field::MiniTableField;
pub use message::MiniTable;

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::gtest;

    #[gtest]
    fn assert_mini_table_linked() {
        use super::super::assert_linked;
        assert_linked!(upb_MiniTable_FindFieldByNumber);
        assert_linked!(upb_MiniTable_GetFieldByIndex);
        assert_linked!(upb_MiniTable_SubMessage);
    }
}
