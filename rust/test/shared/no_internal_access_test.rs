use googletest::gtest;
use protobuf::__internal;

#[gtest]
#[allow(clippy::unit_cmp)]
fn test_no_internal_access() {
    // This test is to ensure that the `__internal` is 'blocked' by instead being a
    // unit type instead of a module.
    assert_eq!(__internal, ());
}
