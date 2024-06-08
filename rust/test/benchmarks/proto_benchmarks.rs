// All benchmarks are exported as C functions so that they can be called from
// the C++. The benchmarks are exported with the name of the benchmark
// followed by the name of the kernel and _bench.

use bench_data_rust_proto::BenchData;
use paste::paste;
use std::sync::OnceLock;

macro_rules! export_proto_bench {
    ($name:ident, $protokernel:ident) => {
        paste! {
            #[no_mangle]
            pub extern "C" fn [<$name _rs_ $protokernel _bench>] () {
                $name();
            }
        }
    };
}

macro_rules! export_benches {
    ($name:ident, $($rest:tt)*) => {
        #[cfg(bench_upb)]
        export_proto_bench!($name, upb);
        #[cfg(bench_cpp)]
        export_proto_bench!($name, cpp);

        export_benches!($($rest)*);
    };
    ($name:ident) => {
        #[cfg(bench_upb)]
        export_proto_bench!($name, upb);
        #[cfg(bench_cpp)]
        export_proto_bench!($name, cpp);
    };
}

export_benches!(
    set_string,
    set_int,
    add_10_repeated_msg,
    copy_from_10_repeated_msg,
    extend_10_repeated_msg,
    add_100_ints,
    copy_from_100_ints,
    extend_100_ints,
    sum_1000_ints
);

pub fn set_string() {
    let mut data = BenchData::new();
    data.set_name("a relatively long string that will avoid any short string optimizations.");
}

pub fn set_int() {
    let mut data = BenchData::new();
    data.set_num2(123456789);
    assert_eq!(data.num2(), 123456789);
}

pub fn add_10_repeated_msg() {
    let mut data = BenchData::new();
    let mut repeated = data.subs_mut();
    for i in 0..10 {
        let mut new = BenchData::new();
        new.set_num2(i);
        repeated.push(new.as_view());
    }
}

static COPY_FROM_10: OnceLock<BenchData> = OnceLock::new();

fn create_10_repeated() -> BenchData {
    let mut data = BenchData::new();
    let mut repeated = data.subs_mut();
    for i in 0..10 {
        let mut new = BenchData::new();
        new.set_num2(i);
        repeated.push(new.as_view());
    }
    data
}

pub fn copy_from_10_repeated_msg() {
    let source = COPY_FROM_10.get_or_init(create_10_repeated);

    let mut data = BenchData::new();
    data.subs_mut().copy_from(source.subs());
}

pub fn extend_10_repeated_msg() {
    let source = COPY_FROM_10.get_or_init(create_10_repeated);
    let mut data = BenchData::new();
    data.subs_mut().extend(source.subs());
}

pub fn add_100_ints() {
    let mut data = BenchData::new();
    let mut repeated = data.nums_mut();
    for i in 0..100 {
        repeated.push(i);
    }
}

fn create_100_ints() -> BenchData {
    let mut data = BenchData::new();
    data.nums_mut().extend(0..100);
    data
}

static COPY_FROM_100: OnceLock<BenchData> = OnceLock::new();

pub fn copy_from_100_ints() {
    let source = COPY_FROM_100.get_or_init(create_100_ints);

    let mut data = BenchData::new();
    data.nums_mut().copy_from(source.nums());
}

pub fn extend_100_ints() {
    let mut data = BenchData::new();
    let mut repeated = data.nums_mut();
    repeated.extend(0..100);
    assert!(repeated.len() == 100);
}

static ONE_THOUSAND: OnceLock<BenchData> = OnceLock::new();
pub fn sum_1000_ints() {
    let source = ONE_THOUSAND.get_or_init(|| {
        let mut data = BenchData::new();
        data.nums_mut().extend(0..1000);
        data
    });

    let sum: i32 = source.nums().iter().sum();
    assert!(sum == 499500);
}
