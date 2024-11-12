use googletest::gtest;
use protobuf::{__internal, __runtime};

#[gtest]
#[allow(clippy::unit_cmp)]
fn test_no_internal_access() {
    // This test is to ensure that the `__internal` and `__runtime` mods are
    // 'blocked' by instead being a unit type instead of a module.
    assert_eq!(__internal, ());
    assert_eq!(__runtime, ());
}
