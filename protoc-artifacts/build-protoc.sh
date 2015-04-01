#!/bin/bash

# Builds protoc executable into target/protoc.exe
# To be run from Maven.
# Usage: build-protoc.sh <OS> <ARCH>
# <OS> and <ARCH> are ${os.detected.name} and ${os.detected.arch} from os-maven-plugin
OS=$1
ARCH=$2

# Under Cygwin, bash doesn't have these in PATH when called from Maven which
# runs in Windows version of Java.
export PATH="/bin:/usr/bin:$PATH"

############################################################################
# Helper functions
############################################################################
E_PARAM_ERR=98
E_ASSERT_FAILED=99

fail()
{
  echo "Error: $1"
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
############################################################################

echo "Building protoc, OS=$OS ARCH=$ARCH"

cd $(dirname "$0")
WORKING_DIR=$(pwd)
CONFIGURE_ARGS="--disable-shared"

MAKE_TARGET="protoc"
if [[ "$OS" == windows ]]; then
  MAKE_TARGET="${MAKE_TARGET}.exe"
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
elif [[ "$(uname)" == Linux* ]]; then
  assertEq "$OS" linux $LINENO
  if [[ "$ARCH" == x86_64 ]]; then
    CONFIGURE_ARGS="$CONFIGURE_ARGS --host=x86_64-linux-gnu"
  elif [[ "$ARCH" == x86_32 ]]; then
    CONFIGURE_ARGS="$CONFIGURE_ARGS --host=i686-linux-gnu"
  else
    fail "Unsupported arch by CYGWIN: $ARCH"
  fi
elif [[ "$(uname)" == Darwin* ]]; then
  assertEq "$OS" osx $LINENO
else
  fail "Unsupported system: $(uname)"
fi

# Override the default value set in configure.ac that has '-g' which produces
# huge binary.
export CXXFLAGS="-DNDEBUG"

# Statically link libgcc and libstdc++.
# -s to produce stripped binary
export LDFLAGS="-static-libgcc -static-libstdc++ -s"

cd "$WORKING_DIR"/.. && ./configure $CONFIGURE_ARGS &&
  cd src && make clean && make $MAKE_TARGET &&
  cd "$WORKING_DIR" && mkdir -p target &&
  (cp ../src/protoc target/protoc.exe || cp ../src/protoc.exe target/protoc.exe)
