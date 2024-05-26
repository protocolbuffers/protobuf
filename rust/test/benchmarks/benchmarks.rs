// All benchmarks that don't need to be duplicated for cpp and upb should be
// defined here.

use std::sync::OnceLock;

#[no_mangle]
pub extern "C" fn extend_100_ints_vec_rs_bench() {
    let data: Vec<i32> = std::hint::black_box((0..100).collect());
    assert!(data.len() == 100);
}

static ONE_THOUSAND: OnceLock<Vec<i32>> = OnceLock::new();
#[no_mangle]
pub extern "C" fn sum_1000_ints_vec_rs_bench() {
    let source = ONE_THOUSAND.get_or_init(|| (0..1000).collect());
    let data: i32 = source.iter().sum();
    assert!(data == 499500);
}

extern "C" {
    fn benchmark_thunk_set_num2(proto: *mut std::ffi::c_void, num: i32);
    fn benchmark_thunk_add_num(proto: *mut std::ffi::c_void, num: i32);
}

/// # Safety
/// This function is safe because it just passes the arguments to the C
/// function.
#[no_mangle]
pub unsafe extern "C" fn benchmark_thunk_set_num2_rs(proto: *mut std::ffi::c_void, num: i32) {
    benchmark_thunk_set_num2(proto, num);
}

/// # Safety
/// This function is safe because it just passes the arguments to the C
/// function.
#[no_mangle]
pub unsafe extern "C" fn benchmark_thunk_add_num_rs(proto: *mut std::ffi::c_void, num: i32) {
    benchmark_thunk_add_num(proto, num);
}
