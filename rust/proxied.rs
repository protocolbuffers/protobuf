// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Operating on borrowed data owned by a message is a central concept in
//! Protobuf (and Rust in general). The way this is normally accomplished in
//! Rust is to pass around references and operate on those. Unfortunately,
//! references come with two major drawbacks:
//!
//! * We must store the value somewhere in the memory to create a reference to
//!   it. The value must be readable by a single load. However for Protobuf
//!   fields it happens that the actual memory representation of a value differs
//!   from what users expect and it is an implementation detail that can change
//!   as more optimizations are implemented. For example, rarely accessed
//!   `int64` fields can be represented in a packed format with 32 bits for the
//!   value in the common case. Or, a single logical value can be spread across
//!   multiple memory locations. For example, presence information for all the
//!   fields in a protobuf message is centralized in a bitset.
//! * We cannot store extra data on the reference that might be necessary for
//!   correctly manipulating it (and custom-metadata DSTs do not exist yet in
//!   Rust). Concretely, messages, string, bytes, and repeated fields in UPB
//!   need to carry around an arena parameter separate from the data pointer to
//!   enable mutation (for example adding an element to a repeated field) or
//!   potentially to enable optimizations (for example referencing a string
//!   value using a Cord-like type instead of copying it if the source and
//!   target messages are on the same arena already). Mutable references to
//!   messages have one additional drawback: Rust allows users to
//!   indiscriminately run a bytewise swap() on mutable references, which could
//!   result in pointers to the wrong arena winding up on a message. For
//!   example, imagine swapping a submessage across two root messages allocated
//!   on distinct arenas A and B; after the swap, the message allocated in A may
//!   contain pointers from B by way of the submessage, because the swap does
//!   not know to fix up those pointers as needed. The C++ API uses
//!   message-owned arenas, and this ends up resembling self-referential types,
//!   which need `Pin` in order to be sound. However, `Pin` has much stronger
//!   guarantees than we need to uphold.
//!
//! These drawbacks put the "idiomatic Rust" goal in conflict with the
//! "performance", "evolvability", and "safety" goals. Given the project design
//! priorities we decided to not use plain Rust references. Instead, we
//! implemented the concept of "proxy" types. Proxy types are a reference-like
//! indirection between the user and the internal memory representation.

use crate::__internal::{Private, SealedInternal};
use std::fmt::Debug;

/// A type that can be accessed through a reference-like proxy.
///
/// An instance of a `Proxied` can be accessed immutably via `Proxied::View`.
///
/// All Protobuf field types implement `Proxied`.
pub trait Proxied: SealedInternal + AsView<Proxied = Self> + Sized {
    /// The proxy type that provides shared access to a `T`, like a `&'msg T`.
    ///
    /// Most code should use the type alias [`View`].
    type View<'msg>: ViewProxy<'msg, Proxied = Self>
    where
        Self: 'msg;
}

/// A type that can be be accessed through a reference-like proxy.
///
/// An instance of a `MutProxied` can be accessed mutably via `MutProxied::Mut`
/// and immutably via `MutProxied::View`.
///
/// `MutProxied` is implemented by message, map and repeated field types.
pub trait MutProxied: SealedInternal + Proxied + AsMut<MutProxied = Self> {
    /// The proxy type that provides exclusive mutable access to a `T`, like a
    /// `&'msg mut T`.
    ///
    /// Most code should use the type alias [`Mut`].
    type Mut<'msg>: MutProxy<'msg, MutProxied = Self>
    where
        Self: 'msg;
}

/// A proxy type that provides shared access to a `T`, like a `&'msg T`.
///
/// This is more concise than fully spelling the associated type.
#[allow(dead_code)]
pub type View<'msg, T> = <T as Proxied>::View<'msg>;

/// A proxy type that provides exclusive mutable access to a `T`, like a
/// `&'msg mut T`.
///
/// This is more concise than fully spelling the associated type.
#[allow(dead_code)]
pub type Mut<'msg, T> = <T as MutProxied>::Mut<'msg>;

/// Used to semantically do a cheap "to-reference" conversion. This is
/// implemented on both owned `Proxied` types as well as ViewProxy and MutProxy
/// types.
///
/// On ViewProxy this will behave as a reborrow into a shorter lifetime.
pub trait AsView: SealedInternal {
    type Proxied: Proxied;

