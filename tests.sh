#!/bin/bash
#
# Build and runs tests for the protobuf project. We use this script to run
# tests on kokoro (Ubuntu and MacOS). It can run locally as well but you
# will need to make sure the required compilers/tools are available.

# For when some other test needs the C++ main build, including protoc and
# libprotobuf.
LAST_RELEASED=3.9.0

internal_build_cpp() {
  if [ -f src/protoc ]; then
    # Already built.
    return
  fi

  # Initialize any submodules.
  git submodule update --init --recursive

  ./autogen.sh
  ./configure CXXFLAGS="-fPIC -std=c++11"  # -fPIC is needed for python cpp test.
                                           # See python/setup.py for more details
  make -j$(nproc)
}

build_cpp() {
  internal_build_cpp
  make check -j$(nproc) || (cat src/test-suite.log; false)
  cd conformance && make test_cpp && cd ..

  # The benchmark code depends on cmake, so test if it is installed before
  # trying to do the build.
  if [[ $(type cmake 2>/dev/null) ]]; then
    # Verify benchmarking code can build successfully.
    cd benchmarks && make cpp-benchmark && cd ..
  else
    echo ""
    echo "WARNING: Skipping validation of the bench marking code, cmake isn't installed."
    echo ""
  fi
}

build_cpp_tcmalloc() {
  internal_build_cpp
  ./configure LIBS=-ltcmalloc && make clean && make \
      PTHREAD_CFLAGS='-pthread -DGOOGLE_PROTOBUF_HEAP_CHECK_DRACONIAN' \
      check
  cd src
  PPROF_PATH=/usr/bin/google-pprof HEAPCHECK=strict ./protobuf-test
}

build_cpp_distcheck() {
  grep -q -- "-Og" src/Makefile.am &&
    echo "The -Og flag is incompatible with Clang versions older than 4.0." &&
    exit 1

  # Initialize any submodules.
  git submodule update --init --recursive
  ./autogen.sh
  ./configure
  make dist

  # List all files that should be included in the distribution package.
  git ls-files | grep "^\(java\|python\|objectivec\|csharp\|js\|ruby\|php\|cmake\|examples\|src/google/protobuf/.*\.proto\)" |\
    grep -v ".gitignore" | grep -v "java/compatibility_tests" | grep -v "java/lite/proguard.pgcfg" |\
    grep -v "python/compatibility_tests" | grep -v "python/docs" | grep -v "python/.repo-metadata.json" |\
    grep -v "csharp/compatibility_tests" > dist.lst
  # Unzip the dist tar file.
  DIST=`ls *.tar.gz`
  tar -xf $DIST
  cd ${DIST//.tar.gz}
  # Check if every file exists in the dist tar file.
  FILES_MISSING=""
  for FILE in $(<../dist.lst); do
    [ -f "$FILE" ] || {
      echo "$FILE is not found!"
      FILES_MISSING="$FILE $FILES_MISSING"
    }
  done
  cd ..
  if [ ! -z "$FILES_MISSING" ]; then
    echo "Missing files in EXTRA_DIST: $FILES_MISSING"
    exit 1
  fi

  # Do the regular dist-check for C++.
  make distcheck -j$(nproc)
}

build_dist_install() {
  # Initialize any submodules.
  git submodule update --init --recursive
  ./autogen.sh
  ./configure
  make dist

  # Unzip the dist tar file and install it.
  DIST=`ls *.tar.gz`
  tar -xf $DIST
  pushd ${DIST//.tar.gz}
  ./configure && make check -j4 && make install

  export LD_LIBRARY_PATH=/usr/local/lib

  # Try to install Java
  pushd java
  use_java jdk7
  $MVN install
  popd

  # Try to install Python
  virtualenv --no-site-packages venv
  source venv/bin/activate
  pushd python
  python setup.py clean build sdist
  pip install dist/protobuf-*.tar.gz
  popd
  deactivate
  rm -rf python/venv
}

build_csharp() {
  # Required for conformance tests and to regenerate protos.
  internal_build_cpp
  NUGET=/usr/local/bin/nuget.exe

  # Disable some unwanted dotnet options
  export DOTNET_SKIP_FIRST_TIME_EXPERIENCE=true
  export DOTNET_CLI_TELEMETRY_OPTOUT=true

  # TODO(jtattermusch): is this still needed with "first time experience"
  # disabled?
  # Perform "dotnet new" once to get the setup preprocessing out of the
  # way. That spews a lot of output (including backspaces) into logs
  # otherwise, and can cause problems. It doesn't matter if this step
  # is performed multiple times; it's cheap after the first time anyway.
  # (It also doesn't matter if it's unnecessary, which it will be on some
  # systems. It's necessary on Jenkins in order to avoid unprintable
  # characters appearing in the JUnit output.)
  mkdir dotnettmp
  (cd dotnettmp; dotnet new > /dev/null)
  rm -rf dotnettmp

  # Check that the protos haven't broken C# codegen.
  # TODO(jonskeet): Fail if regenerating creates any changes.
  csharp/generate_protos.sh

  csharp/buildall.sh
  cd conformance && make test_csharp && cd ..

  # Run csharp compatibility test between 3.0.0 and the current version.
  csharp/compatibility_tests/v3.0.0/test.sh 3.0.0

  # Run csharp compatibility test between last released and the current version.
  csharp/compatibility_tests/v3.0.0/test.sh $LAST_RELEASED
}

build_golang() {
  # Go build needs `protoc`.
  internal_build_cpp
  # Add protoc to the path so that the examples build finds it.
  export PATH="`pwd`/src:$PATH"

  export GOPATH="$HOME/gocode"
  mkdir -p "$GOPATH/src/github.com/protocolbuffers"
  mkdir -p "$GOPATH/src/github.com/golang"
  rm -f "$GOPATH/src/github.com/protocolbuffers/protobuf"
  rm -f "$GOPATH/src/github.com/golang/protobuf"
  ln -s "`pwd`" "$GOPATH/src/github.com/protocolbuffers/protobuf"
  export PATH="$GOPATH/bin:$PATH"
  (cd $GOPATH/src/github.com/golang && git clone https://github.com/golang/protobuf.git && cd protobuf && git checkout v1.3.5)
  go install github.com/golang/protobuf/protoc-gen-go

  cd examples && PROTO_PATH="-I../src -I." make gotest && cd ..
}

use_java() {
  version=$1
  case "$version" in
    jdk8)
      export PATH=/usr/lib/jvm/java-8-openjdk-amd64/bin:$PATH
      export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
      ;;
    jdk7)
      export PATH=/usr/lib/jvm/java-7-openjdk-amd64/bin:$PATH
      export JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64
      ;;
    oracle7)
      export PATH=/usr/lib/jvm/java-7-oracle/bin:$PATH
      export JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64
      ;;
  esac

  MAVEN_LOCAL_REPOSITORY=/var/maven_local_repository
  MVN="$MVN -e -X -Dhttps.protocols=TLSv1.2 -Dmaven.repo.local=$MAVEN_LOCAL_REPOSITORY"

  which java
  java -version
  $MVN -version
}

