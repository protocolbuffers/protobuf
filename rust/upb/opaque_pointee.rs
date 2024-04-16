// Macro to create structs that will act as opaque pointees. These structs are
// never intended to be dereferenced in Rust.
// This is a workaround until stabilization of [`extern type`].
// TODO: convert to extern type once stabilized.
// [`extern type`]: https://github.com/rust-lang/rust/issues/43467
macro_rules! opaque_pointee {
    ($name: ident) => {
        #[repr(C)]
        pub struct $name {
            _data: [u8; 0],
            _marker: std::marker::PhantomData<(*mut u8, std::marker::PhantomPinned)>,
        }
    };
}

pub(crate) use opaque_pointee;
