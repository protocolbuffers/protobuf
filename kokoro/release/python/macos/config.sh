# Define custom utilities
# Test for OSX with [ -n "$IS_OSX" ]

function remove_travis_ve_pip {
    # Removing the system virtualenv or pip can be very problematic for
    # macOS on Kokoro, so just leave them be.
    :;
}

function install_pip {
    check_python
    PIP_CMD="sudo $PYTHON_EXE -m pip${pip_args:+ $pip_args}"
    $PIP_CMD install --upgrade pip
}

function install_virtualenv {
    check_python
    check_pip
    $PIP_CMD install --upgrade virtualenv
    VIRTUALENV_CMD="$PYTHON_EXE -m virtualenv"
}

function pre_build {
    # Any stuff that you need to do before you start building the wheels
    # Runs in the root directory of this repository.
    pushd protobuf

    # Build protoc and protobuf libraries
    bazel build //:protoc
    export PROTOC=$PWD/bazel-bin/protoc
    mkdir src/.libs
    ln -s $PWD/bazel-bin/libprotobuf.a src/.libs/libprotobuf.a
    ln -s $PWD/bazel-bin/libprotobuf_lite.a src/.libs/libprotobuf-lite.a

    # Generate python dependencies.
    pushd python
    python setup.py build_py
    popd

    popd
}

function bdist_wheel_cmd {
    # Builds wheel with bdist_wheel, puts into wheelhouse
    #
    # It may sometimes be useful to use bdist_wheel for the wheel building
    # process.  For example, versioneer has problems with versions which are
    # fixed with bdist_wheel:
    # https://github.com/warner/python-versioneer/issues/121
    local abs_wheelhouse=$1

    # Modify build version
    pwd
    ls
    python setup.py bdist_wheel --cpp_implementation --compile_static_extension
    cp dist/*.whl $abs_wheelhouse
}

function build_wheel {
    build_wheel_cmd "bdist_wheel_cmd" $@
}

function run_tests {
    # Runs tests on installed distribution from an empty directory
    python --version
    python -c "from google.protobuf.pyext import _message;"
}
