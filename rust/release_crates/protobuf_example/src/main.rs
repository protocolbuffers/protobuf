// #[path = "protobuf_generated/foo.u.pb.rs"]
// mod protos;

include!(concat!(env!("OUT_DIR"), "/protobuf_generated/mod.rs"));

use protobuf::proto;

use foo::Foo;

fn main() {
    let foo = proto!(Foo { name: "foo" });
    dbg!(foo);
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test] // allow_core_test
    fn set_strings() {
        let foo = proto!(Foo { name: "foo" });
        assert_eq!(foo.name(), "foo");
    }

    #[test] // allow_core_test
    fn set_ints() {
        let foo = proto!(Foo { int: 42 });
        assert_eq!(foo.int(), 42);
    }
}
