use debug_rust_proto::DebugMsg;
use googletest::prelude::*;

#[cfg(not(lite_runtime))]
#[test]
fn test_debug() {
    let mut msg = DebugMsg::new();
    msg.set_id(1);
    msg.set_secret_user_data("password");

    assert_that!(format!("{msg:?}"), contains_substring("id: 1"));
    assert_that!(format!("{msg:?}"), not(contains_substring("password")));
}

#[cfg(lite_runtime)]
#[test]
fn test_debug_lite() {
    let mut msg = DebugMsg::new();
    msg.set_id(1);
    msg.set_secret_user_data("password");

    assert_that!(format!("{msg:?}"), contains_substring("MessageLite"));
    assert_that!(format!("{msg:?}"), not(contains_substring("password")));
}
