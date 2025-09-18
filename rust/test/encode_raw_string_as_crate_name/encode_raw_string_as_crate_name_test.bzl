"""
Unit tests for the encode_raw_string_as_crate_name() function.
"""

load("@bazel_skylib//lib:unittest.bzl", "asserts", "unittest")
load(
    "//rust/bazel:encode_raw_string_as_crate_name.bzl",
    "decode_crate_name_as_raw_string_for_testing",
    "encode_raw_string_as_crate_name",
    "substitutions_for_testing",
)

def _encode_raw_string_as_crate_name_test_impl(ctx):
    env = unittest.begin(ctx)

    asserts.equals(env, encode_raw_string_as_crate_name("some_project:utils"), "some_project_x_utils")
    asserts.equals(env, encode_raw_string_as_crate_name("_zpd_"), "_zz_pd_")

    # Typical cases for labels.
    asserts.equals(
        env,
        "target",
        encode_raw_string_as_crate_name("target"),
    )
    asserts.equals(
        env,
        "package_x_target",
        encode_raw_string_as_crate_name("package:target"),
    )
    asserts.equals(
        env,
        "some_y_package_x_target",
        encode_raw_string_as_crate_name("some/package:target"),
    )
    asserts.equals(
        env,
        "_y__y_some_y_package_x_target",
        encode_raw_string_as_crate_name("//some/package:target"),
    )
    asserts.equals(
        env,
        "_ao_repo_y__y_some_y_package_x_target",
        encode_raw_string_as_crate_name("@repo//some/package:target"),
    )
    asserts.equals(
        env,
        "x_y_y_x_z",
        encode_raw_string_as_crate_name("x/y:z"),
    )

    # Target name includes a character illegal in crate names.
    asserts.equals(
        env,
        "some_y_package_x_foo_y_target",
        encode_raw_string_as_crate_name("some/package:foo/target"),
    )

    # Package/target includes some of the encodings.
    asserts.equals(
        env,
        "some_s__y_package_x_target_dot_foo",
        encode_raw_string_as_crate_name("some_s_/package:target_dot_foo"),
    )

    # Some pathological cases: test that round-tripping the encoding works as
    # expected.

    # Label includes a quoted encoding.
    target = "_zpd_:target"
    asserts.equals(env, "_zz_pd__x_target", encode_raw_string_as_crate_name(target))
    asserts.equals(env, target, decode_crate_name_as_raw_string_for_testing(encode_raw_string_as_crate_name(target)))

    target = "x_y_y:z"
    asserts.equals(env, "x_zy_y_x_z", encode_raw_string_as_crate_name(target))
    asserts.equals(env, target, decode_crate_name_as_raw_string_for_testing(encode_raw_string_as_crate_name(target)))

    # Package is identical to a valid encoding already.
    target = "_zz_pd__x_target:target"
    asserts.equals(env, "_zz_z_zpd__zx_target_x_target", encode_raw_string_as_crate_name(target))
    asserts.equals(env, target, decode_crate_name_as_raw_string_for_testing(encode_raw_string_as_crate_name(target)))

    # No surprises in the application of the substitutions, everything is
    # encoded as expected.
    for (orig, encoded) in substitutions_for_testing:
        asserts.equals(env, encode_raw_string_as_crate_name(orig), encoded)

    return unittest.end(env)

def _substitutions_concatenate_test_impl(ctx):
    env = unittest.begin(ctx)

    # Every combination of orig + orig, orig + encoded, encoded + orig, and
    # encoded + encoded round trips the encoding successfully.
    all_symbols = [s for pair in substitutions_for_testing for s in pair]
    for s in all_symbols:
        for t in all_symbols:
            concatenated = s + t
            asserts.equals(env, decode_crate_name_as_raw_string_for_testing(encode_raw_string_as_crate_name(concatenated)), concatenated)

    return unittest.end(env)

def _decode_test_impl(ctx):
    env = unittest.begin(ctx)
    asserts.equals(env, decode_crate_name_as_raw_string_for_testing("some_project_x_utils"), "some_project:utils")
    asserts.equals(env, decode_crate_name_as_raw_string_for_testing("_zz_pd_"), "_zpd_")

    # No surprises in the application of the substitutions, everything is
    # decoded as expected.
    for (orig, encoded) in substitutions_for_testing:
        asserts.equals(env, decode_crate_name_as_raw_string_for_testing(encoded), orig)

    return unittest.end(env)

encode_raw_string_as_crate_name_test = unittest.make(_encode_raw_string_as_crate_name_test_impl)
substitutions_concatenate_test = unittest.make(_substitutions_concatenate_test_impl)
decode_test = unittest.make(_decode_test_impl)

def encode_raw_string_as_crate_name_test_suite(name):
    unittest.suite(
        name,
        encode_raw_string_as_crate_name_test,
        substitutions_concatenate_test,
        decode_test,
    )
