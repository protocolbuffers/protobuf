use crate::opaque_pointee::opaque_pointee;
use std::ptr::NonNull;

opaque_pointee!(upb_MiniTable);
pub type RawMiniTable = NonNull<upb_MiniTable>;
