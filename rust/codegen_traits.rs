// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Traits that are implemeted by codegen types.

use crate::{MutProxied, MutProxy, ViewProxy};
use create::Parse;
use read::Serialize;
use std::fmt::Debug;
use write::{Clear, ClearAndParse};

/// A trait that all generated owned message types implement.
pub trait Message: MutProxied
  // Create traits:
  + Parse + Default
  // Read traits:
  + Debug + Serialize
  // Write traits:
  + Clear + ClearAndParse
  // Thread safety:
  + Send + Sync
  // Copy/Clone:
  + Clone
  {}

/// A trait that all generated message views implement.
pub trait MessageView<'msg>: ViewProxy<'msg, Proxied = Self::Message>
    // Read traits:
    + Debug + Serialize
    // Thread safety:
    + Send + Sync
    // Copy/Clone:
    + Copy + Clone
{
    #[doc(hidden)]
    type Message: Message;
}

/// A trait that all generated message muts implement.
pub trait MessageMut<'msg>:
    MutProxy<'msg, MutProxied = Self::Message>
    // Read traits:
    + Debug + Serialize
    // Write traits:
    // TODO: MsgMut should impl ClearAndParse.
    + Clear
    // Thread safety:
    + Sync
    // Copy/Clone:
    // (Neither)
{
    #[doc(hidden)]
    type Message: Message;
}

/// Operations related to constructing a message. Only owned messages implement
/// these traits.
pub(crate) mod create {
    pub trait Parse: Sized {
        fn parse(serialized: &[u8]) -> Result<Self, crate::ParseError>;
    }
}

/// Operations related to reading some aspect of a message (methods that would
/// have a `&self` receiver on an owned message). Owned messages, views, and
/// muts all implement these traits.
pub(crate) mod read {
    pub trait Serialize {
        fn serialize(&self) -> Result<Vec<u8>, crate::SerializeError>;
    }
}

/// Operations related to mutating a message (methods that would have a `&mut
/// self` receiver on an owned message). Owned messages and muts implement these
/// traits.
pub(crate) mod write {

    pub trait Clear {
        fn clear(&mut self);
    }

    pub trait ClearAndParse {
        fn clear_and_parse(&mut self, data: &[u8]) -> Result<(), crate::ParseError>;
    }
}
