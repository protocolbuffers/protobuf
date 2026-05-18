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
use protobuf::__internal::{new_message_extension_id, new_repeated_extension_id, Private};
use protobuf::{ExtensionId, ProtoString, Repeated};
use std::sync::LazyLock;

use extensions_rust_proto::{SimpleSubmessage, TestExtensions};

const REP_STR_EXT_NUMBER: u32 = 19001;
const SUBMSG_EXT_NUMBER: u32 = 19002;
const UPB_FIELDTYPE_STRING: i32 = 9;
const UPB_FIELDTYPE_MESSAGE: i32 = 11;
const UPB_FIELD_MOD_NONE: u64 = 0;
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

fn extension_mini_descriptor(field_type: i32, field_num: u32, field_mod: u64) -> &'static str {
    let mut buf = [0i8; 32];
    let mut enc = MtDataEncoder {
        end: unsafe { buf.as_mut_ptr().add(buf.len()) },
        internal: [0; 32],
    };
    let start = buf.as_mut_ptr();
    let end =
        unsafe { upb_MtDataEncoder_EncodeExtension(&mut enc, start, field_type, field_num, field_mod) };
    assert!(end >= start);
    let len = unsafe { end.offset_from(start) as usize };
    let bytes = unsafe { std::slice::from_raw_parts(start.cast::<u8>(), len) };
    let text = std::str::from_utf8(bytes).expect("mini descriptor should be valid utf8/base92");
    Box::leak(text.to_owned().into_boxed_str())
}

static REP_STR_MT: LazyLock<MiniTableExtensionInitPtr> = LazyLock::new(|| unsafe {
    MiniTableExtensionInitPtr(build_extension_mini_table(
        extension_mini_descriptor(
            UPB_FIELDTYPE_STRING,
            REP_STR_EXT_NUMBER,
            UPB_FIELD_MOD_IS_REPEATED,
        ),
        <TestExtensions as AssociatedMiniTable>::mini_table(),
        ExtensionSub::None,
    ))
});

static REP_STR_EXT_ID: LazyLock<ExtensionId<TestExtensions, Repeated<ProtoString>>> =
    LazyLock::new(|| {
        new_repeated_extension_id(Private, REP_STR_EXT_NUMBER, InnerExtensionId::new(&REP_STR_MT))
    });

static SUBMSG_MT: LazyLock<MiniTableExtensionInitPtr> = LazyLock::new(|| unsafe {
    MiniTableExtensionInitPtr(build_extension_mini_table(
        extension_mini_descriptor(UPB_FIELDTYPE_MESSAGE, SUBMSG_EXT_NUMBER, UPB_FIELD_MOD_NONE),
        <TestExtensions as AssociatedMiniTable>::mini_table(),
        ExtensionSub::Message(<SimpleSubmessage as AssociatedMiniTable>::mini_table()),
    ))
});

static SUBMSG_EXT_ID: LazyLock<ExtensionId<TestExtensions, SimpleSubmessage>> = LazyLock::new(|| {
    new_message_extension_id(Private, SUBMSG_EXT_NUMBER, InnerExtensionId::new(&SUBMSG_MT))
});

#[gtest]
fn message_extension_get_mut_round_trip() {
    let mut msg = TestExtensions::new();

    {
        let mut sub = (*SUBMSG_EXT_ID).get_mut(msg.as_mut());
        sub.set_i32_field(7);
    }
    assert_that!((*SUBMSG_EXT_ID).get(msg.as_view()).i32_field(), eq(7_i32));

    {
        let mut sub = (*SUBMSG_EXT_ID).get_mut(msg.as_mut());
        sub.set_i32_field(11);
    }
    assert_that!((*SUBMSG_EXT_ID).get(msg.as_view()).i32_field(), eq(11_i32));
}

#[gtest]
fn extension_get_mut_lifecycle_stress() {
    let mut msg = TestExtensions::new();

    for i in 0..128 {
        {
            let mut rep = (*REP_STR_EXT_ID).get_mut(msg.as_mut());
            rep.push("abc");
            assert_that!(rep.len(), eq((i + 1) as usize));
        }
        assert_that!((*REP_STR_EXT_ID).get(msg.as_view()).len(), eq((i + 1) as usize));

        {
            let mut sub = (*SUBMSG_EXT_ID).get_mut(msg.as_mut());
            sub.set_i32_field(i);
        }
        assert_that!((*SUBMSG_EXT_ID).get(msg.as_view()).i32_field(), eq(i));
    }
}

