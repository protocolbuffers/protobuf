pub mod base;
pub mod mem;
pub mod message;
pub mod mini_table;
pub mod opaque_pointee;
pub mod text;
pub mod wire;

#[cfg(not(bzl))]
pub mod reflection;

#[cfg(test)]
mod test_helpers;
