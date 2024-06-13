use crate::opaque_pointee::opaque_pointee;
use std::ptr::NonNull;

opaque_pointee!(upb_ExtensionRegistry);
pub type RawExtensionRegistry = NonNull<upb_ExtensionRegistry>;