# --batch-mode suppresses download progress output that spams the logs.
MVN="mvn --batch-mode"

build_java() {
  version=$1
  dir=java_$version
  # Java build needs `protoc`.
  internal_build_cpp
  cp -r java $dir
  cd $dir && $MVN clean && $MVN test
  cd ../..
}

# The conformance tests are hard-coded to work with the $ROOT/java directory.
# So this can't run in parallel with two different sets of tests.
build_java_with_conformance_tests() {
  # Java build needs `protoc`.
  internal_build_cpp
  # This local installation avoids the problem caused by a new version not yet in Maven Central
  cd java/bom && $MVN install
  cd ../..
  cd java && $MVN test && $MVN install
  cd util && $MVN package assembly:single
  cd ../..
  cd conformance && make test_java && cd ..
}

build_java_jdk7() {
  use_java jdk7
  build_java_with_conformance_tests
}
build_java_oracle7() {
  use_java oracle7
  build_java oracle7
}
build_java_compatibility() {
  use_java jdk7
  internal_build_cpp
  # Use the unit-tests extracted from 2.5.0 to test the compatibility between
  # 3.0.0-beta-4 and the current version.
  cd java/compatibility_tests/v2.5.0
  ./test.sh 3.0.0-beta-4

  # Test the last released and current version.
  ./test.sh $LAST_RELEASED
}
build_java_linkage_monitor() {
  # Linkage Monitor checks compatibility with other Google libraries
  # https://github.com/GoogleCloudPlatform/cloud-opensource-java/tree/master/linkage-monitor

  use_java jdk8
  internal_build_cpp

  # Linkage Monitor uses $HOME/.m2 local repository
  MVN="mvn -e -B -Dhttps.protocols=TLSv1.2"
  cd java
  # Installs the snapshot version locally
  $MVN install -Dmaven.test.skip=true

  # Linkage Monitor uses the snapshot versions installed in $HOME/.m2 to verify compatibility
  JAR=linkage-monitor-latest-all-deps.jar
  curl -v -O "https://storage.googleapis.com/cloud-opensource-java-linkage-monitor/${JAR}"
  # Fails if there's new linkage errors compared with baseline
  java -jar $JAR com.google.cloud:libraries-bom
}

