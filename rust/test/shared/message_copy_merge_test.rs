use googletest::prelude::*;
use protobuf::prelude::*;
use unittest_rust_proto::{NestedTestAllTypes, TestAllTypes};

#[gtest]
fn copy_from() {
    let mut dst = TestAllTypes::new();
    let mut src = TestAllTypes::new();
    dst.copy_from(src.as_view());
    assert_that!(dst.has_optional_int32(), eq(false));

    src.set_optional_int32(42);
    assert_that!(src.has_optional_int32(), eq(true));
    assert_that!(dst.has_optional_int32(), eq(false)); // Not aliased.

    dst.copy_from(src);
    assert_that!(dst.has_optional_int32(), eq(true));
}

#[gtest]
fn take_from() {
    let mut dst = TestAllTypes::new();
    let mut src = TestAllTypes::new();
    src.set_optional_int32(42);
    src.optional_nested_message_mut().set_bb(10);

    dst.take_from(src.as_mut());
    assert_that!(src.has_optional_int32(), eq(false)); // Take_from clears the original message.
    assert_that!(src.optional_nested_message().bb(), eq(0));

    assert_that!(dst.has_optional_int32(), eq(true));
    assert_that!(dst.optional_nested_message().bb(), eq(10));

    dst.take_from(src); // Consuming take_from of an empty thing.
    assert_that!(dst.has_optional_int32(), eq(false));
}

#[gtest]
fn take_from_submsg() {
    let mut dst = NestedTestAllTypes::new();
    let mut src = NestedTestAllTypes::new();
    src.child_mut().child_mut().payload_mut().set_optional_int32(42);

    dst.child_mut().take_from(src.child_mut());

    // The src's submessage is empty but its presence on parent is not affected.
    assert_that!(src.has_child(), eq(true));
    assert_that!(src.child().has_child(), eq(false));

    assert_that!(dst.child().child().payload().optional_int32(), eq(42));
}

#[gtest]
fn merge_from_empty() {
    let mut dst = TestAllTypes::new();
    let src = TestAllTypes::new();
    dst.merge_from(src.as_view());
    assert_that!(dst.has_optional_int32(), eq(false));
}

#[gtest]
fn merge_from_non_empty() {
    let mut dst = TestAllTypes::new();
    let src = proto!(TestAllTypes { optional_int32: 42 });
    dst.as_mut().merge_from(src.as_view());
    assert_eq!(dst.optional_int32(), 42);
}

#[gtest]
fn merge_repeated_empty() {
    let mut dst = TestAllTypes::new();
    let mut src = TestAllTypes::new();
    src.repeated_int32_mut().extend(0..5);
    dst.merge_from(src.as_view());
    assert_that!(
        &dst.repeated_int32().iter().collect::<Vec<_>>(),
        eq(&src.repeated_int32().iter().collect::<Vec<_>>())
    );
}

#[gtest]
fn merge_repeated_non_empty() {
    let mut dst = TestAllTypes::new();
    let mut src = TestAllTypes::new();
    dst.repeated_int32_mut().extend(0..5);
    src.repeated_int32_mut().extend(5..10);
    dst.merge_from(src.as_view());
    assert_that!(
        &dst.repeated_int32().iter().collect::<Vec<_>>(),
        eq(&(0..10).collect::<Vec<_>>())
    );
}

#[gtest]
fn merge_from_sub_message() {
    let mut dst = NestedTestAllTypes::new();
    let src = proto!(NestedTestAllTypes {
        child: NestedTestAllTypes { payload: TestAllTypes { optional_int32: 42 } }
    });
    dst.merge_from(src.as_view());
    assert_that!(dst.child().payload().optional_int32(), eq(42));
}
