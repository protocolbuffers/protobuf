#!/bin/bash

# Builds protoc executable into target/<OS>/<ARCH>/protoc.exe; optionally builds
# protoc plugins into target/<OS>/<ARCH>/protoc-gen-*.exe
#
# Usage: ./build-protoc.sh <OS> <ARCH> <TARGET>
#
# <TARGET> can be "protoc" or "protoc-gen-javalite". Supported <OS> <ARCH>
# combinations:
#   HOST   <OS>    <ARCH>   <COMMENT>
#   cygwin windows x86_32   Requires: i686-w64-mingw32-gcc
#   cygwin windows x86_64   Requires: x86_64-w64-mingw32-gcc
#   linux  linux   aarch_64 Requires: g++-aarch64-linux-gnu
#   linux  linux   x86_32
#   linux  linux   x86_64
#   linux  windows x86_32   Requires: i686-w64-mingw32-gcc
#   linux  windows x86_64   Requires: x86_64-w64-mingw32-gcc
#   macos  osx     x86_32
#   macos  osx     x86_64
#   mingw  windows x86_32
#   mingw  windows x86_64

OS=$1
ARCH=$2
BAZEL_TARGET=$3

if [[ $# < 3 ]]; then
  echo "Not enough arguments provided."
  exit 1
fi

if [[ $BAZEL_TARGET != protoc ]]; then
    echo "Target ""$BAZEL_TARGET"" invalid."
    exit 1
fi

# Under Cygwin, bash doesn't have these in PATH when called from Maven which
# runs in Windows version of Java.
export PATH="/bin:/usr/bin:$PATH"

############################################################################
# Helper functions
############################################################################
E_PARAM_ERR=98
E_ASSERT_FAILED=99

# Usage:
fail()
{
  echo "ERROR: $1"
  echo
  exit $E_ASSERT_FAILED
}

# Usage: assertEq VAL1 VAL2 $LINENO
assertEq ()
{
  lineno=$3
  if [ -z "$lineno" ]; then
    echo "lineno not given"
    exit $E_PARAM_ERR
  fi

  if [[ "$1" != "$2" ]]; then
    echo "Assertion failed:  \"$1\" == \"$2\""
    echo "File \"$0\", line $lineno"    # Give name of file and line number.
    exit $E_ASSERT_FAILED
  fi
}

# Checks the artifact is for the expected architecture
# Usage: checkArch <path-to-protoc>
checkArch ()
{
  echo
  echo "Checking file format ..."
  if [[ "$OS" == windows || "$OS" == linux ]]; then
    format="$(objdump -f "$1" | grep -o "file format .*$" | grep -o "[^ ]*$")"
    echo Format=$format
    if [[ "$OS" == linux ]]; then
      host_machine="$(uname -m)";
      if [[ "$ARCH" == x86_32 ]]; then
        assertEq $format "elf32-i386" $LINENO
      elif [[ "$ARCH" == x86_64 ]]; then
        assertEq $format "elf64-x86-64" $LINENO
      elif [[ "$ARCH" == aarch_64 ]]; then
        assertEq $format "elf64-little" $LINENO
      elif [[ "$ARCH" == s390_64 ]]; then
	if [[ $host_machine == s390x ]];then
	  assertEq $format "elf64-s390" $LINENO
	else
          assertEq $format "elf64-big" $LINENO
	fi
      elif [[ "$ARCH" == ppcle_64 ]]; then
	if [[ $host_machine == ppc64le ]];then
	  assertEq $format "elf64-powerpcle" $LINENO
	else
          assertEq $format "elf64-little" $LINENO
	fi
      else
        fail "Unsupported arch: $ARCH"
      fi
    else
      # $OS == windows
      if [[ "$ARCH" == x86_32 ]]; then
        assertEq $format "pei-i386" $LINENO
      elif [[ "$ARCH" == x86_64 ]]; then
        assertEq $format "pei-x86-64" $LINENO
      else
        fail "Unsupported arch: $ARCH"
      fi
    fi
  elif [[ "$OS" == osx ]]; then
    format="$(file -b "$1" | grep -o "[^ ]*$")"
    echo Format=$format
    if [[ "$ARCH" == x86_32 ]]; then
      assertEq $format "i386" $LINENO
    elif [[ "$ARCH" == x86_64 ]]; then
      assertEq $format "x86_64" $LINENO
    else
      fail "Unsupported arch: $ARCH"
    fi
  else
    fail "Unsupported system: $OS"
  fi
  echo
}

# Checks the dependencies of the artifact. Artifacts should only depend on
# system libraries.
# Usage: checkDependencies <path-to-protoc>
checkDependencies ()
{
  if [[ "$OS" == windows ]]; then
    dump_cmd='objdump -x '"$1"' | fgrep "DLL Name"'
    white_list="KERNEL32\.dll\|msvcrt\.dll"
  elif [[ "$OS" == linux ]]; then
    host_machine="$(uname -m)";
    dump_cmd='ldd '"$1"
    if [[ "$ARCH" == x86_32 ]]; then
      white_list="linux-gate\.so\.1\|libpthread\.so\.0\|libm\.so\.6\|libc\.so\.6\|libgcc_s\.so\.1\|libstdc++\.so\.6\|ld-linux\.so\.2"
    elif [[ "$ARCH" == x86_64 ]]; then
      white_list="linux-vdso\.so\.1\|libpthread\.so\.0\|libm\.so\.6\|libc\.so\.6\|libgcc_s\.so\.1\|libstdc++\.so\.6\|ld-linux-x86-64\.so\.2"
    elif [[ "$ARCH" == s390_64 ]]; then
      if [[ $host_machine != s390x ]];then
        dump_cmd='objdump -p '"$1"' | grep NEEDED'
      fi
      white_list="linux-vdso64\.so\.1\|libpthread\.so\.0\|libm\.so\.6\|libc\.so\.6\|libgcc_s\.so\.1\|libstdc++\.so\.6\|libz\.so\.1\|ld64\.so\.1"
    elif [[ "$ARCH" == ppcle_64 ]]; then
      if [[ $host_machine != ppc64le ]];then
        dump_cmd='objdump -p '"$1"' | grep NEEDED'
      fi
      white_list="linux-vdso64\.so\.1\|libpthread\.so\.0\|libm\.so\.6\|libc\.so\.6\|libgcc_s\.so\.1\|libstdc++\.so\.6\|libz\.so\.1\|ld64\.so\.2"
    elif [[ "$ARCH" == aarch_64 ]]; then
      dump_cmd='objdump -p '"$1"' | grep NEEDED'
      white_list="libpthread\.so\.0\|libm\.so\.6\|libc\.so\.6\|libgcc_s\.so\.1\|libstdc++\.so\.6\|ld-linux-aarch64\.so\.1"
    fi
  elif [[ "$OS" == osx ]]; then
    dump_cmd='otool -L '"$1"' | fgrep dylib'
    white_list="libz\.1\.dylib\|libstdc++\.6\.dylib\|libSystem\.B\.dylib"
  fi
  if [[ -z "$white_list" || -z "$dump_cmd" ]]; then
    fail "Unsupported platform $OS-$ARCH."
  fi
  echo "Checking for expected dependencies ..."
  eval $dump_cmd | grep -i "$white_list" || fail "doesn't show any expected dependencies"
  echo "Checking for unexpected dependencies ..."
  eval $dump_cmd | grep -i -v "$white_list"
  ret=$?
  if [[ $ret == 0 ]]; then
    fail "found unexpected dependencies (listed above)."
  elif [[ $ret != 1 ]]; then
    fail "Error when checking dependencies."
  fi  # grep returns 1 when "not found", which is what we expect
  echo "Dependencies look good."
  echo
}
############################################################################

echo "Building protoc, OS=$OS ARCH=$ARCH TARGET=$BAZEL_TARGET"

BAZEL_ARGS=("--dynamic_mode=off" "--compilation_mode=opt" "--copt=-g0" "--copt=-fpic")

if [[ "$OS" == windows ]]; then
  BAZEL_TARGET="${BAZEL_TARGET}.exe"
fi

if [[ "$(uname)" == CYGWIN* ]]; then
  assertEq "$OS" windows $LINENO
  # Use mingw32 compilers because executables produced by Cygwin compiler
  # always have dependency on Cygwin DLL.
  if [[ "$ARCH" == x86_64 ]]; then
    CONFIGURE_ARGS="$CONFIGURE_ARGS --host=x86_64-w64-mingw32"
  elif [[ "$ARCH" == x86_32 ]]; then
    CONFIGURE_ARGS="$CONFIGURE_ARGS --host=i686-pc-mingw32"
  else
    fail "Unsupported arch by CYGWIN: $ARCH"
  fi
elif [[ "$(uname)" == MINGW32* ]]; then
  assertEq "$OS" windows $LINENO
  assertEq "$ARCH" x86_32 $LINENO
elif [[ "$(uname)" == MINGW64* ]]; then
  assertEq "$OS" windows $LINENO
  assertEq "$ARCH" x86_64 $LINENO
elif [[ "$(uname)" == Linux* ]]; then
  if [[ "$OS" == linux ]]; then
    BAZEL_ARGS+=("--config=linux-$ARCH")
  elif [[ "$OS" == windows ]]; then
    # Cross-compilation for Windows
    if [[ "$ARCH" == x86_64 ]]; then
      BAZEL_ARGS+=("--config=win64")
    elif [[ "$ARCH" == x86_32 ]]; then
      BAZEL_ARGS+=("--config=win32")
    else
      fail "Unsupported arch: $ARCH"
    fi
  else
    fail "Cannot build $OS on $(uname)"
  fi
elif [[ "$(uname)" == Darwin* ]]; then
  assertEq "$OS" osx $LINENO
  # Make the binary compatible with OSX 10.7 and later
  BAZEL_ARGS+=("--config=osx-$ARCH" "--macos_minimum_os=10.7")
else
  fail "Unsupported system: $(uname)"
fi

# Nested double quotes are unintuitive, but it works.
cd "$(dirname "$0")"

WORKING_DIR="$(pwd)"
TARGET_FILE="target/$OS/$ARCH/$BAZEL_TARGET.exe"
DOCKER_IMAGE=us-docker.pkg.dev/protobuf-build/release-containers/linux@sha256:85fd92cce31eb9446ca85d108b424b8167cea3469b58f2b9a9d30e37fc467a00
gcloud components update --quiet
gcloud auth configure-docker us-docker.pkg.dev --quiet

tmpfile=$(mktemp -u) &&
docker run --cidfile $tmpfile -v $WORKING_DIR/../../..:/workspace $DOCKER_IMAGE \
    build //:$BAZEL_TARGET "${BAZEL_ARGS[@]}" &&
mkdir -p $(dirname $TARGET_FILE) &&
docker cp \
  `cat $tmpfile`:/workspace/bazel-bin/$BAZEL_TARGET $TARGET_FILE || exit 1

if [[ "$OS" == osx ]]; then
  # Since Mac linker doesn't accept "-s", we need to run strip
  strip $TARGET_FILE || exit 1
fi

checkArch $TARGET_FILE && checkDependencies $TARGET_FILE
