def internal_ruby_extension(
        name,
        extension,
        deps = [],
        **kwargs):
    """Bazel rule to wrap up a generated ruby extension.

    NOTE: the rule is only an internal workaround. The interface may change and
    the rule may be removed when everything is properly "Bazelified".

    Args:
      name: the name of the target.
      extension: the path of the extension file.
      deps: extra dependencies to add.
      **kwargs: extra arguments to forward to the genrule.
    """


    native.genrule(
        name = name,
        srcs = deps + [
            "Rakefile",
            ":srcs",
            ":test_ruby_protos",
            ":tests",
            "//third_party/utf8_range:all_files",
        ],
        tags = ["manual"],
        outs = [extension],
        cmd = "pushd `dirname $(location Rakefile)`\n" +
              "BAZEL=true rake\n" +
              "popd\n" +
              "cp `dirname $(location Rakefile)`/%s $(OUTS)\n" % extension,
        **kwargs,
    )
