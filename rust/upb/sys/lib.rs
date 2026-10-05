pub mod base;
pub mod mem;
pub mod message;
pub mod mini_table;
pub mod opaque_pointee;
pub mod text;
pub mod wire;

#[cfg(not(bzl))]
#[allow(dead_code)] // TODO - Remove when the public print API is implemented.
pub mod reflection;

#[cfg(test)]
mod test_helpers;
