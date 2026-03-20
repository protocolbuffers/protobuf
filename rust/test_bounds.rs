pub trait EntityType {
    type Tag;
}
pub struct MessageTag;
// We want to blanket implement MessageType for all T with MessageTag
pub trait MessageType {}
impl<T: EntityType<Tag = MessageTag>> MessageType for T {}
fn main() {}
