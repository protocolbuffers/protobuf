// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use crate::__internal::{Private, SealedInternal};
use crate::{
    AsView, IntoProxied, IntoView, ProtoBytes, ProtoStr, ProtoString, Proxied, Proxy, View,
    ViewProxy,
};
use paste::paste;
use std::cmp::PartialEq;
use std::ops::Deref;

macro_rules! impl_cord_types {
    ($($t:ty, $vt:ty);*) => {
        paste! { $(
          #[derive(Debug)]
          pub struct [< $t Cord>](Private);

          #[derive(Debug)]
          pub enum [< $t Cow>]<'a> {
            Borrowed(View<'a, $t>),
            Owned($t),
          }

          impl SealedInternal for [< $t Cord>] {}

          impl<'msg> SealedInternal for [< $t Cow>]<'msg> {}

          impl Proxied for [< $t Cord>] {
            type View<'msg> = [< $t Cow>]<'msg>;
          }

          impl AsView for [< $t Cord>] {
            type Proxied = Self;
            fn as_view(&self) -> [< $t Cow>]<'_> {
              unimplemented!("Proto Cord should never be constructed");
            }
          }

          impl<'msg> Proxy<'msg> for [< $t Cow>]<'msg> {}

          impl<'msg> ViewProxy<'msg> for [< $t Cow>]<'msg> {}

          impl<'msg> AsView for [< $t Cow>]<'msg> {
            type Proxied = [< $t Cord>];

            fn as_view(&self) -> [< $t Cow>]<'_> {
              match self {
                [< $t Cow>]::Owned(owned) => [< $t Cow>]::Borrowed((*owned).as_view()),
                [< $t Cow>]::Borrowed(borrowed) => [< $t Cow>]::Borrowed(borrowed),
              }
            }
          }

          impl<'msg> IntoView<'msg> for [< $t Cow>]<'msg> {
            fn into_view<'shorter>(self) -> [< $t Cow>]<'shorter>
            where
                'msg: 'shorter, {
              match self {
                [< $t Cow>]::Owned(owned) => [< $t Cow>]::Owned(owned),
                [< $t Cow>]::Borrowed(borrow) => [< $t Cow>]::Borrowed(borrow.into_view()),
              }
            }
          }

          impl IntoProxied<$t> for [< $t Cow>]<'_> {
              fn into_proxied(self, _private: Private) -> $t {
                match self {
                  [< $t Cow>]::Owned(owned) => owned,
                  [< $t Cow>]::Borrowed(borrowed) => borrowed.into_proxied(Private),
                }
              }
          }

          impl<'a> Deref for [< $t Cow>]<'a> {
            type Target = $vt;

            fn deref(&self) -> View<'_, $t> {
                match self {
                    [< $t Cow>]::Borrowed(borrow) => borrow,
                    [< $t Cow>]::Owned(owned) => (*owned).as_view(),
                }
            }
          }

          impl AsRef<[u8]> for [< $t Cow>]<'_> {
            fn as_ref(&self) -> &[u8] {
                match self {
                  [< $t Cow>]::Borrowed(borrow) => borrow.as_ref(),
                  [< $t Cow>]::Owned(owned) => owned.as_ref(),
                }
            }
          }
          )*
        }
    }
}

impl_cord_types!(
    ProtoString, ProtoStr;
    ProtoBytes, [u8]
);

macro_rules! impl_eq {
  ($($t1:ty, $t2:ty);*) => {
      paste! { $(
        impl PartialEq<$t1> for $t2 {
          fn eq(&self, rhs: &$t1) -> bool {
              AsRef::<[u8]>::as_ref(self) == AsRef::<[u8]>::as_ref(rhs)
          }
        }
        )*
      }
  }
}

impl_eq!(
  ProtoStringCow<'_>, ProtoStringCow<'_>;
  str, ProtoStringCow<'_>;
  ProtoStringCow<'_>, str;
  ProtoBytesCow<'_>, ProtoBytesCow<'_>;
  [u8], ProtoBytesCow<'_>;
  ProtoBytesCow<'_>, [u8]
);
