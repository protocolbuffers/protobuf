#[macro_export]
macro_rules! proto {
    ($msgtype:ty { $($tt:tt)* }) => {
        $crate::proto_internal!($msgtype { $($tt)* });
    }
}

#[macro_export(local_inner_macros)]
#[doc(hidden)]
macro_rules! proto_internal {
    // nested message,
    (@msg $msg:ident $submsg:ident : $msgtype:ty { $field:ident : $($value:tt)* }, $($rest:tt)*) => {
        proto_internal!(@msg $msg $submsg : $msgtype { $field : $($value)* });
        proto_internal!(@msg $msg $($rest)*);
    };

    // nested message
    (@msg $msg:ident $submsg:ident : $msgtype:ty { $field:ident : $($value:tt)* }) => {
        {
            let mut $msg: <$msgtype as $crate::Proxied>::Mut<'_> = $crate::__internal::paste!($msg.[<$submsg _mut>]());
            proto_internal!(@msg $msg $field : $($value)*);
        }
    };

    // empty nested message,
    (@msg $msg:ident $submsg:ident : $msgtype:ty { }, $($rest:tt)*) => {
        proto_internal!(@msg $msg $submsg : $msgtype { });
        proto_internal!(@msg $msg $($rest)*);
    };

    // empty nested message
    (@msg $msg:ident $submsg:ident : $msgtype:ty { }) => {
        {
            let mut $msg = $crate::__internal::paste!($msg.[<$submsg _mut>]());
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
        $crate::__internal::paste!{
            $msg.[<set_ $ident>]($expr);
        }
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
