# Define custom utilities
# Test for OSX with [ -n "$IS_OSX" ]

function pre_build {
    # Any stuff that you need to do before you start building the wheels
    # Runs in the root directory of this repository.
    pushd protobuf

    if [ "$PLAT" == "aarch64" ]
    then
      local configure_host_flag="--host=aarch64"
    fi

    # Build protoc and libprotobuf
    ./autogen.sh
    CXXFLAGS="-fPIC -g -O2" ./configure $configure_host_flag
    make -j8

    if [ "$PLAT" == "aarch64" ]
    then
      # we are crosscompiling for aarch64 while running on x64
      # the simplest way for build_py command to be able to generate
      # the protos is by running the protoc process under
      # an emulator. That way we don't have to build a x64 version
      # of protoc. The qemu-arm emulator is already included
      # in the dockcross docker image.
      # Running protoc under an emulator is fast as protoc doesn't
      # really do much.

      # create a simple shell wrapper that runs crosscompiled protoc under qemu
      echo '#!/bin/bash' >protoc_qemu_wrapper.sh
      echo 'exec qemu-aarch64 "../src/protoc" "$@"' >>protoc_qemu_wrapper.sh
      chmod ugo+x protoc_qemu_wrapper.sh

      # PROTOC variable is by build_py step that runs under ./python directory
      export PROTOC=../protoc_qemu_wrapper.sh
    fi

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

    if [ "$PLAT" == "aarch64" ]
    then
      # when crosscompiling for aarch64, --plat-name needs to be set explicitly
      # to end up with correctly named wheel file
      # the value should be manylinuxABC_ARCH and dockcross docker image
      # conveniently provides the value in the AUDITWHEEL_PLAT env
      local plat_name_flag="--plat-name=$AUDITWHEEL_PLAT"

      # override the value of EXT_SUFFIX to make sure the crosscompiled .so files in the wheel have the correct filename suffix
      export PROTOCOL_BUFFERS_OVERRIDE_EXT_SUFFIX="$(python -c 'import sysconfig; print(sysconfig.get_config_var("EXT_SUFFIX").replace("-x86_64-linux-gnu.so", "-aarch64-linux-gnu.so"))')"
    fi

    python setup.py bdist_wheel --cpp_implementation --compile_static_extension $plat_name_flag
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

if [ "$PLAT" == "aarch64" ]
then
  # when crosscompiling for aarch64, override the default multibuild's repair_wheelhouse logic
  # since "auditwheel repair" doesn't work for crosscompiled wheels
  function repair_wheelhouse {
      echo "Skipping repair_wheelhouse since auditwheel requires build architecture to match wheel architecture."
  }
fi