    /// Converts a borrow into a `View` with the lifetime of that borrow.
    ///
    /// In non-generic code we don't need to use `as_view` because the proxy
    /// types are covariant over `'msg`. However, generic code conservatively
    /// treats `'msg` as [invariant], therefore we need to call
    /// `as_view` to explicitly perform the operation that in concrete code
    /// coercion would perform implicitly.
    ///
    /// For example, the call to `.as_view()` in the following snippet
    /// wouldn't be necessary in concrete code:
    /// ```ignore
    /// fn reborrow<'a, 'b, T>(x: &'b View<'a, T>) -> View<'b, T>
    /// where 'a: 'b, T: Proxied
    /// {
    ///   x.as_view()
    /// }
    /// ```
    ///
    /// [invariant]: https://doc.rust-lang.org/nomicon/subtyping.html#variance
    fn as_view(&self) -> View<'_, Self::Proxied>;
}

/// Used to turn another 'borrow' into a ViewProxy.
///
/// On a MutProxy this borrows to a View (semantically matching turning a `&mut
/// T` into a `&T`).
///
/// On a ViewProxy this will behave as a reborrow into a shorter lifetime
/// (semantically matching a `&'a T` into a `&'b T` where `'a: 'b`).
pub trait IntoView<'msg>: SealedInternal + AsView {
    /// Converts into a `View` with a potentially shorter lifetime.
    ///
    /// In non-generic code we don't need to use `into_view` because the proxy
    /// types are covariant over `'msg`. However, generic code conservatively
    /// treats `'msg` as [invariant], therefore we need to call
    /// `into_view` to explicitly perform the operation that in concrete
    /// code coercion would perform implicitly.
    ///
    /// ```ignore
    /// fn reborrow_generic_view_into_view<'a, 'b, T>(
    ///     x: View<'a, T>,
    ///     y: View<'b, T>,
    /// ) -> [View<'b, T>; 2]
    /// where
    ///     T: MutProxied,
    ///     'a: 'b,
    /// {
    ///     // `[x, y]` fails to compile because `'a` is not the same as `'b` and the `View`
    ///     // lifetime parameter is (conservatively) invariant.
    ///     // `[x.as_view(), y]` fails because that borrow cannot outlive `'b`.
    ///     [x.into_view(), y]
    /// }
    /// ```
    ///
    /// [invariant]: https://doc.rust-lang.org/nomicon/subtyping.html#variance
    fn into_view<'shorter>(self) -> View<'shorter, Self::Proxied>
    where
        'msg: 'shorter;
}

/// Used to semantically do a cheap "to-mut-reference" conversion. This is
/// implemented on both owned `Proxied` types as well as MutProxy types.
///
/// On MutProxy this will behave as a reborrow into a shorter lifetime.
pub trait AsMut: SealedInternal + AsView<Proxied = Self::MutProxied> {
    type MutProxied: MutProxied;

    /// Converts a borrow into a `Mut` with the lifetime of that borrow.
    fn as_mut(&mut self) -> Mut<'_, Self::MutProxied>;
}

/// Used to turn another 'borrow' into a MutProxy.
///
/// On a MutProxy this will behave as a reborrow into a shorter lifetime
/// (semantically matching a `&mut 'a T` into a `&mut 'b T` where `'a: 'b`).
pub trait IntoMut<'msg>: SealedInternal + AsMut {
    /// Converts into a `Mut` with a potentially shorter lifetime.
    ///
    /// In non-generic code we don't need to use `into_mut` because the proxy
    /// types are covariant over `'msg`. However, generic code conservatively
    /// treats `'msg` as [invariant], therefore we need to call
    /// `into_mut` to explicitly perform the operation that in concrete code
    /// coercion would perform implicitly.
    ///
    /// ```ignore
    /// fn reborrow_generic_mut_into_mut<'a, 'b, T>(x: Mut<'a, T>, y: Mut<'b, T>) -> [Mut<'b, T>; 2]
    /// where
    ///     T: Proxied,
    ///     'a: 'b,
    /// {
    ///     // `[x, y]` fails to compile because `'a` is not the same as `'b` and the `Mut`
    ///     // lifetime parameter is (conservatively) invariant.
    ///     // `[x.as_mut(), y]` fails because that borrow cannot outlive `'b`.
    ///     [x.into_mut(), y]
    /// }
    /// ```
    ///
    /// [invariant]: https://doc.rust-lang.org/nomicon/subtyping.html#variance
    fn into_mut<'shorter>(self) -> Mut<'shorter, Self::MutProxied>
    where
        'msg: 'shorter;
}

/// Declares conversion operations common to all proxies (both views and mut
/// proxies).
///
/// This trait is intentionally made non-object-safe to prevent a potential
/// future incompatible change.
pub trait Proxy<'msg>:
    SealedInternal + 'msg + IntoView<'msg> + Sync + Unpin + Sized + Debug
{
}

/// Declares conversion operations common to view proxies.
pub trait ViewProxy<'msg>: SealedInternal + Proxy<'msg> + Send {}

