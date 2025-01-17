// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::description::Description;
use googletest::matcher::{Matcher, MatcherBase, MatcherResult};
use protobuf::__internal::MatcherEq;

#[derive(MatcherBase)]
pub struct MessageMatcher<T: MatcherEq> {
    expected: T,
}

impl<T> Matcher<&T> for MessageMatcher<T>
where
    T: MatcherEq,
{
    fn matches(&self, actual: &T) -> MatcherResult {
        actual.matches(&self.expected).into()
    }

    fn describe(&self, matcher_result: MatcherResult) -> Description {
        match matcher_result {
            MatcherResult::Match => format!("is equal to {:?}", self.expected).into(),
            MatcherResult::NoMatch => format!("is not equal to {:?}", self.expected).into(),
        }
    }
}

impl<T> Matcher<T> for MessageMatcher<T>
where
    T: MatcherEq + Copy,
{
    fn matches(&self, actual: T) -> MatcherResult {
        actual.matches(&self.expected).into()
    }

    fn describe(&self, matcher_result: MatcherResult) -> Description {
        match matcher_result {
            MatcherResult::Match => format!("is equal to {:?}", self.expected).into(),
            MatcherResult::NoMatch => format!("is not equal to {:?}", self.expected).into(),
        }
    }
}

pub fn proto_eq<T: MatcherEq>(expected: T) -> MessageMatcher<T> {
    MessageMatcher { expected }
}
