// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#[cfg(not(bzl))]
mod protos;
#[cfg(not(bzl))]
use protos::*;

use googletest::prelude::*;
use protobuf::prelude::*;
use protobuf::{OwnedProtoCord, ProtoBytes, ProtoCord, ProtoString, SerializeOptions};
use std::borrow::Cow;

fn assert_send_sync<T: Send + Sync>() {}

#[gtest]
fn test_bytes_cord() {
    use unittest_rust_proto::TestCord;

    assert_send_sync::<ProtoCord<'_>>();
    assert_send_sync::<OwnedProtoCord>();

    let mut msg = TestCord::new();
    assert_that!(msg.has_optional_bytes_cord(), eq(false));
    assert_that!(msg.optional_bytes_cord(), eq("".as_bytes()));
    assert_that!(msg.optional_bytes_cord_cord_opt(), none());
    assert_that!(msg.optional_bytes_cord_cord().len(), eq(0));
    assert_that!(msg.optional_bytes_cord_cord().chunks().count(), eq(0));

    msg.set_optional_bytes_cord(b"hello");
    assert_that!(msg.has_optional_bytes_cord(), eq(true));
    assert_that!(msg.optional_bytes_cord(), eq("hello".as_bytes()));
    assert_that!(msg.optional_bytes_cord_cord_opt().is_some(), eq(true));
    assert_that!(msg.optional_bytes_cord_cord() == b"hello", eq(true));

    let mut msg2 = TestCord::new();
    msg2.set_optional_bytes_cord(msg.optional_bytes_cord());
    assert_that!(msg2.optional_bytes_cord(), eq("hello".as_bytes()));

    // Exercise View and Mut `_cord` accessors as well.
    assert_that!(msg2.as_view().optional_bytes_cord_cord() == b"hello", eq(true));
    assert_that!(msg2.as_view().optional_bytes_cord_cord_opt().is_some(), eq(true));
    msg2.as_mut().set_optional_bytes_cord_cord(b"mutated");
    assert_that!(msg2.as_mut().optional_bytes_cord_cord() == b"mutated", eq(true));
}

#[gtest]
fn test_into_proto_cord_inputs_and_methods() {
    use unittest_rust_proto::TestCord;

    let mut msg = TestCord::new();

    // Vec<u8> with spare capacity (exercises capacity-preserving releaser on C++).
    let mut vec_with_spare = Vec::with_capacity(256);
    vec_with_spare.extend_from_slice(b"spare-cap-payload");
    msg.set_optional_bytes_cord_cord(vec_with_spare);
    assert_that!(msg.optional_bytes_cord_cord() == b"spare-cap-payload", eq(true));

    // Empty Vec<u8> with non-zero capacity (must deallocate cleanly without leaking).
    let empty_with_cap: Vec<u8> = Vec::with_capacity(128);
    msg.set_optional_bytes_cord_cord(empty_with_cap);
    assert_that!(msg.optional_bytes_cord_cord().is_empty(), eq(true));
    assert_that!(msg.optional_bytes_cord_cord().chunks().count(), eq(0));

    // OwnedProtoCord::from_static, Box<[u8]>, Cow, ProtoBytes, ProtoBytesCow.
    let static_cord = OwnedProtoCord::from_static(b"static-bytes");
    msg.set_optional_bytes_cord_cord(&static_cord);
    assert_that!(msg.optional_bytes_cord_cord() == static_cord, eq(true));

    msg.set_optional_bytes_cord_cord(Box::<[u8]>::from(&b"boxed"[..]));
    assert_that!(msg.optional_bytes_cord_cord() == b"boxed", eq(true));

    msg.set_optional_bytes_cord_cord(Cow::Borrowed(&b"cow-borrowed"[..]));
    assert_that!(msg.optional_bytes_cord_cord() == b"cow-borrowed", eq(true));

    msg.set_optional_bytes_cord_cord(ProtoBytes::from(&b"proto-bytes"[..]));
    assert_that!(msg.optional_bytes_cord_cord() == b"proto-bytes", eq(true));

    let cord_view = msg.optional_bytes_cord_cord();
    let mut buf = vec![0u8; cord_view.len()];
    cord_view.copy_to_slice(&mut buf);
    assert_that!(buf.as_slice(), eq(b"proto-bytes"));
    assert_that!(cord_view.bytes().collect::<Vec<u8>>(), eq(b"proto-bytes"));
    assert_that!(cord_view.to_cow_bytes().as_ref(), eq(&b"proto-bytes"[..]));

    // Invalid UTF-8 handling on `to_utf8()`.
    msg.set_optional_bytes_cord_cord(&[0xff, 0xfe][..]);
    assert_that!(msg.optional_bytes_cord_cord().to_utf8().is_err(), eq(true));

    // Ordering & Debug.
    let a = ProtoCord::from_slice(b"abc");
    let b = ProtoCord::from_slice(b"abd");
    assert_that!(a < b, eq(true));
    assert_that!(format!("{a:?}"), eq("ProtoCord(\"abc\")"));
}

#[gtest]
fn test_serialize_to_cord_and_parse_from_cord() {
    use unittest_rust_proto::{TestCord, TestRequired};

    let mut msg = TestCord::new();
    let payload = vec![b'A'; 8192];
    msg.set_optional_bytes_cord_cord(payload.clone());

    let serialized_cord = msg.serialize_to_cord().unwrap();
    let serialized_det =
        msg.serialize_to_cord_with_options(SerializeOptions::new().deterministic(true)).unwrap();
    assert_that!(serialized_cord.len(), eq(msg.serialized_len()));
    assert_that!(serialized_cord == serialized_det, eq(true));
    assert_that!(serialized_cord.to_vec(), eq(&msg.serialize().unwrap()));

    // Parse from borrowed `&OwnedProtoCord`, `ProtoCord`, and owned `OwnedProtoCord`.
    let parsed_ref = TestCord::parse_from_cord(&serialized_cord).unwrap();
    assert_that!(parsed_ref.optional_bytes_cord_cord() == payload.as_slice(), eq(true));

    let parsed_view = TestCord::parse_from_cord(serialized_cord.as_view()).unwrap();
    assert_that!(parsed_view.optional_bytes_cord_cord() == payload.as_slice(), eq(true));

    let parsed_owned = TestCord::parse_from_cord(serialized_cord).unwrap();
    assert_that!(parsed_owned.optional_bytes_cord_cord() == payload.as_slice(), eq(true));

    // Malformed wire input clears existing fields and returns Err(ParseError).
    let mut target = parsed_owned;
    assert_that!(
        target.clear_and_parse_cord(&b"\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"[..]).is_err(),
        eq(true)
    );
    assert_that!(target.has_optional_bytes_cord(), eq(false));

    // Partial required-field enforcement parity with `parse_dont_enforce_required`:
    // craft partial wire data with only `a = 1` (tag 0x08, varint 0x01) while `b` and `c` are
    // unset.
    let partial_wire = OwnedProtoCord::from_static(&[0x08, 0x01]);
    assert_that!(TestRequired::parse_from_cord(&partial_wire).is_err(), eq(true));
    let partial_ok = TestRequired::parse_from_cord_dont_enforce_required(partial_wire).unwrap();
    assert_that!(partial_ok.a(), eq(1));
}
