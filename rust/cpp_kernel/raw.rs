use super::*;

/// Defines a set of opaque, unique, non-accessible pointees.
///
/// The [Rustonomicon][nomicon] currently recommends a zero-sized struct,
/// though this should use [`extern type`] when that is stabilized.
/// [nomicon]: https://doc.rust-lang.org/nomicon/ffi.html#representing-opaque-structs
/// [`extern type`]: https://github.com/rust-lang/rust/issues/43467
#[doc(hidden)]
pub mod _opaque_pointees {
    /// Opaque pointee for [`RawMessage`]
    ///
    /// This type is not meant to be dereferenced in Rust code.
    /// It is only meant to provide type safety for raw pointers
    /// which are manipulated behind FFI.
    ///
    /// [`RawMessage`]: super::RawMessage
    #[repr(C)]
    pub struct RawMessageData {
        _data: [u8; 0],
        _marker: std::marker::PhantomData<(*mut u8, ::std::marker::PhantomPinned)>,
    }

    /// Opaque pointee for [`RawRepeatedField`]
    ///
    /// This type is not meant to be dereferenced in Rust code.
    /// It is only meant to provide type safety for raw pointers
    /// which are manipulated behind FFI.
    #[repr(C)]
    pub struct RawRepeatedFieldData {
        _data: [u8; 0],
        _marker: std::marker::PhantomData<(*mut u8, ::std::marker::PhantomPinned)>,
    }

    /// Opaque pointee for [`RawMap`]
    ///
    /// This type is not meant to be dereferenced in Rust code.
    /// It is only meant to provide type safety for raw pointers
    /// which are manipulated behind FFI.
    #[repr(C)]
    pub struct RawMapData {
        _data: [u8; 0],
        _marker: std::marker::PhantomData<(*mut u8, ::std::marker::PhantomPinned)>,
    }

    /// Opaque pointee for [`CppStdString`]
    ///
    /// This type is not meant to be dereferenced in Rust code.
    /// It is only meant to provide type safety for raw pointers
    /// which are manipulated behind FFI.
    #[repr(C)]
    pub struct CppStdStringData {
        _data: [u8; 0],
        _marker: std::marker::PhantomData<(*mut u8, ::std::marker::PhantomPinned)>,
    }
}

/// A raw pointer to the underlying message for this runtime.
#[doc(hidden)]
pub type RawMessage = NonNull<_opaque_pointees::RawMessageData>;

/// A raw pointer to the underlying repeated field container for this runtime.
#[doc(hidden)]
pub type RawRepeatedField = NonNull<_opaque_pointees::RawRepeatedFieldData>;

/// A raw pointer to the underlying arena for this runtime.
#[doc(hidden)]
pub type RawMap = NonNull<_opaque_pointees::RawMapData>;

/// A raw pointer to a std::string.
#[doc(hidden)]
pub type CppStdString = NonNull<_opaque_pointees::CppStdStringData>;

pub type RawMapIter = UntypedMapIterator;
