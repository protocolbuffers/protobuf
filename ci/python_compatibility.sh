#!/bin/bash

set -ex

PROTOC_VERSION=$1
PROTOC_DOWNLOAD=https://github.com/protocolbuffers/protobuf/releases/download/v$PROTOC_VERSION/protoc-$PROTOC_VERSION-linux-x86_64.zip
PY_SITE_PACKAGES=$(python -c 'import site; print(site.getsitepackages()[0])')

rm -rf protoc-old
mkdir protoc-old
pushd protoc-old
wget $PROTOC_DOWNLOAD
unzip *.zip
PROTOC=$(pwd)/bin/protoc
popd

# protoc prior to 28.0 doesn't support inf/nan option values.
sed -i 's/\(inf\|nan\)/0/g' src/google/protobuf/unittest_custom_options.proto

bazel build //python:copied_test_proto_files //python:copied_wkt_proto_files

COMPAT_COPIED_PROTOS=(
  # Well-known types give good build coverage
  any
  api
  duration
  empty
  field_mask
  source_context
  struct
  timestamp
  type
  wrappers
  # These protos are used in tests of custom options handling.
  unittest_custom_options
  unittest_import
)

for proto in ${COMPAT_COPIED_PROTOS[@]}; do
  $PROTOC --python_out=$PY_SITE_PACKAGES \
    bazel-bin/python/google/protobuf/$proto.proto \
    -Ibazel-bin/python
done

COMPAT_PROTOS=(
  # All protos without transitive dependencies on edition 2023+.
  descriptor_pool_test1
  descriptor_pool_test2
  factory_test1
  factory_test2
  file_options_test
  import_test_package/import_public
  import_test_package/import_public_nested
  import_test_package/inner
  import_test_package/outer
  message_set_extensions
  missing_enum_values
  more_extensions
  more_extensions_dynamic
  more_messages
  no_package
  packed_field_test
  test_bad_identifiers
  test_proto2
  test_proto3_optional
)

for proto in ${COMPAT_PROTOS[@]}; do
  $PROTOC --python_out=$PY_SITE_PACKAGES \
    python/google/protobuf/internal/$proto.proto \
    -Ipython
done

# Exclude pybind11 tests because they require C++ code that doesn't ship with
# our test wheels.
TEST_EXCLUSIONS="_pybind11_test.py"
TESTS=$(pip show -f protobuftests | grep -i _test.py | grep --invert-match $TEST_EXCLUSIONS | sed 's,/,.,g' | sed -E 's,.py$,,g')
python -m unittest -v ${TESTS[@]}