build_objectivec_ios() {
  # Reused the build script that takes care of configuring and ensuring things
  # are up to date.  The OS X test runs the objc conformance test, so skip it
  # here.
  objectivec/DevTools/full_mac_build.sh \
      --core-only --skip-xcode-osx --skip-xcode-tvos --skip-objc-conformance "$@"
}

build_objectivec_ios_debug() {
  build_objectivec_ios --skip-xcode-release
}

build_objectivec_ios_release() {
  build_objectivec_ios --skip-xcode-debug
}

build_objectivec_osx() {
  # Reused the build script that takes care of configuring and ensuring things
  # are up to date.
  objectivec/DevTools/full_mac_build.sh \
      --core-only --skip-xcode-ios --skip-xcode-tvos
}

build_objectivec_tvos() {
  # Reused the build script that takes care of configuring and ensuring things
  # are up to date.  The OS X test runs the objc conformance test, so skip it
  # here.
  objectivec/DevTools/full_mac_build.sh \
      --core-only --skip-xcode-ios --skip-xcode-osx --skip-objc-conformance "$@"
}

build_objectivec_tvos_debug() {
  build_objectivec_tvos --skip-xcode-release
}

build_objectivec_tvos_release() {
  build_objectivec_tvos --skip-xcode-debug
}

build_objectivec_cocoapods_integration() {
  objectivec/Tests/CocoaPods/run_tests.sh
}

build_python() {
  internal_build_cpp
  cd python
  if [ $(uname -s) == "Linux" ]; then
    envlist=py\{27,33,34,35,36\}-python
  else
    envlist=py\{27,36\}-python
  fi
  tox -e $envlist
  cd ..
}

build_python_version() {
  internal_build_cpp
  cd python
  envlist=$1
  tox -e $envlist
  cd ..
}

build_python27() {
  build_python_version py27-python
}

build_python33() {
  build_python_version py33-python
}

build_python34() {
  build_python_version py34-python
}

build_python35() {
  build_python_version py35-python
}

build_python36() {
  build_python_version py36-python
}

build_python37() {
  build_python_version py37-python
}

build_python38() {
  build_python_version py38-python
}

build_python_cpp() {
  internal_build_cpp
  export LD_LIBRARY_PATH=../src/.libs # for Linux
  export DYLD_LIBRARY_PATH=../src/.libs # for OS X
  cd python
  if [ $(uname -s) == "Linux" ]; then
    envlist=py\{27,33,34,35,36\}-cpp
  else
    envlist=py\{27,36\}-cpp
  fi
  tox -e $envlist
  cd ..
}

build_python_cpp_version() {
  internal_build_cpp
  export LD_LIBRARY_PATH=../src/.libs # for Linux
  export DYLD_LIBRARY_PATH=../src/.libs # for OS X
  cd python
  envlist=$1
  tox -e $envlist
  cd ..
}

build_python27_cpp() {
  build_python_cpp_version py27-cpp
}

build_python33_cpp() {
  build_python_cpp_version py33-cpp
}

build_python34_cpp() {
  build_python_cpp_version py34-cpp
}

build_python35_cpp() {
  build_python_cpp_version py35-cpp
}

build_python36_cpp() {
  build_python_cpp_version py36-cpp
}

build_python37_cpp() {
  build_python_cpp_version py37-cpp
}

build_python38_cpp() {
  build_python_cpp_version py38-cpp
}

build_python_compatibility() {
  internal_build_cpp
  # Use the unit-tests extracted from 2.5.0 to test the compatibility.
  cd python/compatibility_tests/v2.5.0
  # Test between 2.5.0 and the current version.
  ./test.sh 2.5.0
  # Test between 3.0.0-beta-1 and the current version.
  ./test.sh 3.0.0-beta-1

  # Test between last released and current version.
  ./test.sh $LAST_RELEASED
}

build_ruby23() {
  internal_build_cpp  # For conformance tests.
  cd ruby && bash travis-test.sh ruby-2.3.8 && cd ..
}
build_ruby24() {
  internal_build_cpp  # For conformance tests.
  cd ruby && bash travis-test.sh ruby-2.4 && cd ..
}
build_ruby25() {
  internal_build_cpp  # For conformance tests.
  cd ruby && bash travis-test.sh ruby-2.5.1 && cd ..
}
build_ruby26() {
  internal_build_cpp  # For conformance tests.
  cd ruby && bash travis-test.sh ruby-2.6.0 && cd ..
}
build_ruby27() {
  internal_build_cpp  # For conformance tests.
  cd ruby && bash travis-test.sh ruby-2.7.0 && cd ..
}

