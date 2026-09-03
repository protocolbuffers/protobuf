// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::description::Description;
use googletest::matcher::{Matcher, MatcherBase, MatcherResult};
use protobuf::__internal::MatcherEq;
use protobuf::Parse;
use std::fmt::Debug;
use std::marker::PhantomData;

////////////////////////////////////////////////////////////////////////////////
// Matchers
////////////////////////////////////////////////////////////////////////////////

/// Matches a protobuf message for equality.
///
/// This method may have false-negatives or false-positives in the face of unknown fields; see the
/// comments on `message_eq` in message.rs for precise semantics.
pub fn proto_eq<T: MatcherEq>(expected: T) -> MessageMatcher<T> {
    MessageMatcher { expected, partial: false }
}

/// A matcher that can be used to check for partial proto equality.
///
/// In a partial match, only fields present in the expected protobuf are considered.
/// Extra fields set only in the actual protobuf will be ignored during comparison.
///
/// # Examples
/// ```rust
/// use googletest::prelude::*;
/// use protobuf_gtest_matchers::proto_partially_eq;
///
/// expect_that!(actual, proto_partially_eq(expected));
/// ```
pub fn proto_partially_eq<T: MatcherEq>(expected: T) -> MessageMatcher<T> {
    MessageMatcher { expected, partial: true }
}

/// Matches a byte sequence that can be deserialized as a protobuf message matching `inner`.
///
/// The matcher accepts anything implementing `AsRef<[u8]>`, including `&[u8]`, `Vec<u8>`,
/// `&str`, and `String`.
///
/// The matcher reports no match if deserialization fails or if `inner` does not match
/// the deserialized protobuf.
///
/// # Examples
/// ```rust
/// use googletest::prelude::*;
/// use protobuf_gtest_matchers::{proto_eq, when_deserialized};
///
/// let serialized = msg.serialize().unwrap();
/// expect_that!(&serialized, when_deserialized(proto_eq(expected)));
/// ```
pub fn when_deserialized<M, InnerMatcherT>(
    inner: InnerMatcherT,
) -> WhenDeserializedMatcher<M, InnerMatcherT>
where
    // We need to be able to parse the serialized value and print it for error messages.
    M: Parse + Debug,
    // The inner matcher must be a matcher for a message type.
    InnerMatcherT: for<'a> Matcher<&'a M>,
{
    WhenDeserializedMatcher { inner, _phantom: PhantomData }
}

/// Matches a byte sequence that can be deserialized as a protobuf message of type `M` matching
/// `inner`.
///
/// This is equivalent to [`when_deserialized`] with an explicit message type parameter.
///
/// # Examples
/// ```rust
/// use googletest::prelude::*;
/// use protobuf_gtest_matchers::when_deserialized_as;
///
/// let serialized = msg.serialize().unwrap();
/// expect_that!(&serialized, when_deserialized_as::<MyProto, _>(anything()));
/// ```
pub fn when_deserialized_as<M, InnerMatcherT>(
    inner: InnerMatcherT,
) -> WhenDeserializedMatcher<M, InnerMatcherT>
where
    // We need to be able to parse the serialized value and print it for error messages.
    M: Parse + Debug,
    // The inner matcher must be a matcher for a message type.
    InnerMatcherT: for<'a> Matcher<&'a M>,
{
    when_deserialized(inner)
}

////////////////////////////////////////////////////////////////////////////////
// Implementation details
////////////////////////////////////////////////////////////////////////////////

#[derive(MatcherBase)]
pub struct WhenDeserializedMatcher<M, InnerMatcherT> {
    inner: InnerMatcherT,
    _phantom: PhantomData<M>,
}

impl<M, InnerMatcherT, ActualT> Matcher<ActualT> for WhenDeserializedMatcher<M, InnerMatcherT>
where
    // We need to be able to parse the serialized value and print it for error messages.
    M: Parse + Debug,
    // The inner matcher must be a matcher for a message type.
    InnerMatcherT: for<'a> Matcher<&'a M>,
    // Needed to call `parse_dont_enforce_required`
    ActualT: AsRef<[u8]>,
    // Requirements of Matcher
    ActualT: Debug + Copy,
{
    fn matches(&self, actual: ActualT) -> MatcherResult {
        // We use the non-enforcing version to support use with partial matching.
        match M::parse_dont_enforce_required(actual.as_ref()) {
            Ok(msg) => self.inner.matches(&msg),
            Err(_) => MatcherResult::NoMatch,
        }
    }

    fn describe(&self, matcher_result: MatcherResult) -> Description {
        match matcher_result {
            MatcherResult::Match => format!(
                "can be deserialized as a protobuf which {}",
                self.inner.describe(MatcherResult::Match)
            )
            .into(),
            MatcherResult::NoMatch => format!(
                "cannot be deserialized as a protobuf which {}",
                self.inner.describe(MatcherResult::Match)
            )
            .into(),
        }
    }

    fn explain_match(&self, actual: ActualT) -> Description {
        // We use the non-enforcing version to support use with partial matching.
        match M::parse_dont_enforce_required(actual.as_ref()) {
            Ok(msg) => Description::new()
                .text(format!("which deserializes to {msg:?}"))
                .nested(self.inner.explain_match(&msg)),
            Err(e) => format!("which cannot be deserialized as a protobuf: {e}").into(),
        }
    }
}

#[derive(MatcherBase)]
pub struct MessageMatcher<T: MatcherEq> {
    expected: T,
    partial: bool,
}

impl<T> Matcher<&T> for MessageMatcher<T>
where
    T: MatcherEq,
{
    fn matches(&self, actual: &T) -> MatcherResult {
        if self.partial {
            actual.matches_partially(&self.expected).into()
        } else {
            actual.matches(&self.expected).into()
        }
    }

    fn describe(&self, matcher_result: MatcherResult) -> Description {
        match (matcher_result, self.partial) {
            (MatcherResult::Match, false) => format!("is equal to {:?}", self.expected).into(),
            (MatcherResult::NoMatch, false) => {
                format!("is not equal to {:?}", self.expected).into()
            }
            (MatcherResult::Match, true) => {
                format!("is partially equal to {:?}", self.expected).into()
            }
            (MatcherResult::NoMatch, true) => {
                format!("is not partially equal to {:?}", self.expected).into()
            }
        }
    }
}

impl<T> Matcher<T> for MessageMatcher<T>
where
    T: MatcherEq + Copy,
{
    fn matches(&self, actual: T) -> MatcherResult {
        if self.partial {
            actual.matches_partially(&self.expected).into()
        } else {
            actual.matches(&self.expected).into()
        }
    }

    fn describe(&self, matcher_result: MatcherResult) -> Description {
        match (matcher_result, self.partial) {
            (MatcherResult::Match, false) => format!("is equal to {:?}", self.expected).into(),
            (MatcherResult::NoMatch, false) => {
                format!("is not equal to {:?}", self.expected).into()
            }
            (MatcherResult::Match, true) => {
                format!("is partially equal to {:?}", self.expected).into()
            }
            (MatcherResult::NoMatch, true) => {
                format!("is not partially equal to {:?}", self.expected).into()
            }
        }
    }
}
