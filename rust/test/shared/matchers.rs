use googletest::description::Description;
use googletest::matcher::MatcherResult;
use googletest::prelude::*;
use protobuf::{AbsentField, Optional, PresentField, ProxiedWithPresence};
use std::marker::PhantomData;

/// ===============================
///               IS_UNSET
/// ===============================
pub fn is_unset<'a, T: std::fmt::Debug + ProxiedWithPresence + ?Sized + 'a>()
-> impl Matcher<ActualT = Optional<PresentField<'a, T>, AbsentField<'a, T>>> {
    UnsetMatcher::<T> { _phantom: PhantomData }
}

struct UnsetMatcher<'a, T: ProxiedWithPresence + ?Sized> {
    _phantom: PhantomData<PresentField<'a, T>>,
}

impl<'a, T: std::fmt::Debug + ProxiedWithPresence + ?Sized> Matcher for UnsetMatcher<'a, T> {
    type ActualT = Optional<PresentField<'a, T>, AbsentField<'a, T>>;

    fn matches(&self, actual: &Self::ActualT) -> MatcherResult {
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
-> impl Matcher<ActualT = Optional<PresentField<'a, T>, AbsentField<'a, T>>> {
    SetMatcher::<T> { _phantom: PhantomData }
}

struct SetMatcher<'a, T: ProxiedWithPresence + ?Sized> {
    _phantom: PhantomData<PresentField<'a, T>>,
}

impl<'a, T: std::fmt::Debug + ProxiedWithPresence + ?Sized> Matcher for SetMatcher<'a, T> {
    type ActualT = Optional<PresentField<'a, T>, AbsentField<'a, T>>;

    fn matches(&self, actual: &Self::ActualT) -> MatcherResult {
        actual.is_set().into()
    }

    fn describe(&self, matcher_result: MatcherResult) -> Description {
        match matcher_result {
            MatcherResult::Match => "is set".into(),
            MatcherResult::NoMatch => "is not set".into(),
        }
    }
}
