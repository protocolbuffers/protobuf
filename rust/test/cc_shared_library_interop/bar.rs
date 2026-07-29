// Dummy comment 2
use foo_rust_proto::Foo;

#[unsafe(no_mangle)]
pub extern "C" fn bar() {
    let mut foo = Foo::new();
    foo.set_text("Hello from Rust!".to_string());
    foo.set_value(42);
    println!("Value: {:?}", foo);
}
