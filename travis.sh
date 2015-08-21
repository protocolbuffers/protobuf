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
  cd conformance && make test_csharp && cd ..
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
  cd java && mvn test && cd ..
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
  build_java
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
  sudo pip install tox
  # Only install Python2.6 on Linux.
  if [ $(uname -s) == "Linux" ]; then
    sudo apt-get install -y python-software-properties # for apt-add-repository
    sudo apt-add-repository -y ppa:fkrull/deadsnakes
    sudo apt-get update -qq
    sudo apt-get install -y python2.6 python2.6-dev
  fi
}


build_python() {
  internal_build_cpp
  internal_install_python_deps
  cd python
  # Only test Python 2.6 on Linux
  if [ $(uname -s) == "Linux" ]; then
    envlist=py26-python,py27-python
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
  # Only test Python 2.6 on Linux
  if [ $(uname -s) == "Linux" ]; then
    envlist=py26-cpp,py27-cpp
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
