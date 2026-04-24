use treeshaking_rust_proto::UsedMessage;

fn main() {
    let mut msg = UsedMessage::new();
    msg.set_id(42);
    println!("Used ID: {}", msg.id());
}
