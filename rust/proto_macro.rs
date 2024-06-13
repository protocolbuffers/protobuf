// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/// proto! enables the use of Rust struct initialization syntax to create
/// protobuf messages. The macro does its best to try and detect the
/// initialization of submessages, but it is only able to do so while the
/// submessage is being defined as part of the original struct literal.
/// Introducing an expression using () or {} as the value of a field will
/// require another call to this macro in order to return a submessage
/// literal.``` /*
/// Given the following proto definition
/// message Data {
///     int32 number = 1;
///     string letters = 2;
///     Data nested = 3;
/// }
/// */
/// use protobuf::proto;
/// let message = proto!(Data {
///     number: 42,
///     letters: "Hello World",
///     nested: Data {
///         number: {
///             let x = 100;
///             x + 1
///         }
///     }
/// }); ```
#[macro_export]
macro_rules! proto {
    ($msgtype:ty { $($tt:tt)* }) => {
        $crate::proto_internal!($msgtype { $($tt)* })
    }
}

#[macro_export(local_inner_macros)]
#[doc(hidden)]
macro_rules! proto_internal {
    // nested message,
    (@msg $msg:ident $submsg:ident : $($msgtype:ident)::+ { $field:ident : $($value:tt)* }, $($rest:tt)*) => {
        proto_internal!(@msg $msg $submsg : $($msgtype)::+ { $field : $($value)* });
        proto_internal!(@msg $msg $($rest)*);
    };
    (@msg $msg:ident $submsg:ident : ::$($msgtype:ident)::+ { $field:ident : $($value:tt)* }, $($rest:tt)*) => {
        proto_internal!(@msg $msg $submsg : ::$($msgtype)::+ { $field : $($value)* });
        proto_internal!(@msg $msg $($rest)*);
    };

    // nested message
    (@msg $msg:ident $submsg:ident : $($msgtype:ident)::+ { $field:ident : $($value:tt)* }) => {
        {
            let mut $msg: <$($msgtype)::+ as $crate::MutProxied>::Mut<'_> = $crate::__internal::paste!($msg.[<$submsg _mut>]());
            proto_internal!(@msg $msg $field : $($value)*);
        }
    };
    (@msg $msg:ident $submsg:ident : ::$($msgtype:ident)::+ { $field:ident : $($value:tt)* }) => {
        {
            let mut $msg: <::$($msgtype)::+ as $crate::MutProxied>::Mut<'_> = $crate::__internal::paste!($msg.[<$submsg _mut>]());
            proto_internal!(@msg $msg $field : $($value)*);
        }
    };

    // empty nested message,
    (@msg $msg:ident $submsg:ident : $($msgtype:ident)::+ { }, $($rest:tt)*) => {
        proto_internal!(@msg $msg $submsg : $($msgtype)::+ { });
        proto_internal!(@msg $msg $($rest)*);
    };
    (@msg $msg:ident $submsg:ident : ::$($msgtype:ident)::+ { }, $($rest:tt)*) => {
        proto_internal!(@msg $msg $submsg : ::$($msgtype)::+ { });
        proto_internal!(@msg $msg $($rest)*);
    };

    // empty nested message
    (@msg $msg:ident $submsg:ident : $($msgtype:ident)::+ { }) => {
        {
            let mut $msg: <$($msgtype)::+ as $crate::MutProxied>::Mut<'_> = $crate::__internal::paste!($msg.[<$submsg _mut>]());
        }
    };
    (@msg $msg:ident $submsg:ident : ::$($msgtype:ident)::+ { }) => {
        {
            let mut $msg: <::$($msgtype)::+ as $crate::MutProxied>::Mut<'_> = $crate::__internal::paste!($msg.[<$submsg _mut>]());
        }
    };

    // field: expr,
    (@msg $msg:ident $ident:ident : $expr:expr, $($rest:tt)*) => {
        // delegate without ,
        proto_internal!(@msg $msg $ident : $expr);
        proto_internal!(@msg $msg $($rest)*);
    };

    // field: expr
    (@msg $msg:ident $ident:ident : $expr:expr) => {
        $crate::__internal::paste!($msg.[<set_ $ident>]($expr));
    };

    (@msg $msg:ident) => {};

    // entry point
    ($msgtype:ty { $($tt:tt)* }) => {
        {
            let mut message = <$msgtype>::new();
            proto_internal!(@msg message $($tt)*);
            message
        }
    };
}
