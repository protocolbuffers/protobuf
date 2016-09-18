new_http_archive(
  name = "gmock_archive",
  url = "https://github.com/google/googletest/archive/release-1.8.0.zip",
  sha256 = "f3ed3b58511efd272eb074a3a6d6fb79d7c2e6a0e374323d1e6bcbcc1ef141bf",
  strip_prefix = "googletest-release-1.8.0",
  build_file = "gmock.BUILD",
)

new_http_archive(
  name = "six_archive",
  url = "https://pypi.python.org/packages/source/s/six/six-1.10.0.tar.gz#md5=34eed507548117b2ab523ab14b2f8b55",
  sha256 = "105f8d68616f8248e24bf0e9372ef04d3cc10104f1980f54d57b2ce73a5ad56a",
  build_file = "six.BUILD",
)

bind(
  name = "gtest",
  actual = "@gmock_archive//:gtest",
)

bind(
  name = "gtest_main",
  actual = "@gmock_archive//:gtest_main",
)

bind(
  name = "six",
  actual = "@six_archive//:six",
)
