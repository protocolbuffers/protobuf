use googletest::prelude::*;
use protobuf::{Extension, Parse};
use unittest_rust_proto::{test_all_types, TestAllExtensions};

#[gtest]
fn test_get_message_extension() {
    // Create a TestAllExtensions message with optional_nested_message_extension (18) set.
    // field 18 (tag: 18<<3|2 = 146 -> 0x92, 0x01)
    // content: TestAllTypes.NestedMessage { bb: 42 }
    // NestedMessage.bb is field 1.
    // tag 1<<3|0 = 8.
    // value 42.

    let submsg_bytes = [0x08, 42];
    let mut bytes = Vec::new();
    // Tag 18 (0x92 0x01)
    bytes.push(0x92);
    bytes.push(0x01);
    // Length
    bytes.push(submsg_bytes.len() as u8);
    bytes.extend_from_slice(&submsg_bytes);

    let msg = TestAllExtensions::parse(&bytes).expect("parse failed");

    // We need an Extension object.
    let ext = Extension::<TestAllExtensions, test_all_types::NestedMessage>::new(18);

    let submsg = ext.get(msg.as_view());
    assert!(submsg.is_some(), "Extension should be present");
    let submsg = submsg.unwrap();
    assert_eq!(submsg.bb(), 42);
}

#[gtest]
fn test_nested_message_access() {
    let mut msg = test_all_types::NestedMessage::new();
    msg.set_bb(42);
    assert_eq!(msg.bb(), 42);
    let view = msg.as_view();
    assert_eq!(view.bb(), 42);
}

#[gtest]
fn test_get_message_extension_not_present() {
    let msg = TestAllExtensions::new();
    let ext = Extension::<TestAllExtensions, test_all_types::NestedMessage>::new(18);
    let submsg = ext.get(msg.as_view());
    assert!(submsg.is_none());
}

#[gtest]
fn test_has_set_extension() {
    let mut msg = TestAllExtensions::new();
    let ext = Extension::<TestAllExtensions, test_all_types::NestedMessage>::new(18);

    assert!(!ext.has(msg.as_view()));
    assert!(ext.get(msg.as_view()).is_none());

    let mut nested = test_all_types::NestedMessage::new();
    nested.set_bb(100);

    ext.set(msg.as_mut(), nested);

    assert!(ext.has(msg.as_view()));
    let fetched = ext.get(msg.as_view()).expect("Should have extension");
    assert_eq!(fetched.bb(), 100);

    // Update
    let mut nested2 = test_all_types::NestedMessage::new();
    nested2.set_bb(200);
    ext.set(msg.as_mut(), nested2);

    assert!(ext.has(msg.as_view()));
    let fetched2 = ext.get(msg.as_view()).expect("Should have extension");
    assert_eq!(fetched2.bb(), 200);
}
