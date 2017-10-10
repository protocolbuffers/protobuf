new_git_repository(
    name = "googletest",
    build_file = "gmock.BUILD",
    remote = "https://github.com/google/googletest",
    tag = "release-1.8.0",
)

new_http_archive(
    name = "six_archive",
    build_file = "six.BUILD",
    sha256 = "105f8d68616f8248e24bf0e9372ef04d3cc10104f1980f54d57b2ce73a5ad56a",
    url = "https://pypi.python.org/packages/source/s/six/six-1.10.0.tar.gz#md5=34eed507548117b2ab523ab14b2f8b55",
)

bind(
    name = "python_headers",
    actual = "//util/python:python_headers",
)

bind(
    name = "gtest",
    actual = "@googletest//:gtest",
)

bind(
    name = "gtest_main",
    actual = "@googletest//:gtest_main",
)

bind(
    name = "six",
    actual = "@six_archive//:six",
)

maven_jar(
    name = "guava_maven",
    artifact = "com.google.guava:guava:18.0",
)

bind(
    name = "guava",
    actual = "@guava_maven//jar",
)

maven_jar(
    name = "gson_maven",
    artifact = "com.google.code.gson:gson:2.7",
)

bind(
    name = "gson",
    actual = "@gson_maven//jar",
)
