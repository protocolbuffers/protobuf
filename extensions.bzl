"bzlmod extensions, needed to support Bazel 6 users as there was no use_repo_rule in MODULE.bazel."
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def _one_off_ext_impl(ctx):
    http_archive(
        name = "utf8_range",
        urls = ["https://github.com/protocolbuffers/utf8_range/archive/d863bc33e15cba6d873c878dcca9e6fe52b2f8cb.zip"],
        sha256 = "568988b5f7261ca181468dba38849fabf59dd9200fb2ed4b2823da187ef84d8c",
        strip_prefix = "utf8_range-d863bc33e15cba6d873c878dcca9e6fe52b2f8cb",
    )

deps = module_extension(_one_off_ext_impl)