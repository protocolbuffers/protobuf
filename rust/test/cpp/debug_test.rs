use debug_rust_proto::DebugMsg;
use googletest::prelude::*;
use optimize_for_lite_rust_proto::OptimizeForLiteTestMessage;

#[cfg(not(lite_runtime))]
#[gtest]
fn test_debug() {
    let mut msg = DebugMsg::new();
    msg.set_id(1);
    msg.set_secret_user_data("password");

    assert_that!(format!("{msg:?}"), contains_substring("id: 1"));
}

#[cfg(lite_runtime)]
#[gtest]
fn test_debug_lite() {
    let mut msg = DebugMsg::new();
    msg.set_id(1);
    msg.set_secret_user_data("password");

    assert_that!(format!("{msg:?}"), contains_substring("MessageLite"));
}

/// A message with the option set to optimize for lite will behave as a lite
/// message regardless of the `lite_runtime` feature. Run this test not guarded
/// by the cfg(lite_runtime) and ensure it functions as lite.
#[gtest]
fn test_optimize_for_lite_option() {
    let mut msg = OptimizeForLiteTestMessage::new();
    msg.set_value("password");
    assert_that!(format!("{msg:?}"), contains_substring("MessageLite"));
}
