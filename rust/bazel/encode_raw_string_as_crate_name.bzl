# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Implements encode_raw_string_as_crate_name.

Based on an implementation in rules_rust:
* https://github.com/bazelbuild/rules_rust/blob/cdaf15f5796e3e934b074526272823284bbaed01/rust/private/utils.bzl#L643
"""

# This is a list of pairs, where the first element of the pair is a character
# that is allowed in Bazel package or target names but not in crate names; and
# the second element is an encoding of that char suitable for use in a crate
# name.
_encodings = (
    (":", "x"),
    ("!", "excl"),
    ("%", "prc"),
    ("@", "ao"),
    ("^", "caret"),
    ("`", "bt"),
    (" ", "sp"),
    ("\"", "dq"),
    ("#", "octo"),
    ("$", "dllr"),
    ("&", "amp"),
    ("'", "sq"),
    ("(", "lp"),
    (")", "rp"),
    ("*", "astr"),
    ("-", "d"),
    ("+", "pl"),
    (",", "cm"),
    (";", "sm"),
    ("<", "la"),
    ("=", "eq"),
    (">", "ra"),
    ("?", "qm"),
    ("[", "lbk"),
    ("]", "rbk"),
    ("{", "lbe"),
    ("|", "pp"),
    ("}", "rbe"),
    ("~", "td"),
    ("/", "y"),
    (".", "pd"),
)

# For each of the above encodings, we generate two substitution rules: one that
# ensures any occurrences of the encodings themselves in the package/target
# aren't clobbered by this translation, and one that does the encoding itself.
# We also include a rule that protects the clobbering-protection rules from
# getting clobbered.
_substitutions = [("_z", "_zz_")] + [
    subst
    for (pattern, replacement) in _encodings
    for subst in (
        ("_{}_".format(replacement), "_z{}_".format(replacement)),
        (pattern, "_{}_".format(replacement)),
    )
]

# Expose the substitutions for testing only.
substitutions_for_testing = _substitutions

def _replace_all(string, substitutions):
    """Replaces occurrences of the given patterns in `string`.

    There are a few reasons this looks complicated:
    * The substitutions are performed with some priority, i.e. patterns that are
      listed first in `substitutions` are higher priority than patterns that are
      listed later.
    * We also take pains to avoid doing replacements that overlap with each
      other, since overlaps invalidate pattern matches.
    * To avoid hairy offset invalidation, we apply the substitutions
      right-to-left.
    * To avoid the "_quote" -> "_quotequote_" rule introducing new pattern
      matches later in the string during decoding, we take the leftmost
      replacement, in cases of overlap.  (Note that no rule can induce new
      pattern matches *earlier* in the string.) (E.g. "_quotedot_" encodes to
      "_quotequote_dot_". Note that "_quotequote_" and "_dot_" both occur in
      this string, and overlap.).

    Args:
        string (string): the string in which the replacements should be performed.
        substitutions: the list of patterns and replacements to apply.

    Returns:
        A string with the appropriate substitutions performed.
    """

    # Find the highest-priority pattern matches for each string index, going
    # left-to-right and skipping indices that are already involved in a
    # pattern match.
    plan = {}
    matched_indices_set = {}
    for pattern_start in range(len(string)):
        if pattern_start in matched_indices_set:
            continue
        for (pattern, replacement) in substitutions:
            if not string.startswith(pattern, pattern_start):
                continue
            length = len(pattern)
            plan[pattern_start] = (length, replacement)
            matched_indices_set.update([(pattern_start + i, True) for i in range(length)])
            break

    # Execute the replacement plan, working from right to left.
    for pattern_start in sorted(plan.keys(), reverse = True):
        length, replacement = plan[pattern_start]
        after_pattern = pattern_start + length
        string = string[:pattern_start] + replacement + string[after_pattern:]

    return string

def encode_raw_string_as_crate_name(str):
    """Encodes a string using the above encoding format.

    Args:
        str (string): The string to be encoded.

    Returns:
        An encoded version of the input string.
    """
    return _replace_all(str, _substitutions)

def decode_crate_name_as_raw_string_for_testing(crate_name):
    """Decodes a crate_name that was encoded by encode_raw_string_as_crate_name.

    This is used to check that the encoding is bijective; it is expected to only
    be used in tests.

    Args:
        crate_name (string): The name of the crate.

    Returns:
        A string representing the Bazel label (package and target).
    """
    return _replace_all(crate_name, [(t[1], t[0]) for t in _substitutions])
