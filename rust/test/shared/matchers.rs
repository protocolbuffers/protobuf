use googletest::description::Description;
use googletest::matcher::MatcherResult;
use googletest::prelude::*;
use protobuf::{AbsentField, Optional, PresentField, ProxiedWithPresence};
use std::marker::PhantomData;

/// ===============================
///               IS_UNSET
/// ===============================
pub fn is_unset<'a, 'b, T: std::fmt::Debug + ProxiedWithPresence + ?Sized + 'a>()
-> impl Matcher<&'b Optional<PresentField<'a, T>, AbsentField<'a, T>>> {
    UnsetMatcher
}

#[derive(MatcherBase)]
struct UnsetMatcher;

impl<'a, 'b, T: std::fmt::Debug + ProxiedWithPresence + ?Sized>
    Matcher<&'b Optional<PresentField<'a, T>, AbsentField<'a, T>>> for UnsetMatcher
{
    fn matches(
        &self,
        actual: &'b Optional<PresentField<'a, T>, AbsentField<'a, T>>,
    ) -> MatcherResult {
        actual.is_unset().into()
    }

    fn describe(&self, matcher_result: MatcherResult) -> Description {
        match matcher_result {
            MatcherResult::Match => "is not set".into(),
            MatcherResult::NoMatch => "is set".into(),
        }
    }
}

/// ===============================
///               IS_SET
/// ===============================
pub fn is_set<'a, 'b, T: std::fmt::Debug + ProxiedWithPresence + ?Sized + 'a>()
-> impl Matcher<&'b Optional<PresentField<'a, T>, AbsentField<'a, T>>> {
    SetMatcher
}

#[derive(MatcherBase)]
struct SetMatcher;

impl<'a, 'b, T: std::fmt::Debug + ProxiedWithPresence + ?Sized>
    Matcher<&'b Optional<PresentField<'a, T>, AbsentField<'a, T>>> for SetMatcher
{
    fn matches(
        &self,
        actual: &'b Optional<PresentField<'a, T>, AbsentField<'a, T>>,
    ) -> MatcherResult {
        actual.is_set().into()
    }

    fn describe(&self, matcher_result: MatcherResult) -> Description {
        match matcher_result {
            MatcherResult::Match => "is set".into(),
            MatcherResult::NoMatch => "is not set".into(),
        }
    }
}
