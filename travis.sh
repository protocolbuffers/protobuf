#!/usr/bin/env bash

# Note: travis currently does not support testing more than one language so the
# .travis.yml cheats and claims to only be cpp.  If they add multiple language
# support, this should probably get updated to install steps and/or
# rvm/gemfile/jdk/etc. entries rather than manually doing the work.

# .travis.yml uses matrix.exclude to block the cases where app-get can't be
# use to install things.

# For when some other test needs the C++ main build, including protoc and
# libprotobuf.
internal_build_cpp() {
  if [ $(uname -s) == "Linux" ]; then
    # Install GCC 4.8 to replace the default GCC 4.6. We need 4.8 for more
    # decent C++ 11 support in order to compile conformance tests.
    sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
    sudo apt-get update -qq
    sudo apt-get install -qq g++-4.8
    export CXX="g++-4.8" CC="gcc-4.8"
  fi

  ./autogen.sh
  ./configure
  make -j2
}

build_cpp() {
  internal_build_cpp
  make check -j2
  cd conformance && make test_cpp && cd ..
}

build_cpp_distcheck() {
  ./autogen.sh
  ./configure
  make distcheck -j2
}

build_csharp() {
  # Just for the conformance tests. We don't currently
  # need to really build protoc, but it's simplest to keep with the
  # conventions of the other builds.
  internal_build_cpp

  # Install latest version of Mono
  sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF
  echo "deb http://download.mono-project.com/repo/debian wheezy main" | sudo tee /etc/apt/sources.list.d/mono-xamarin.list
  echo "deb http://download.mono-project.com/repo/debian wheezy-libtiff-compat main" | sudo tee -a /etc/apt/sources.list.d/mono-xamarin.list
  sudo apt-get update -qq
  sudo apt-get install -qq mono-devel referenceassemblies-pcl nunit
  wget www.nuget.org/NuGet.exe -O nuget.exe

  (cd csharp/src; mono ../../nuget.exe restore)
  csharp/buildall.sh
  # TODO(xiaofeng): The conformance tests are disable because the testee program
  # crashes on some inputs.
  # cd conformance && make test_csharp && cd ..
}

build_golang() {
  # Go build needs `protoc`.
  internal_build_cpp
  # Add protoc to the path so that the examples build finds it.
  export PATH="`pwd`/src:$PATH"

  # Install Go and the Go protobuf compiler plugin.
  sudo apt-get update -qq
  sudo apt-get install -qq golang
  export GOPATH="$HOME/gocode"
  mkdir -p "$GOPATH/src/github.com/google"
  ln -s "`pwd`" "$GOPATH/src/github.com/google/protobuf"
  export PATH="$GOPATH/bin:$PATH"
  go get github.com/golang/protobuf/protoc-gen-go

  cd examples && make gotest && cd ..
}

use_java() {
  version=$1
  case "$version" in
    jdk6)
      sudo apt-get install openjdk-6-jdk
      export PATH=/usr/lib/jvm/java-6-openjdk-amd64/bin:$PATH
      ;;
    jdk7)
      sudo apt-get install openjdk-7-jdk
      export PATH=/usr/lib/jvm/java-7-openjdk-amd64/bin:$PATH
      ;;
    oracle7)
      sudo apt-get install python-software-properties # for apt-add-repository
      echo "oracle-java7-installer shared/accepted-oracle-license-v1-1 select true" | \
        sudo debconf-set-selections
      yes | sudo apt-add-repository ppa:webupd8team/java
      yes | sudo apt-get install oracle-java7-installer
      export PATH=/usr/lib/jvm/java-7-oracle/bin:$PATH
      ;;
  esac

  which java
  java -version
}

build_java() {
  # Java build needs `protoc`.
  internal_build_cpp
  cd java && mvn test && mvn install
  cd util && mvn test
  cd ../..
}

build_java_with_conformance_tests() {
  # Java build needs `protoc`.
  internal_build_cpp
  cd java && mvn test && mvn install
  cd util && mvn test && mvn assembly:single
  cd ../..
  cd conformance && make test_java && cd ..
}

build_javanano() {
  # Java build needs `protoc`.
  internal_build_cpp
  cd javanano && mvn test && cd ..
}

build_java_jdk6() {
  use_java jdk6
  build_java
}
build_java_jdk7() {
  use_java jdk7
  build_java_with_conformance_tests
}
build_java_oracle7() {
  use_java oracle7
  build_java
}

build_javanano_jdk6() {
  use_java jdk6
  build_javanano
}
build_javanano_jdk7() {
  use_java jdk7
  build_javanano
}
build_javanano_oracle7() {
  use_java oracle7
  build_javanano
}

