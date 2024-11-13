use googletest::prelude::*;
use protobuf::prelude::*;
use unittest_rust_proto::{NestedTestAllTypes, TestAllTypes};

#[gtest]
fn merge_from_empty() {
    let mut dst = TestAllTypes::new();
    let src = TestAllTypes::new();
    dst.merge_from(src.as_view());
    assert_that!(dst.as_view().has_optional_int32(), eq(false));
}

#[gtest]
fn merge_from_non_empty() {
    let mut dst = TestAllTypes::new();
    let src = proto!(TestAllTypes { optional_int32: 42 });
    dst.as_mut().merge_from(src.as_view());
    assert_eq!(dst.as_view().optional_int32(), 42);
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
    assert_that!(dst.as_view().child().payload().optional_int32(), eq(42));
}
