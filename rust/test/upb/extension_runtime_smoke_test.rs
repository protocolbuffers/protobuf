// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use protobuf::__internal::runtime::{
    build_extension_mini_table, AssociatedMiniTable, ExtensionSub, InnerExtensionId,
    MiniTableExtensionInitPtr,
};
use protobuf::__internal::{new_repeated_extension_id, Private};
use protobuf::{ExtensionId, ProtoString, Repeated};
use std::sync::LazyLock;

use extensions_rust_proto::TestExtensions;

const EXT_NUMBER: u32 = 19001;
const UPB_FIELDTYPE_STRING: i32 = 9;
const UPB_FIELD_MOD_IS_REPEATED: u64 = 1 << 0;

#[repr(C)]
struct MtDataEncoder {
    end: *mut i8,
    internal: [i8; 32],
}

unsafe extern "C" {
    fn upb_MtDataEncoder_EncodeExtension(
        e: *mut MtDataEncoder,
        ptr: *mut i8,
        r#type: i32,
        field_num: u32,
        field_mod: u64,
    ) -> *mut i8;
}

fn repeated_string_extension_mini_descriptor() -> &'static str {
    let mut buf = [0i8; 32];
    let mut enc = MtDataEncoder {
        end: unsafe { buf.as_mut_ptr().add(buf.len()) },
        internal: [0; 32],
    };
    let start = buf.as_mut_ptr();
    let end = unsafe {
        upb_MtDataEncoder_EncodeExtension(
            &mut enc,
            start,
            UPB_FIELDTYPE_STRING,
            EXT_NUMBER,
            UPB_FIELD_MOD_IS_REPEATED,
        )
    };
    assert!(end >= start);
    let len = unsafe { end.offset_from(start) as usize };
    let bytes = unsafe { std::slice::from_raw_parts(start.cast::<u8>(), len) };
    let text = std::str::from_utf8(bytes).expect("mini descriptor should be valid utf8/base92");
    Box::leak(text.to_owned().into_boxed_str())
}

static MT: LazyLock<MiniTableExtensionInitPtr> = LazyLock::new(|| unsafe {
    MiniTableExtensionInitPtr(build_extension_mini_table(
        repeated_string_extension_mini_descriptor(),
        <TestExtensions as AssociatedMiniTable>::mini_table(),
        ExtensionSub::None,
    ))
});

static REP_STR_EXT_ID: LazyLock<ExtensionId<TestExtensions, Repeated<ProtoString>>> =
    LazyLock::new(|| new_repeated_extension_id(Private, EXT_NUMBER, InnerExtensionId::new(&MT)));

#[gtest]
fn repeated_extension_runtime_path_smoke() {
    let mut msg = TestExtensions::new();

    let mut rep = (*REP_STR_EXT_ID).get_mut(msg.as_mut());
    rep.push("aaa");
    rep.push("bbb");
    assert_that!(rep.len(), eq(2_usize));
    drop(rep);

    let view = (*REP_STR_EXT_ID).get(msg.as_view());
    assert_that!(view.len(), eq(2_usize));
}
