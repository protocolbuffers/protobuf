// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use protobuf::proto;

mod protos {
    include!(concat!(env!("OUT_DIR"), "/protobuf_generated/proto_example/generated.rs"));
}
use protos::Foo;

fn main() {
    let foo = proto!(Foo { name: "foo", bar: __ { name: "bar" } });
    dbg!(foo);
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test] // allow_core_test
    fn set_strings() {
        let foo = proto!(Foo { name: "foo", bar: __ { name: "bar" } });

        assert_eq!(foo.name(), "foo");
        assert_eq!(foo.bar().name(), "bar");
    }

    #[test] // allow_core_test
    fn set_ints() {
        let foo = proto!(Foo { int: 42, bar: __ { numbers: [1, 2, 3] } });

        assert_eq!(foo.int(), 42);
        let nums: Vec<_> = foo.bar().numbers().iter().collect();
        assert_eq!(nums, vec![1, 2, 3]);
    }

    #[test] // allow_core_test
    fn set_well_known_type() {
        let foo = proto!(Foo { timestamp: __ { seconds: 99, nanos: 13 } });

        assert_eq!(foo.timestamp().seconds(), 99);
        assert_eq!(foo.timestamp().nanos(), 13);
    }
}
