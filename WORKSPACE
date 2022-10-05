workspace(name = "com_google_protobuf")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

local_repository(
    name = "com_google_protobuf_examples",
    path = "examples",
)

http_archive(
    name = "com_google_googletest",
    sha256 = "ea54c9845568cb31c03f2eddc7a40f7f83912d04ab977ff50ec33278119548dd",
    strip_prefix = "googletest-4c9a3bb62bf3ba1f1010bf96f9c8ed767b363774",
    urls = [
        "https://github.com/google/googletest/archive/4c9a3bb62bf3ba1f1010bf96f9c8ed767b363774.tar.gz",
    ],
)

# Bazel platform rules.
http_archive(
    name = "platforms",
    sha256 = "a879ea428c6d56ab0ec18224f976515948822451473a80d06c2e50af0bbe5121",
    strip_prefix = "platforms-da5541f26b7de1dc8e04c075c99df5351742a4a2",
    urls = ["https://github.com/bazelbuild/platforms/archive/da5541f26b7de1dc8e04c075c99df5351742a4a2.zip"],  # 2022-05-27
)

# Load common dependencies.
load("//:protobuf_deps.bzl", "PROTOBUF_MAVEN_ARTIFACTS", "protobuf_deps")
protobuf_deps()

load("@rules_jvm_external//:defs.bzl", "maven_install")

maven_install(
    artifacts = PROTOBUF_MAVEN_ARTIFACTS,
    # For updating instructions, see:
    # https://github.com/bazelbuild/rules_jvm_external#updating-maven_installjson
    maven_install_json = "//:maven_install.json",
    repositories = [
        "https://repo1.maven.org/maven2",
        "https://repo.maven.apache.org/maven2",
    ],
)

load("@maven//:defs.bzl", "pinned_maven_install")

pinned_maven_install()

# For `cc_proto_blacklist_test` and `build_test`.
load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

http_archive(
    name = "rules_python",
    sha256 = "9fcf91dbcc31fde6d1edb15f117246d912c33c36f44cf681976bd886538deba6",
    strip_prefix = "rules_python-0.8.0",
    url = "https://github.com/bazelbuild/rules_python/archive/refs/tags/0.8.0.tar.gz",
)

load("@rules_python//python:pip.bzl", "pip_install")

# For `numpy_test.py`
pip_install(
    requirements = "//python:requirements.txt"
)

bazel_skylib_workspace()

load("@rules_pkg//:deps.bzl", "rules_pkg_dependencies")
rules_pkg_dependencies()

# For `kt_jvm_library`
load("@io_bazel_rules_kotlin//kotlin:repositories.bzl", "kotlin_repositories")
kotlin_repositories()

load("@io_bazel_rules_kotlin//kotlin:core.bzl", "kt_register_toolchains")
kt_register_toolchains()

load("@upb//bazel:workspace_deps.bzl", "upb_deps")
upb_deps()

load("@upb//bazel:system_python.bzl", "system_python")
system_python(name = "local_config_python")

bind(
    name = "python_headers",
    actual = "@local_config_python//:python_headers",
)
