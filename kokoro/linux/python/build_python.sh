#!/bin/bash -eux
#
# This script builds the native extension or pure Python implementations
# of Protocol Buffers.
#
# This script should be run from the root of the protobuf repo.

# The following values can be overridden by environment variable:
#
# PROTOBUF_PYTHON_RUNTIME
#   "cpp" (default, C++ native extension) or "python" (pure Python)
: ${PROTOBUF_PYTHON_RUNTIME:=cpp}

# Validate the runtime selection.
if [[ ${PROTOBUF_PYTHON_RUNTIME} == "cpp" ]]; then
  BUILD_NATIVE=1
elif [[ ${PROTOBUF_PYTHON_RUNTIME} != "python" ]]; then
  echo -e "Unrecognized PROTOBUF_PYTHON_RUNTIME: ${PROTOBUF_PYTHON_RUNTIME@Q}" >&2
  exit 1
fi

# Builds the protobuf library. At a minimum, this is needed for protoc.
build_libprotobuf() {
  # To build the C++ extension, we must make sure to build PIC.
  ./autogen.sh
  ./configure CXXFLAGS="-fPIC"
  make -j$(nproc)
  export PROTOC=$PWD/src/protoc
}

# Builds the pure-Python runtime.
build_runtime() {
  (
    cd python
    python setup.py build
    pip install -e .
  )
}

# Builds the native extension runtime.
build_runtime_cpp() {
  (
    cd python
    # The native extension requires passing extra flags to setup.py.
    setup_py_args=(--cpp_implementation --compile_static_extension)
    python setup.py build_py "${setup_py_args[@]}"
    # Build the extension in-place so we can use `pip install -e`.
    python setup.py build_ext --inplace "${setup_py_args[@]}"
    python setup.py build "${setup_py_args[@]}"
    pip install -e .
  )
}

# Ensures the correct implementation is used.
validate_implementation() {
  python <<EOF
from google.protobuf.internal import api_implementation
assert(api_implementation._implementation_type == "${PROTOBUF_PYTHON_RUNTIME}")
EOF
}

# Runs tests.
run_tests() {
  # Discover the unit tests to run.
  python -m unittest discover python/google/protobuf/internal "*_test.py"
  # Run conformance test.
  # There are different targets for native and pure Python.
  make -C conformance test_python${BUILD_NATIVE:+_cpp}
}

# Build and run tests.
build_libprotobuf
build_runtime${BUILD_NATIVE:+_cpp}
validate_implementation
run_tests