internal_install_python_deps() {
  # Install tox (OS X doesn't have pip).
  if [ $(uname -s) == "Darwin" ]; then
    sudo easy_install tox
  else
    sudo pip install tox
  fi
  # Only install Python2.6/3.x on Linux.
  if [ $(uname -s) == "Linux" ]; then
    sudo apt-get install -y python-software-properties # for apt-add-repository
    sudo apt-add-repository -y ppa:fkrull/deadsnakes
    sudo apt-get update -qq
    sudo apt-get install -y python2.6 python2.6-dev
    sudo apt-get install -y python3.3 python3.3-dev
    sudo apt-get install -y python3.4 python3.4-dev
  fi
}

internal_objectivec_common () {
  # Make sure xctool is up to date. Adapted from
  #  http://docs.travis-ci.com/user/osx-ci-environment/
  # We don't use a before_install because we test multiple OSes.
  brew update
  brew outdated xctool || brew upgrade xctool
  # Reused the build script that takes care of configuring and ensuring things
  # are up to date. Xcode and conformance tests will be directly invoked.
  objectivec/DevTools/full_mac_build.sh \
      --core-only --skip-xcode --skip-objc-conformance
}

internal_xctool_debug_and_release() {
  xctool -configuration Debug "$@"
  xctool -configuration Release "$@"
}

build_objectivec_ios() {
  internal_objectivec_common
  # https://github.com/facebook/xctool/issues/509 - unlike xcodebuild, xctool
  # doesn't support >1 destination, so we have to build first and then run the
  # tests one destination at a time.
  internal_xctool_debug_and_release \
    -project objectivec/ProtocolBuffers_iOS.xcodeproj \
    -scheme ProtocolBuffers \
    -sdk iphonesimulator \
    build-tests
  IOS_DESTINATIONS=(
    "platform=iOS Simulator,name=iPhone 4s,OS=8.1" # 32bit
    "platform=iOS Simulator,name=iPhone 6,OS=9.1" # 64bit
    "platform=iOS Simulator,name=iPad 2,OS=8.1" # 32bit
    "platform=iOS Simulator,name=iPad Air,OS=9.1" # 64bit
  )
  for i in "${IOS_DESTINATIONS[@]}" ; do
    internal_xctool_debug_and_release \
      -project objectivec/ProtocolBuffers_iOS.xcodeproj \
      -scheme ProtocolBuffers \
      -sdk iphonesimulator \
      -destination "${i}" \
      run-tests
  done
}

build_objectivec_osx() {
  internal_objectivec_common
  internal_xctool_debug_and_release \
    -project objectivec/ProtocolBuffers_OSX.xcodeproj \
    -scheme ProtocolBuffers \
    -destination "platform=OS X,arch=x86_64" \
    test
  cd conformance && make test_objc && cd ..
}

build_python() {
  internal_build_cpp
  internal_install_python_deps
  cd python
  # Only test Python 2.6/3.x on Linux
  if [ $(uname -s) == "Linux" ]; then
    envlist=py\{26,27,33,34\}-python
  else
    envlist=py27-python
  fi
  tox -e $envlist
  cd ..
}

build_python_cpp() {
  internal_build_cpp
  internal_install_python_deps
  export LD_LIBRARY_PATH=../src/.libs # for Linux
  export DYLD_LIBRARY_PATH=../src/.libs # for OS X
  cd python
  # Only test Python 2.6/3.x on Linux
  if [ $(uname -s) == "Linux" ]; then
    # py26 is currently disabled due to json_format
    envlist=py\{27,33,34\}-cpp
  else
    envlist=py27-cpp
  fi
  tox -e $envlist
  cd ..
}

build_ruby19() {
  internal_build_cpp  # For conformance tests.
  cd ruby && bash travis-test.sh ruby-1.9 && cd ..
}
build_ruby20() {
  internal_build_cpp  # For conformance tests.
  cd ruby && bash travis-test.sh ruby-2.0 && cd ..
}
build_ruby21() {
  internal_build_cpp  # For conformance tests.
  cd ruby && bash travis-test.sh ruby-2.1 && cd ..
}
build_ruby22() {
  internal_build_cpp  # For conformance tests.
  cd ruby && bash travis-test.sh ruby-2.2 && cd ..
}
build_jruby() {
  internal_build_cpp  # For conformance tests.
  cd ruby && bash travis-test.sh jruby && cd ..
}

build_javascript() {
  internal_build_cpp
  cd js && npm install && npm test && cd ..
}

# -------- main --------

if [ "$#" -ne 1 ]; then
  echo "
Usage: $0 { cpp |
            csharp |
            java_jdk6 |
            java_jdk7 |
            java_oracle7 |
            javanano_jdk6 |
            javanano_jdk7 |
            javanano_oracle7 |
            objectivec_ios |
            objectivec_osx |
            python |
            python_cpp |
            ruby_19 |
            ruby_20 |
            ruby_21 |
            ruby_22 |
            jruby }
"
  exit 1
fi

set -e  # exit immediately on error
set -x  # display all commands
eval "build_$1"
