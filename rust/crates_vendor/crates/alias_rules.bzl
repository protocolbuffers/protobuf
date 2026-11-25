"""Alias that transitions its target to `compilation_mode=opt`.  Use `transition_alias="opt"` to enable."""

load("@rules_cc//cc:defs.bzl", "CcInfo")
load("@rules_rust//rust:rust_common.bzl", "COMMON_PROVIDERS")

def _transition_alias_impl(ctx):
    # `ctx.attr.actual` is a list of 1 item due to the transition
    providers = [ctx.attr.actual[0][provider] for provider in COMMON_PROVIDERS]
    if CcInfo in ctx.attr.actual[0]:
        providers.append(ctx.attr.actual[0][CcInfo])
    return providers

def _change_compilation_mode(compilation_mode):
    def _change_compilation_mode_impl(_settings, _attr):
        return {
            "//command_line_option:compilation_mode": compilation_mode,
        }

    return transition(
        implementation = _change_compilation_mode_impl,
        inputs = [],
        outputs = [
            "//command_line_option:compilation_mode",
        ],
    )

def _transition_alias_rule(compilation_mode):
    return rule(
        implementation = _transition_alias_impl,
        provides = COMMON_PROVIDERS,
        attrs = {
            "actual": attr.label(
                mandatory = True,
                doc = "`rust_library()` target to transition to `compilation_mode=opt`.",
                providers = COMMON_PROVIDERS,
                cfg = _change_compilation_mode(compilation_mode),
            ),
            "_allowlist_function_transition": attr.label(
                default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
            ),
        },
        doc = "Transitions a Rust library crate to the `compilation_mode=opt`.",
    )

transition_alias_dbg = _transition_alias_rule("dbg")
transition_alias_fastbuild = _transition_alias_rule("fastbuild")
transition_alias_opt = _transition_alias_rule("opt")