build_javascript() {
  internal_build_cpp
  NODE_VERSION=node-v12.16.3-darwin-x64
  NODE_TGZ="$NODE_VERSION.tar.gz"
  pushd /tmp
  curl -OL https://nodejs.org/dist/v12.16.3/$NODE_TGZ
  tar zxvf $NODE_TGZ
  export PATH=$PATH:`pwd`/$NODE_VERSION/bin
  popd
  cd js && npm install && npm test && cd ..
  cd conformance && make test_nodejs && cd ..
}

use_php() {
  VERSION=$1
  export PATH=/usr/local/php-${VERSION}/bin:$PATH
  internal_build_cpp
}

use_php_zts() {
  VERSION=$1
  export PATH=/usr/local/php-${VERSION}-zts/bin:$PATH
  internal_build_cpp
}

build_php5.5() {
  use_php 5.5

  pushd php
  rm -rf vendor
  composer update
  composer test
  popd
  (cd conformance && make test_php)
}

build_php5.6() {
  use_php 5.6
  pushd php
  rm -rf vendor
  composer update
  composer test
  popd
  (cd conformance && make test_php)
}

build_php5.6_mac() {
  # Install PHP
  curl -s https://php-osx.liip.ch/install.sh | bash -s 5.6
  PHP_FOLDER=`find /usr/local -type d -name "php5-5.6*"`  # The folder name may change upon time
  test ! -z "$PHP_FOLDER"
  export PATH="$PHP_FOLDER/bin:$PATH"

  internal_build_cpp

  # Run pure-PHP tests only.
  pushd php
  rm -rf vendor
  composer update
  composer test
  popd
  (cd conformance && make test_php)
}

build_php7.0() {
  use_php 7.0
  pushd php
  rm -rf vendor
  composer update
  composer test
  popd
  (cd conformance && make test_php)
}

build_php7.0_c() {
  IS_64BIT=$1
  use_php 7.0
  php/tests/test.sh
  pushd conformance
  if [ "$IS_64BIT" = "true" ]
  then
    make test_php_c
  else
    make test_php_c_32
  fi
  popd
}

build_php7.0_mixed() {
  use_php 7.0
  pushd php
  rm -rf vendor
  composer update
  tests/compile_extension.sh
  tests/generate_protos.sh
  php -dextension=./ext/google/protobuf/modules/protobuf.so ./vendor/bin/phpunit
  popd
}

build_php7.0_zts_c() {
  IS_64BIT=$1
  use_php_zts 7.0
  php/tests/test.sh
  pushd conformance
  if [ "$IS_64BIT" = "true" ]
  then
    make test_php_c
  else
    make test_php_c_32
  fi
  popd
}

build_php7.0_mac() {
  internal_build_cpp
  # Install PHP
  curl -s https://php-osx.liip.ch/install.sh | bash -s 7.0
  PHP_FOLDER=`find /usr/local -type d -name "php5-7.0*"`  # The folder name may change upon time
  test ! -z "$PHP_FOLDER"
  export PATH="$PHP_FOLDER/bin:$PATH"

  # Install valgrind
  echo "#! /bin/bash" > valgrind
  chmod ug+x valgrind
  sudo mv valgrind /usr/local/bin/valgrind

  # Test
  php/tests/test.sh
  (cd conformance && make test_php_c)
}

build_php7.3_mac() {
  internal_build_cpp
  # Install PHP
  # We can't test PHP 7.4 with these binaries yet:
  #   https://github.com/liip/php-osx/issues/276
  curl -s https://php-osx.liip.ch/install.sh | bash -s 7.3
  PHP_FOLDER=`find /usr/local -type d -name "php5-7.3*"`  # The folder name may change upon time
  test ! -z "$PHP_FOLDER"
  export PATH="$PHP_FOLDER/bin:$PATH"

  # Install valgrind
  echo "#! /bin/bash" > valgrind
  chmod ug+x valgrind
  sudo mv valgrind /usr/local/bin/valgrind

  # Test
  php/tests/test.sh
  (cd conformance && make test_php_c)
}

build_php_compatibility() {
  internal_build_cpp
  php/tests/compatibility_test.sh $LAST_RELEASED
}

build_php_multirequest() {
  use_php 7.4
  php/tests/multirequest.sh
}

