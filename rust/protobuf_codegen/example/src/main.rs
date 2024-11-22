#[path = "protos/foo.u.pb.rs"]
mod protos;

use protobuf::proto;

use protos::Foo;

fn main() {
    let foo = proto!(Foo { name: "foo", bar: __ { name: "bar" } });
    dbg!(foo);
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn set_strings() {
        let foo = proto!(Foo { name: "foo", bar: __ { name: "bar" } });

        assert_eq!(foo.name(), "foo");
        assert_eq!(foo.bar().name(), "bar");
    }

    #[test]
    fn set_ints() {
        let foo = proto!(Foo { int: 42, bar: __ { numbers: [1, 2, 3] } });

        assert_eq!(foo.int(), 42);
        let nums: Vec<_> = foo.bar().numbers().iter().collect();
        assert_eq!(nums, vec![1, 2, 3]);
    }
}
