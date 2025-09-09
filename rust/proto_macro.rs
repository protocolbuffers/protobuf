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
/// literal.
///
/// ```rust,ignore
/// /*
/// Given the following proto definition:
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
/// });
/// ```
#[macro_export]
macro_rules! proto {
    ($msgtype:ty { $($tt:tt)* }) => {
        $crate::proto_internal!($msgtype { $($tt)* })
    }
}

#[macro_export(local_inner_macros)]
#[doc(hidden)]
macro_rules! proto_internal {
    // @merge rules are used to find a trailing ..expr on the message and call merge_from on it
    // before the fields of the message are set.
    (@merge $msg:ident $ident:ident : $expr:expr, $($rest:tt)*) => {
        proto_internal!(@merge $msg $($rest)*);
    };

    (@merge $msg:ident $ident:ident : $expr:expr) => {
    };

    (@merge $msg:ident ..$expr:expr) => {
        $crate::MergeFrom::merge_from(&mut $msg, $expr);
    };

    // @msg rules are used to set the fields of the message. There is a lot of duplication here
    // because we need to parse the message type using a :: separated list of identifiers.
    // nested message and trailing fields
    (@msg $msg:ident $submsg:ident : $($msgtype:ident)::+ { $($value:tt)* }, $($rest:tt)*) => {
        proto_internal!(@msg $msg $submsg : $($msgtype)::+ { $($value)* });
        proto_internal!(@msg $msg $($rest)*);
    };
    // nested message with leading :: on type and trailing fields
    (@msg $msg:ident $submsg:ident : ::$($msgtype:ident)::+ { $($value:tt)* }, $($rest:tt)*) => {
        proto_internal!(@msg $msg $submsg : ::$($msgtype)::+ { $($value)* });
        proto_internal!(@msg $msg $($rest)*);
    };

    // nested message using __
    (@msg $msg:ident $submsg:ident : __ { $($value:tt)* }) => {
        {
            let mut $msg = $crate::__internal::paste!($msg.[<$submsg _mut>]());
            proto_internal!(@merge $msg $($value)*);
            proto_internal!(@msg $msg $($value)*);
        }
    };
    // nested message
    (@msg $msg:ident $submsg:ident : $($msgtype:ident)::+ { $($value:tt)* }) => {
        {
            let mut $msg: <$($msgtype)::+ as $crate::MutProxied>::Mut<'_> = $crate::__internal::paste!($msg.[<$submsg _mut>]());
            proto_internal!(@merge $msg $($value)*);
            proto_internal!(@msg $msg $($value)*);
        }
    };
    // nested message with leading ::
    (@msg $msg:ident $submsg:ident : ::$($msgtype:ident)::+ { $($value:tt)* }) => {
        {
            let mut $msg: <::$($msgtype)::+ as $crate::MutProxied>::Mut<'_> = $crate::__internal::paste!($msg.[<$submsg _mut>]());
            proto_internal!(@merge $msg $($value)*);
            proto_internal!(@msg $msg $($value)*);
        }
    };

    // field with array literal and trailing fields
    (@msg $msg:ident $ident:ident : [$($elems:tt)*], $($rest:tt)*) => {
        proto_internal!(@msg $msg $ident : [$($elems)*]);
        proto_internal!(@msg $msg $($rest)*);
    };
    // field with array literal, calls out to @array to look for nested messages
    (@msg $msg:ident $ident:ident : [$($elems:tt)*]) => {
        {
            let _repeated = $msg.$ident();
            let elems = proto_internal!(@array $msg _repeated [] $($elems)*);
            $crate::__internal::paste!($msg.[<set_ $ident>](elems.into_iter()));
        }
    };

    // @array searches through an array literal for nested messages.
    // If a message is found then we recursively call the macro on it to set the fields.
    // This will create an array literal of owned messages to be used while setting the field.
    // For primitive types they should just be passed through as an $expr.
    // The array literal is constructed recursively, so the [] case has to be handled separately so
    // that we can properly insert commas. This leads to a lot of duplication.

    // Message nested in array literal with trailing array items
    (@array $msg:ident $repeated:ident [$($vals:expr),+] __ { $($value:tt)* }, $($rest:tt)*) => {
        proto_internal!(@array $msg $repeated [
                $($vals),+ ,
                {
                    let mut $msg = $crate::__internal::get_repeated_default_value($crate::__internal::Private, $repeated);
                    proto_internal!(@merge $msg $($value)*);
                    proto_internal!(@msg $msg $($value)*);
                    $msg
                }
            ] $($rest)*)
    };

    // Message nested in [] literal
    (@array $msg:ident $repeated:ident [$($vals:expr),+] __ { $($value:tt)* }) => {
        [
            $($vals),+ ,
            {
                let mut $msg = $crate::__internal::get_repeated_default_value($crate::__internal::Private, $repeated);
                proto_internal!(@merge $msg $($value)*);
                proto_internal!(@msg $msg $($value)*);
                $msg
            }
        ]
    };

    // Message nested in array literal with trailing array items
    (@array $msg:ident $repeated:ident [] __ { $($value:tt)* }, $($rest:tt)*) => {
        proto_internal!(@array $msg $repeated [
                {
                    let mut $msg = $crate::__internal::get_repeated_default_value($crate::__internal::Private, $repeated);
                    proto_internal!(@merge $msg $($value)*);
                    proto_internal!(@msg $msg $($value)*);
                    $msg
                }
            ] $($rest)*)
    };

    // Message nested in [] literal
    (@array $msg:ident $repeated:ident [] __ { $($value:tt)* }) => {
        [
            {
                let mut $msg = $crate::__internal::get_repeated_default_value($crate::__internal::Private, $repeated);
                proto_internal!(@merge $msg $($value)*);
                proto_internal!(@msg $msg $($value)*);
                $msg
            }
        ]
    };

    // End of __ repeated, now we need to handle named types

    // Message nested in array literal with trailing array items
    (@array $msg:ident $repeated:ident [$($vals:expr),+] $($msgtype:ident)::+ { $($value:tt)* }, $($rest:tt)*) => {
        proto_internal!(@array $msg $repeated [
                $($vals),+ ,
                {
                    let mut $msg = $($msgtype)::+::new();
                    proto_internal!(@merge $msg $($value)*);
                    proto_internal!(@msg $msg $($value)*);
                    $msg
                }
            ] $($rest)*)
    };
    // Message nested in [] literal with leading :: on type and trailing array items
    (@array $msg:ident $repeated:ident [$($vals:expr),+] ::$($msgtype:ident)::+ { $($value:tt)* }, $($rest:tt)*) => {
        proto_internal!(@array $msg $repeated [
                $($vals),+ ,
                {
                    let mut $msg = ::$($msgtype)::+::new();
                    proto_internal!(@merge $msg $($value)*);
                    proto_internal!(@msg $msg $($value)*);
                    $msg
                }
            ] $($rest)*)
    };
    // Message nested in [] literal
    (@array $msg:ident $repeated:ident [$($vals:expr),+] $($msgtype:ident)::+ { $($value:tt)* }) => {
        [
            $($vals),+ ,
            {
                let mut $msg = $($msgtype)::+::new();
                proto_internal!(@merge $msg $($value)*);
                proto_internal!(@msg $msg $($value)*);
                $msg
            }
        ]
    };
    // Message nested in [] literal with leading :: on type
    (@array $msg:ident $repeated:ident [$($vals:expr),+] ::$($msgtype:ident)::+ { $($value:tt)* }) => {
        [
            $($vals),+ ,
            {
                let mut $msg = ::$($msgtype)::+::new();
                proto_internal!(@merge $msg $($value)*);
                proto_internal!(@msg $msg $($value)*);
                $msg
            }
        ]
    };

    // Message nested in array literal with trailing array items
    (@array $msg:ident $repeated:ident [] $($msgtype:ident)::+ { $($value:tt)* }, $($rest:tt)*) => {
        proto_internal!(@array $msg $repeated [
                {
                    let mut $msg = $($msgtype)::+::new();
                    proto_internal!(@merge $msg $($value)*);
                    proto_internal!(@msg $msg $($value)*);
                    $msg
                }
            ] $($rest)*)
    };
    // with leading ::
    (@array $msg:ident $repeated:ident [] ::$($msgtype:ident)::+ { $($value:tt)* }, $($rest:tt)*) => {
        proto_internal!(@array $msg $repeated [
                {
                    let mut $msg = ::$($msgtype)::+::new();
                    proto_internal!(@merge $msg $($value)*);
                    proto_internal!(@msg $msg $($value)*);
                    $msg
                }
            ] $($rest)*)
    };
    // Message nested in [] literal
    (@array $msg:ident $repeated:ident [] $($msgtype:ident)::+ { $($value:tt)* }) => {
        [
            {
                let mut $msg = $($msgtype)::+::new();
                proto_internal!(@merge $msg $($value)*);
                proto_internal!(@msg $msg $($value)*);
                $msg
            }
        ]
    };
    (@array $msg:ident $repeated:ident [] ::$($msgtype:ident)::+ { $($value:tt)* }) => {
        [
            {
                let mut $msg = ::$($msgtype)::+::new();
                proto_internal!(@merge $msg $($value)*);
                proto_internal!(@msg $msg $($value)*);
                $msg
            }
        ]
    };

    // Begin handling (key, value) for Maps in array literals
    // Message nested in array literal with trailing array items
    (@array $msg:ident $map:ident [$($vals:expr),+] ($key:expr, __ { $($value:tt)* }), $($rest:tt)*) => {
        proto_internal!(@array $msg $map [
                $($vals),+ ,
                (
                    $key,
                    {
                        let mut $msg = $crate::__internal::get_map_default_value($crate::__internal::Private, $map);
                        proto_internal!(@merge $msg $($value)*);
                        proto_internal!(@msg $msg $($value)*);
                        $msg
                    }
                )
            ] $($rest)*)
    };

    // Message nested in [] literal
    (@array $msg:ident $map:ident [$($vals:expr),+] ($key:expr, __ { $($value:tt)* })) => {
        [
            $($vals),+ ,
            (
                $key,
                {
                    let mut $msg = $crate::__internal::get_map_default_value($crate::__internal::Private, $map);
                    proto_internal!(@merge $msg $($value)*);
                    proto_internal!(@msg $msg $($value)*);
                    $msg
                }
            )
        ]
    };

    // Message nested in array literal with trailing array items
    (@array $msg:ident $map:ident [] ($key:expr, __ { $($value:tt)* }), $($rest:tt)*) => {
        proto_internal!(@array $msg $map [
                (
                    $key,
                    {
                        let mut $msg = $crate::__internal::get_map_default_value($crate::__internal::Private, $map);
                        proto_internal!(@merge $msg $($value)*);
                        proto_internal!(@msg $msg $($value)*);
                        $msg
                    }
                )
            ] $($rest)*)
    };

    // Message nested in [] literal
    (@array $msg:ident $map:ident [] ($key:expr, __ { $($value:tt)* })) => {
        [
            (
                $key,
                {
                    let mut $msg = $crate::__internal::get_map_default_value($crate::__internal::Private, $map);
                    proto_internal!(@merge $msg $($value)*);
                    proto_internal!(@msg $msg $($value)*);
                    $msg
                }
            )
        ]
    };

    // End of __ repeated, now we need to handle named types

    // Message nested in array literal with trailing array items
    (@array $msg:ident $map:ident [$($vals:expr),+] ($key:expr, $($msgtype:ident)::+ { $($value:tt)* }), $($rest:tt)*) => {
        proto_internal!(@array $msg $map [
                $($vals),+ ,
                (
                    $key,
                    {
                        let mut $msg = $($msgtype)::+::new();
                        proto_internal!(@merge $msg $($value)*);
                        proto_internal!(@msg $msg $($value)*);
                        $msg
                    }
                )
            ] $($rest)*)
    };
    // Message nested in [] literal with leading :: on type and trailing array items
    (@array $msg:ident $map:ident [$($vals:expr),+] ($key:expr, ::$($msgtype:ident)::+ { $($value:tt)* }), $($rest:tt)*) => {
        proto_internal!(@array $msg $map [
                $($vals),+ ,
                (
                    $key,
                    {
                        let mut $msg = ::$($msgtype)::+::new();
                        proto_internal!(@merge $msg $($value)*);
                        proto_internal!(@msg $msg $($value)*);
                        $msg
                    }
                )
            ] $($rest)*)
    };
    // Message nested in [] literal
    (@array $msg:ident $map:ident [$($vals:expr),+] ($key:expr, $($msgtype:ident)::+ { $($value:tt)* })) => {
        [
            $($vals),+ ,
            (
                $key,
                {
                    let mut $msg = $($msgtype)::+::new();
                    proto_internal!(@merge $msg $($value)*);
                    proto_internal!(@msg $msg $($value)*);
                    $msg
                }
            )
        ]
    };
    // Message nested in [] literal with leading :: on type
    (@array $msg:ident $map:ident [$($vals:expr),+] ($key:expr, ::$($msgtype:ident)::+ { $($value:tt)* })) => {
        [
            $($vals),+ ,
            (
                $key,
                {
                    let mut $msg = ::$($msgtype)::+::new();
                    proto_internal!(@merge $msg $($value)*);
                    proto_internal!(@msg $msg $($value)*);
                    $msg
                }
            )
        ]
    };

    // Message nested in array literal with trailing array items
    (@array $msg:ident $map:ident [] ($key:expr, $($msgtype:ident)::+ { $($value:tt)* }), $($rest:tt)*) => {
        proto_internal!(@array $msg $map [
                (
                    $key,
                    {
                        let mut $msg = $($msgtype)::+::new();
                        proto_internal!(@merge $msg $($value)*);
                        proto_internal!(@msg $msg $($value)*);
                        $msg
                    }
                )
            ] $($rest)*)
    };
    // with leading ::
    (@array $msg:ident $map:ident [] ($key:expr, ::$($msgtype:ident)::+ { $($value:tt)* }), $($rest:tt)*) => {
        proto_internal!(@array $msg $map [
                (
                    $key,
                    {
                        let mut $msg = ::$($msgtype)::+::new();
                        proto_internal!(@merge $msg $($value)*);
                        proto_internal!(@msg $msg $($value)*);
                        $msg
                    }
                )
            ] $($rest)*)
    };
    // Message nested in [] literal
    (@array $msg:ident $map:ident [] ($key:expr, $($msgtype:ident)::+ { $($value:tt)* })) => {
        [
            (
                $key,
                {
                    let mut $msg = $($msgtype)::+::new();
                    proto_internal!(@merge $msg $($value)*);
                    proto_internal!(@msg $msg $($value)*);
                    $msg
                }
            )
        ]
    };
    (@array $msg:ident $map:ident [] ::($key:expr, $($msgtype:ident)::+ { $($value:tt)* })) => {
        [
            (
                $key,
                {
                    let mut $msg = ::$($msgtype)::+::new();
                    proto_internal!(@merge $msg $($value)*);
                    proto_internal!(@msg $msg $($value)*);
                    $msg
                }
            )
        ]
    };
    // End handling of (key, value) for Maps

    (@array $msg:ident $repeated:ident [$($vals:expr),+] $expr:expr, $($rest:tt)*) => {
        proto_internal!(@array $msg $repeated [$($vals),+, $expr]  $($rest)*)
    };
    (@array $msg:ident $repeated:ident [$($vals:expr),+] $expr:expr) => {
        [$($vals),+, $expr]
    };
    (@array $msg:ident $repeated:ident [] $expr:expr, $($rest:tt)*) => {
        proto_internal!(@array $msg $repeated [$expr]  $($rest)*)
    };
    (@array $msg:ident $repeated:ident [] $expr:expr) => {
        [$expr]
    };
    (@array $msg:ident $repeated:ident []) => {
        []
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

    (@msg $msg:ident ..$expr:expr) => {
    };

    (@msg $msg:ident) => {};
    (@merge $msg:ident) => {};

    // entry point
    ($msgtype:ty { $($tt:tt)* }) => {
        {
            let mut message = <$msgtype>::new();
            proto_internal!(@merge message $($tt)*);
            proto_internal!(@msg message $($tt)*);
            message
        }
    };
}