build_php7.1() {
  use_php 7.1
  pushd php
  rm -rf vendor
  composer update
  composer test
  popd
  (cd conformance && make test_php)
}

build_php7.1_c() {
  IS_64BIT=$1
  use_php 7.1
  php/tests/test.sh
  pushd conformance
  if [ "$IS_64BIT" = "true" ]
  then
    make test_php_c
  else
    make test_php_c_32
  fi
  popd
}

build_php7.1_mixed() {
  use_php 7.1
  pushd php
  rm -rf vendor
  composer update
  tests/compile_extension.sh
  tests/generate_protos.sh
  php -dextension=./ext/google/protobuf/modules/protobuf.so ./vendor/bin/phpunit
  popd
}

build_php7.1_zts_c() {
  IS_64BIT=$1
  use_php_zts 7.1
  php/tests/test.sh
  pushd conformance
  if [ "$IS_64BIT" = "true" ]
  then
    make test_php_c
  else
    make test_php_c_32
  fi
  popd
}

build_php7.4() {
  use_php 7.4
  pushd php
  rm -rf vendor
  composer update
  composer test
  popd
  (cd conformance && make test_php)
}

build_php7.4_c() {
  IS_64BIT=$1
  use_php 7.4
  php/tests/test.sh
  pushd conformance
  if [ "$IS_64BIT" = "true" ]
  then
    make test_php_c
  else
    make test_php_c_32
  fi
  popd
}

build_php7.4_mixed() {
  use_php 7.4
  pushd php
  rm -rf vendor
  composer update
  tests/compile_extension.sh
  tests/generate_protos.sh
  php -dextension=./ext/google/protobuf/modules/protobuf.so ./vendor/bin/phpunit
  popd
}

build_php7.4_zts_c() {
  IS_64BIT=$1
  use_php_zts 7.4
  php/tests/test.sh
  pushd conformance
  if [ "$IS_64BIT" = "true" ]
  then
    make test_php_c
  else
    make test_php_c_32
  fi
  popd
}

build_php8.0() {
  use_php 8.0
  pushd php
  rm -rf vendor
  composer update
  composer test
  popd
  (cd conformance && make test_php)
}

build_php8.0_c() {
  IS_64BIT=$1
  use_php 8.0
  php/tests/test.sh
  pushd conformance
  if [ "$IS_64BIT" = "true" ]
  then
    make test_php_c
  else
    make test_php_c_32
  fi
  popd
}

build_php8.0_c_64() {
  build_php8.0_c true
}

build_php8.0_mixed() {
  use_php 8.0
  pushd php
  rm -rf vendor
  composer update
  tests/compile_extension.sh
  tests/generate_protos.sh
  php -dextension=./ext/google/protobuf/modules/protobuf.so ./vendor/bin/phpunit
  popd
}

build_php8.0_all() {
  build_php8.0
  build_php8.0_c_64
  build_php8.0_mixed
}

build_php_all_32() {
  build_php5.5
  build_php5.6
  build_php7.0
  build_php7.1
  build_php7.4
  build_php7.0_c $1
  build_php7.1_c $1
  build_php7.4_c $1
  build_php7.0_mixed
  build_php7.1_mixed
  build_php7.4_mixed
  build_php7.0_zts_c $1
  build_php7.1_zts_c $1
  build_php7.4_zts_c $1
}

build_php_all() {
  build_php_all_32 true
  build_php_multirequest
  build_php_compatibility
}

build_benchmark() {
  use_php 7.2
  cd kokoro/linux/benchmark && ./run.sh
}

# -------- main --------

if [ "$#" -ne 1 ]; then
  echo "
Usage: $0 { cpp |
            cpp_distcheck |
            csharp |
            java_jdk7 |
            java_oracle7 |
            java_compatibility |
            java_linkage_monitor |
            objectivec_ios |
            objectivec_ios_debug |
            objectivec_ios_release |
            objectivec_osx |
            objectivec_tvos |
            objectivec_tvos_debug |
            objectivec_tvos_release |
            objectivec_cocoapods_integration |
            python |
            python_cpp |
            python_compatibility |
            ruby23 |
            ruby24 |
            ruby25 |
            ruby26 |
            ruby27 |
            jruby |
            ruby_all |
            php5.5   |
            php5.6   |
            php7.0   |
            php7.0_c |
            php_compatibility |
            php7.1   |
            php7.1_c |
            php_all |
            dist_install |
            benchmark)
"
  exit 1
fi

set -e  # exit immediately on error
set -x  # display all commands
cd $(dirname $0)
eval "build_$1"
