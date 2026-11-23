# Bazel CI test

The `.bazelci/presubmit.yml` file is used by https://buildkite.com/bazel/protobuf
and Bazel's downstream CI pipeline to build and test protobuf with Bazel on
Bazel CI. The `verify_targets` task should stay in sync with `.bcr/presubmit.yml`,
which runs before publishing protobuf to BCR, while the remaining tasks run root
repository tests across Linux, macOS, and Windows.
