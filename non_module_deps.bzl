load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def _github_archive(repo, commit, **kwargs):
    repo_name = repo.split("/")[-1]
    http_archive(
        urls = [repo + "/archive/" + commit + ".zip"],
        strip_prefix = repo_name + "-" + commit,
        **kwargs
    )

def _non_module_deps_impl(ctx):
    _github_archive(
        name = "utf8_range",
        repo = "https://github.com/protocolbuffers/utf8_range",
        commit = "de0b4a8ff9b5d4c98108bdfe723291a33c52c54f",
        sha256 = "5da960e5e5d92394c809629a03af3c7709d2d3d0ca731dacb3a9fb4bf28f7702",
    )

non_module_deps = module_extension(implementation = _non_module_deps_impl)