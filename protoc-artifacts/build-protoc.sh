#!/bin/bash

# Builds protoc executable into target/protoc.exe
# To be run from Maven.
# Usage: build-protoc.sh <OS> <ARCH>
# <OS> and <ARCH> are ${os.detected.name} and ${os.detected.arch} from os-maven-plugin
OS=$1
ARCH=$2

if [[ $# < 2 ]]; then
  echo "No arguments provided. This script is intended to be run from Maven."
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

# Checks the artifact is for the expected architecture
# Usage: checkArch <path-to-protoc>
checkArch ()
{
  if [[ "$OS" == windows || "$OS" == linux ]]; then
    format="$(objdump -f "$1" | grep -o "file format .*$" | grep -o "[^ ]*$")"
    if [[ "$OS" == linux ]]; then
      if [[ "$ARCH" == x86_32 ]]; then
        assertEq $format "elf32-i386" $LINENO
      elif [[ "$ARCH" == x86_64 ]]; then
        assertEq $format "elf64-x86-64" $LINENO
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
    assertEq $format "x86_64" $LINENO
  else
    fail "Unsupported system: $(uname)"
  fi
}
############################################################################

echo "Building protoc, OS=$OS ARCH=$ARCH"

# Nested double quotes are unintuitive, but it works.
cd "$(dirname "$0")"

WORKING_DIR=$(pwd)
CONFIGURE_ARGS="--disable-shared"

MAKE_TARGET="protoc"
if [[ "$OS" == windows ]]; then
  MAKE_TARGET="${MAKE_TARGET}.exe"
fi

# Override the default value set in configure.ac that has '-g' which produces
# huge binary.
CXXFLAGS="-DNDEBUG"

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
    CXXFLAGS="$CXXFLAGS -m64"
  elif [[ "$ARCH" == x86_32 ]]; then
    CXXFLAGS="$CXXFLAGS -m32"
  else
    fail "Unsupported arch: $ARCH"
  fi
elif [[ "$(uname)" == Darwin* ]]; then
  assertEq "$OS" osx $LINENO
else
  fail "Unsupported system: $(uname)"
fi

export CXXFLAGS

# Statically link libgcc and libstdc++.
# -s to produce stripped binary.
# And they don't work under Mac.
if [[ "$OS" != osx ]]; then
  export LDFLAGS="-static-libgcc -static-libstdc++ -s"
fi

TARGET_FILE=target/protoc.exe

cd "$WORKING_DIR"/.. && ./configure $CONFIGURE_ARGS &&
  cd src && make clean && make $MAKE_TARGET &&
  cd "$WORKING_DIR" && mkdir -p target &&
  (cp ../src/protoc $TARGET_FILE || cp ../src/protoc.exe $TARGET_FILE) &&
  checkArch $TARGET_FILE