/// Declares operations common to all mut proxies.
///
/// This trait is intentionally made non-object-safe to prevent a potential
/// future incompatible change.
pub trait MutProxy<'msg>: SealedInternal + Proxy<'msg> + AsMut + IntoMut<'msg> {
    /// Gets an immutable view of this field. This is shorthand for `as_view`.
    ///
    /// This provides a shorter lifetime than `into_view` but can also be called
    /// multiple times - if the result of `get` is not living long enough
    /// for your use, use that instead.
    fn get(&self) -> View<'_, Self::Proxied> {
        self.as_view()
    }
}

/// A value to `Proxied`-value conversion that consumes the input value.
///
/// All setter functions accept types that implement `IntoProxied`. The purpose
/// of `IntoProxied` is to allow setting arbitrary values on Protobuf fields
/// with the minimal number of copies.
///
/// This trait must not be implemented on types outside the Protobuf codegen and
/// runtime. We expect it to change in backwards incompatible ways in the
/// future.
pub trait IntoProxied<T: Proxied> {
    #[doc(hidden)]
    fn into_proxied(self, _private: Private) -> T;
}

impl<T: Proxied> IntoProxied<T> for T {
    fn into_proxied(self, _private: Private) -> T {
        self
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::prelude::*;

    #[derive(Debug, Default, PartialEq)]
    struct MyProxied {
        val: String,
    }

    impl MyProxied {
        fn as_view(&self) -> View<'_, Self> {
            MyProxiedView { my_proxied_ref: self }
        }

        fn as_mut(&mut self) -> Mut<'_, Self> {
            MyProxiedMut { my_proxied_ref: self }
        }
    }

    impl SealedInternal for MyProxied {}

    impl Proxied for MyProxied {
        type View<'msg> = MyProxiedView<'msg>;
    }

    impl AsView for MyProxied {
        type Proxied = Self;
        fn as_view(&self) -> MyProxiedView<'_> {
            self.as_view()
        }
    }

    impl MutProxied for MyProxied {
        type Mut<'msg> = MyProxiedMut<'msg>;
    }

    impl AsMut for MyProxied {
        type MutProxied = Self;
        fn as_mut(&mut self) -> MyProxiedMut<'_> {
            self.as_mut()
        }
    }

    #[derive(Debug, Clone, Copy)]
    struct MyProxiedView<'msg> {
        my_proxied_ref: &'msg MyProxied,
    }

    impl<'msg> SealedInternal for MyProxiedView<'msg> {}

    impl MyProxiedView<'_> {
        fn val(&self) -> &str {
            &self.my_proxied_ref.val
        }
    }

    impl<'msg> Proxy<'msg> for MyProxiedView<'msg> {}

    impl<'msg> ViewProxy<'msg> for MyProxiedView<'msg> {}

    impl<'msg> AsView for MyProxiedView<'msg> {
        type Proxied = MyProxied;

        fn as_view(&self) -> MyProxiedView<'msg> {
            *self
        }
    }

    impl<'msg> IntoView<'msg> for MyProxiedView<'msg> {
        fn into_view<'shorter>(self) -> MyProxiedView<'shorter>
        where
            'msg: 'shorter,
        {
            self
        }
    }

    #[derive(Debug)]
    struct MyProxiedMut<'msg> {
        my_proxied_ref: &'msg mut MyProxied,
    }

    impl<'msg> SealedInternal for MyProxiedMut<'msg> {}

    impl<'msg> Proxy<'msg> for MyProxiedMut<'msg> {}

    impl<'msg> AsView for MyProxiedMut<'msg> {
        type Proxied = MyProxied;

        fn as_view(&self) -> MyProxiedView<'_> {
            MyProxiedView { my_proxied_ref: self.my_proxied_ref }
        }
    }

    impl<'msg> IntoView<'msg> for MyProxiedMut<'msg> {
        fn into_view<'shorter>(self) -> View<'shorter, MyProxied>
        where
            'msg: 'shorter,
        {
            MyProxiedView { my_proxied_ref: self.my_proxied_ref }
        }
    }

    impl<'msg> AsMut for MyProxiedMut<'msg> {
        type MutProxied = MyProxied;

        fn as_mut(&mut self) -> MyProxiedMut<'_> {
            MyProxiedMut { my_proxied_ref: self.my_proxied_ref }
        }
    }

    impl<'msg> IntoMut<'msg> for MyProxiedMut<'msg> {
        fn into_mut<'shorter>(self) -> MyProxiedMut<'shorter>
        where
            'msg: 'shorter,
        {
            self
        }
    }

    impl<'msg> MutProxy<'msg> for MyProxiedMut<'msg> {}

    #[gtest]
    fn test_as_view() {
        let my_proxied = MyProxied { val: "Hello World".to_string() };

        let my_view = my_proxied.as_view();

        assert_that!(my_view.val(), eq(&my_proxied.val));
    }

    fn reborrow_mut_into_view<'msg>(x: Mut<'msg, MyProxied>) -> View<'msg, MyProxied> {
        // x.as_view() fails to compile with:
        //   `ERROR: attempt to return function-local borrowed content`
        x.into_view() // OK: we return the same lifetime as we got in.
    }

    #[gtest]
    fn test_mut_into_view() {
        let mut my_proxied = MyProxied { val: "Hello World".to_string() };
        reborrow_mut_into_view(my_proxied.as_mut());
    }

    fn require_unified_lifetimes<'msg>(_x: Mut<'msg, MyProxied>, _y: View<'msg, MyProxied>) {}

    #[gtest]
    fn test_require_unified_lifetimes() {
        let mut my_proxied = MyProxied { val: "Hello1".to_string() };
        let my_mut = my_proxied.as_mut();

        {
            let other_proxied = MyProxied { val: "Hello2".to_string() };
            let other_view = other_proxied.as_view();
            require_unified_lifetimes(my_mut, other_view);
        }
    }

    fn reborrow_generic_as_view<'a, 'b, T>(
        x: &'b mut Mut<'a, T>,
        y: &'b View<'a, T>,
    ) -> [View<'b, T>; 2]
    where
        T: MutProxied,
        'a: 'b,
    {
        // `[x, y]` fails to compile because `'a` is not the same as `'b` and the `View`
        // lifetime parameter is (conservatively) invariant.
        [x.as_view(), y.as_view()]
    }

    #[gtest]
    fn test_reborrow_generic_as_view() {
        let mut my_proxied = MyProxied { val: "Hello1".to_string() };
        let mut my_mut = my_proxied.as_mut();
        let my_ref = &mut my_mut;

        {
            let other_proxied = MyProxied { val: "Hello2".to_string() };
            let other_view = other_proxied.as_view();
            reborrow_generic_as_view::<MyProxied>(my_ref, &other_view);
        }
    }

    fn reborrow_generic_view_into_view<'a, 'b, T>(
        x: View<'a, T>,
        y: View<'b, T>,
    ) -> [View<'b, T>; 2]
    where
        T: Proxied,
        'a: 'b,
    {
        // `[x, y]` fails to compile because `'a` is not the same as `'b` and the `View`
        // lifetime parameter is (conservatively) invariant.
        // `[x.as_view(), y]` fails because that borrow cannot outlive `'b`.
        [x.into_view(), y]
    }

    #[gtest]
    fn test_reborrow_generic_into_view() {
        let my_proxied = MyProxied { val: "Hello1".to_string() };
        let my_view = my_proxied.as_view();

        {
            let other_proxied = MyProxied { val: "Hello2".to_string() };
            let other_view = other_proxied.as_view();
            reborrow_generic_view_into_view::<MyProxied>(my_view, other_view);
        }
    }

    fn reborrow_generic_mut_into_view<'a, 'b, T>(x: Mut<'a, T>, y: View<'b, T>) -> [View<'b, T>; 2]
    where
        T: MutProxied,
        'a: 'b,
    {
        [x.into_view(), y]
    }

    #[gtest]
    fn test_reborrow_generic_mut_into_view() {
        let mut my_proxied = MyProxied { val: "Hello1".to_string() };
        let my_mut = my_proxied.as_mut();

        {
            let other_proxied = MyProxied { val: "Hello2".to_string() };
            let other_view = other_proxied.as_view();
            reborrow_generic_mut_into_view::<MyProxied>(my_mut, other_view);
        }
    }

    fn reborrow_generic_mut_into_mut<'a, 'b, T>(x: Mut<'a, T>, y: Mut<'b, T>) -> [Mut<'b, T>; 2]
    where
        T: MutProxied,
        'a: 'b,
    {
        // `[x, y]` fails to compile because `'a` is not the same as `'b` and the `Mut`
        // lifetime parameter is (conservatively) invariant.
        // `[x.as_mut(), y]` fails because that borrow cannot outlive `'b`.
        let tmp: Mut<'b, T> = x.into_mut();
        [tmp, y]
    }

    #[gtest]
    fn test_reborrow_generic_mut_into_mut() {
        let mut my_proxied = MyProxied { val: "Hello1".to_string() };
        let my_mut = my_proxied.as_mut();

        {
            let mut other_proxied = MyProxied { val: "Hello2".to_string() };
            let other_mut = other_proxied.as_mut();
            // No need to reborrow, even though lifetime of &other_view is different
            // than the lifetiem of my_ref. Rust references are covariant over their
            // lifetime.
            reborrow_generic_mut_into_mut::<MyProxied>(my_mut, other_mut);
        }
    }
}
