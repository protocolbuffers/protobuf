load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def _non_module_deps_impl(ctx):
    http_archive(
        name = "jsoncpp",
        urls = ["https://github.com/open-source-parsers/jsoncpp/archive/9059f5cad030ba11d37818847443a53918c327b1.zip"],
        sha256 = "c0c583c7b53a53bcd1f7385f15439dcdf0314d550362379e2db9919a918d1996",
        strip_prefix = "jsoncpp-9059f5cad030ba11d37818847443a53918c327b1",  # 1.9.4
    )
    http_archive(
        name = "lua",
        build_file = "//bazel:lua.BUILD",
        sha256 = "b9e2e4aad6789b3b63a056d442f7b39f0ecfca3ae0f1fc0ae4e9614401b69f4b",
        strip_prefix = "lua-5.2.4",
        urls = [
            "https://mirror.bazel.build/www.lua.org/ftp/lua-5.2.4.tar.gz",
            "https://www.lua.org/ftp/lua-5.2.4.tar.gz",
        ],
    )
    http_archive(
        name = "rules_fuzzing",
        sha256 = "ff52ef4845ab00e95d29c02a9e32e9eff4e0a4c9c8a6bcf8407a2f19eb3f9190",
        strip_prefix = "rules_fuzzing-0.4.1",
        urls = ["https://github.com/bazelbuild/rules_fuzzing/releases/download/v0.4.1/rules_fuzzing-0.4.1.zip"],
        patches = ["//third_party:rules_fuzzing.patch"],
        patch_args = ["-p1"],
    )
    http_archive(
        name = "utf8_range",
        urls = ["https://github.com/protocolbuffers/utf8_range/archive/d863bc33e15cba6d873c878dcca9e6fe52b2f8cb.zip"],
        sha256 = "568988b5f7261ca181468dba38849fabf59dd9200fb2ed4b2823da187ef84d8c",
        strip_prefix = "utf8_range-d863bc33e15cba6d873c878dcca9e6fe52b2f8cb",
    )

non_module_deps = module_extension(implementation = _non_module_deps_impl)
