#!/usr/bin/env bash

build_cpp() {
  ./autogen.sh
  ./configure
  make -j2
  make check -j2
  cd conformance && make test_cpp && cd ..
}

build_cpp_distcheck() {
  ./autogen.sh
  ./configure
  make distcheck -j2
}

build_csharp() {
  # Install latest version of Mono
  sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF
  echo "deb http://download.mono-project.com/repo/debian wheezy main" | sudo tee /etc/apt/sources.list.d/mono-xamarin.list
  echo "deb http://download.mono-project.com/repo/debian wheezy-libtiff-compat main" | sudo tee -a /etc/apt/sources.list.d/mono-xamarin.list
  sudo apt-get update -qq
  sudo apt-get install -qq mono-devel referenceassemblies-pcl nunit
  wget www.nuget.org/NuGet.exe -O nuget.exe

  (cd csharp/src; mono ../../nuget.exe restore)
  csharp/buildall.sh
}

use_java() {
  if [ `uname` != "Linux" ]; then
    # It's nontrivial to programmatically install a new JDK from the command
    # line on OS X, so we rely on testing on Linux for Java code.
    echo "Java not tested on OS X."
    exit 0  # success
  fi
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
  ./autogen.sh
  ./configure
  make -j2
  cd java && mvn test && cd ..
  cd conformance && make test_java && cd ..
}

build_javanano() {
  # Java build needs `protoc`.
  ./autogen.sh
  ./configure
  make -j2
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

build_python() {
  ./autogen.sh
  ./configure
  make -j2
  cd python
  python setup.py build
  python setup.py test
  python setup.py sdist
  sudo pip install virtualenv && virtualenv /tmp/protoenv && /tmp/protoenv/bin/pip install dist/*
  cd ..
}

build_python_cpp() {
  ./autogen.sh
  ./configure
  make -j2
  export   LD_LIBRARY_PATH=../src/.libs # for Linux
  export DYLD_LIBRARY_PATH=../src/.libs # for OS X
  cd python
  python setup.py build --cpp_implementation
  python setup.py test --cpp_implementation
  python setup.py sdist --cpp_implementation
  sudo pip install virtualenv && virtualenv /tmp/protoenv && /tmp/protoenv/bin/pip install dist/*
  cd ..
}

build_ruby19() {
  cd ruby && bash travis-test.sh ruby-1.9 && cd ..
}
build_ruby20() {
  cd ruby && bash travis-test.sh ruby-2.0 && cd ..
}
build_ruby21() {
  cd ruby && bash travis-test.sh ruby-2.1 && cd ..
}
build_ruby22() {
  cd ruby && bash travis-test.sh ruby-2.2 && cd ..
}
build_jruby() {
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
