use googletest::description::Description;
use googletest::matcher::MatcherResult;
use googletest::prelude::*;
use protobuf::{AbsentField, Optional, PresentField, ProxiedWithPresence};

/// ===============================
///               IS_UNSET
/// ===============================
pub fn is_unset<'a, T: std::fmt::Debug + ProxiedWithPresence + ?Sized + 'a>()
-> impl Matcher<Optional<PresentField<'a, T>, AbsentField<'a, T>>> {
    UnsetMatcher
}

#[derive(MatcherExt)]
struct UnsetMatcher;

impl<'a, T: std::fmt::Debug + ProxiedWithPresence + ?Sized>
    Matcher<Optional<PresentField<'a, T>, AbsentField<'a, T>>> for UnsetMatcher
{
    fn matches(&self, actual: &Optional<PresentField<'a, T>, AbsentField<'a, T>>) -> MatcherResult {
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
pub fn is_set<'a, T: std::fmt::Debug + ProxiedWithPresence + ?Sized + 'a>()
-> impl Matcher<Optional<PresentField<'a, T>, AbsentField<'a, T>>> {
    SetMatcher
}

#[derive(MatcherExt)]
struct SetMatcher;

impl<'a, T: std::fmt::Debug + ProxiedWithPresence + ?Sized>
    Matcher<Optional<PresentField<'a, T>, AbsentField<'a, T>>> for SetMatcher
{
    fn matches(&self, actual: &Optional<PresentField<'a, T>, AbsentField<'a, T>>) -> MatcherResult {
        actual.is_set().into()
    }

    fn describe(&self, matcher_result: MatcherResult) -> Description {
        match matcher_result {
            MatcherResult::Match => "is set".into(),
            MatcherResult::NoMatch => "is not set".into(),
        }
    }
}
