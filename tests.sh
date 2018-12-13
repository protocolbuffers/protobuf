#!/bin/bash
#
# Build and runs tests for the protobuf project. We use this script to run
# tests on kokoro (Ubuntu and MacOS). It can run locally as well but you
# will need to make sure the required compilers/tools are available.

# For when some other test needs the C++ main build, including protoc and
# libprotobuf.
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
  make -j4
}

build_cpp() {
  internal_build_cpp
  make check -j4 || (cat src/test-suite.log; false)
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

build_cpp_distcheck() {
  # Initialize any submodules.
  git submodule update --init --recursive
  ./autogen.sh
  ./configure
  make dist

  # List all files that should be included in the distribution package.
  git ls-files | grep "^\(java\|python\|objectivec\|csharp\|js\|ruby\|php\|cmake\|examples\|src/google/protobuf/.*\.proto\)" |\
    grep -v ".gitignore" | grep -v "java/compatibility_tests" |\
    grep -v "python/compatibility_tests" | grep -v "csharp/compatibility_tests" > dist.lst
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
  make distcheck -j4
}

build_csharp() {
  # Required for conformance tests and to regenerate protos.
  internal_build_cpp
  NUGET=/usr/local/bin/nuget.exe

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
}

build_golang() {
  # Go build needs `protoc`.
  internal_build_cpp
  # Add protoc to the path so that the examples build finds it.
  export PATH="`pwd`/src:$PATH"

  export GOPATH="$HOME/gocode"
  mkdir -p "$GOPATH/src/github.com/protocolbuffers"
  rm -f "$GOPATH/src/github.com/protocolbuffers/protobuf"
  ln -s "`pwd`" "$GOPATH/src/github.com/protocolbuffers/protobuf"
  export PATH="$GOPATH/bin:$PATH"
  go get github.com/golang/protobuf/protoc-gen-go

  cd examples && PROTO_PATH="-I../src -I." make gotest && cd ..
}

use_java() {
  version=$1
  case "$version" in
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

# --batch-mode supresses download progress output that spams the logs.
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
  # Use the unit-tests extraced from 2.5.0 to test the compatibilty between
  # 3.0.0-beta-4 and the current version.
  cd java/compatibility_tests/v2.5.0
  ./test.sh 3.0.0-beta-4
}

build_objectivec_ios() {
  # Reused the build script that takes care of configuring and ensuring things
  # are up to date.  The OS X test runs the objc conformance test, so skip it
  # here.
  # Note: travis has xctool installed, and we've looked at using it in the past
  # but it has ended up proving unreliable (bugs), an they are removing build
  # support in favor of xcbuild (or just xcodebuild).
  objectivec/DevTools/full_mac_build.sh \
      --core-only --skip-xcode-osx --skip-objc-conformance "$@"
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
      --core-only --skip-xcode-ios
}

build_objectivec_cocoapods_integration() {
  # Update pod to the latest version.
  gem install cocoapods --no-ri --no-rdoc
  objectivec/Tests/CocoaPods/run_tests.sh
}

build_python() {
  internal_build_cpp
  cd python
  if [ $(uname -s) == "Linux" ]; then
    envlist=py\{27,33,34,35,36\}-python
  else
    envlist=py27-python
  fi
  tox -e $envlist
  cd ..
}

build_python_cpp() {
  internal_build_cpp
  export LD_LIBRARY_PATH=../src/.libs # for Linux
  export DYLD_LIBRARY_PATH=../src/.libs # for OS X
  cd python
  if [ $(uname -s) == "Linux" ]; then
    envlist=py\{27,33,34,35,36\}-cpp
  else
    envlist=py27-cpp
  fi
  tox -e $envlist
  cd ..
}

build_python_compatibility() {
  internal_build_cpp
  # Use the unit-tests extraced from 2.5.0 to test the compatibilty.
  cd python/compatibility_tests/v2.5.0
  # Test between 2.5.0 and the current version.
  ./test.sh 2.5.0
  # Test between 3.0.0-beta-1 and the current version.
  ./test.sh 3.0.0-beta-1
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
build_ruby_all() {
  build_ruby23
  build_ruby24
  build_ruby25
}

build_javascript() {
  internal_build_cpp
  cd js && npm install && npm test && cd ..
  cd conformance && make test_nodejs && cd ..
}

generate_php_test_proto() {
  internal_build_cpp
  pushd php/tests
  # Generate test file
  rm -rf generated
  mkdir generated
  ../../src/protoc --php_out=generated         \
    -I../../src -I.                            \
    proto/empty/echo.proto                     \
    proto/test.proto                           \
    proto/test_include.proto                   \
    proto/test_no_namespace.proto              \
    proto/test_prefix.proto                    \
    proto/test_php_namespace.proto             \
    proto/test_empty_php_namespace.proto       \
    proto/test_reserved_enum_lower.proto       \
    proto/test_reserved_enum_upper.proto       \
    proto/test_reserved_enum_value_lower.proto \
    proto/test_reserved_enum_value_upper.proto \
    proto/test_reserved_message_lower.proto    \
    proto/test_reserved_message_upper.proto    \
    proto/test_service.proto                   \
    proto/test_service_namespace.proto         \
    proto/test_wrapper_type_setters.proto      \
    proto/test_descriptors.proto
  pushd ../../src
  ./protoc --php_out=../php/tests/generated -I../php/tests -I. \
    ../php/tests/proto/test_import_descriptor_proto.proto
  popd
  popd
}

use_php() {
  VERSION=$1
  PHP=`which php`
  PHP_CONFIG=`which php-config`
  PHPIZE=`which phpize`
  ln -sfn "/usr/local/php-${VERSION}/bin/php" $PHP
  ln -sfn "/usr/local/php-${VERSION}/bin/php-config" $PHP_CONFIG
  ln -sfn "/usr/local/php-${VERSION}/bin/phpize" $PHPIZE
  generate_php_test_proto
}

use_php_zts() {
  VERSION=$1
  PHP=`which php`
  PHP_CONFIG=`which php-config`
  PHPIZE=`which phpize`
  ln -sfn "/usr/local/php-${VERSION}-zts/bin/php" $PHP
  ln -sfn "/usr/local/php-${VERSION}-zts/bin/php-config" $PHP_CONFIG
  ln -sfn "/usr/local/php-${VERSION}-zts/bin/phpize" $PHPIZE
  generate_php_test_proto
}

use_php_bc() {
  VERSION=$1
  PHP=`which php`
  PHP_CONFIG=`which php-config`
  PHPIZE=`which phpize`
  ln -sfn "/usr/local/php-${VERSION}-bc/bin/php" $PHP
  ln -sfn "/usr/local/php-${VERSION}-bc/bin/php-config" $PHP_CONFIG
  ln -sfn "/usr/local/php-${VERSION}-bc/bin/phpize" $PHPIZE
  generate_php_test_proto
}

build_php5.5() {
  use_php 5.5

  pushd php
  rm -rf vendor
  cp -r /usr/local/vendor-5.5 vendor
  wget https://phar.phpunit.de/phpunit-4.8.0.phar -O /usr/bin/phpunit
  phpunit
  popd
  pushd conformance
  make test_php
  popd
}

build_php5.5_c() {
  use_php 5.5
  wget https://phar.phpunit.de/phpunit-4.8.0.phar -O /usr/bin/phpunit
  pushd php/tests
  /bin/bash ./test.sh 5.5
  popd
  # TODO(teboring): Add it back
  # pushd conformance
  # make test_php_c
  # popd
}

build_php5.5_zts_c() {
  use_php_zts 5.5
  wget https://phar.phpunit.de/phpunit-4.8.0.phar -O /usr/bin/phpunit
  cd php/tests && /bin/bash ./test.sh 5.5-zts && cd ../..
  # TODO(teboring): Add it back
  # pushd conformance
  # make test_php_zts_c
  # popd
}

build_php5.6() {
  use_php 5.6
  pushd php
  rm -rf vendor
  cp -r /usr/local/vendor-5.6 vendor
  wget https://phar.phpunit.de/phpunit-5.7.0.phar -O /usr/bin/phpunit
  phpunit
  popd
  pushd conformance
  make test_php
  popd
}

build_php5.6_c() {
  use_php 5.6
  wget https://phar.phpunit.de/phpunit-5.7.0.phar -O /usr/bin/phpunit
  cd php/tests && /bin/bash ./test.sh 5.6 && cd ../..
  # TODO(teboring): Add it back
  # pushd conformance
  # make test_php_c
  # popd
}

build_php5.6_zts_c() {
  use_php_zts 5.6
  wget https://phar.phpunit.de/phpunit-5.7.0.phar -O /usr/bin/phpunit
  cd php/tests && /bin/bash ./test.sh 5.6-zts && cd ../..
  # TODO(teboring): Add it back
  # pushd conformance
  # make test_php_zts_c
  # popd
}

build_php5.6_mac() {
  generate_php_test_proto
  # Install PHP
  curl -s https://php-osx.liip.ch/install.sh | bash -s 5.6
  PHP_FOLDER=`find /usr/local -type d -name "php5-5.6*"`  # The folder name may change upon time
  export PATH="$PHP_FOLDER/bin:$PATH"

  # Install phpunit
  curl https://phar.phpunit.de/phpunit-5.6.10.phar -L -o phpunit.phar
  chmod +x phpunit.phar
  sudo mv phpunit.phar /usr/local/bin/phpunit

  # Install valgrind
  echo "#! /bin/bash" > valgrind
  chmod ug+x valgrind
  sudo mv valgrind /usr/local/bin/valgrind

  # Test
  cd php/tests && /bin/bash ./test.sh && cd ../..
  # TODO(teboring): Add it back
  # pushd conformance
  # make test_php_c
  # popd
}

build_php7.0() {
  use_php 7.0
  pushd php
  rm -rf vendor
  cp -r /usr/local/vendor-7.0 vendor
  wget https://phar.phpunit.de/phpunit-5.6.0.phar -O /usr/bin/phpunit
  phpunit
  popd
  pushd conformance
  make test_php
  popd
}

build_php7.0_c() {
  use_php 7.0
  wget https://phar.phpunit.de/phpunit-5.6.0.phar -O /usr/bin/phpunit
  cd php/tests && /bin/bash ./test.sh 7.0 && cd ../..
  # TODO(teboring): Add it back
  # pushd conformance
  # make test_php_c
  # popd
}

build_php7.0_zts_c() {
  use_php_zts 7.0
  wget https://phar.phpunit.de/phpunit-5.6.0.phar -O /usr/bin/phpunit
  cd php/tests && /bin/bash ./test.sh 7.0-zts && cd ../..
  # TODO(teboring): Add it back.
  # pushd conformance
  # make test_php_zts_c
  # popd
}

build_php7.0_mac() {
  generate_php_test_proto
  # Install PHP
  curl -s https://php-osx.liip.ch/install.sh | bash -s 7.0
  PHP_FOLDER=`find /usr/local -type d -name "php7-7.0*"`  # The folder name may change upon time
  export PATH="$PHP_FOLDER/bin:$PATH"

  # Install phpunit
  curl https://phar.phpunit.de/phpunit-5.6.0.phar -L -o phpunit.phar
  chmod +x phpunit.phar
  sudo mv phpunit.phar /usr/local/bin/phpunit

  # Install valgrind
  echo "#! /bin/bash" > valgrind
  chmod ug+x valgrind
  sudo mv valgrind /usr/local/bin/valgrind

  # Test
  cd php/tests && /bin/bash ./test.sh && cd ../..
  # TODO(teboring): Add it back
  # pushd conformance
  # make test_php_c
  # popd
}

build_php_compatibility() {
  internal_build_cpp
  php/tests/compatibility_test.sh
}

build_php7.1() {
  use_php 7.1
  pushd php
  rm -rf vendor
  cp -r /usr/local/vendor-7.1 vendor
  wget https://phar.phpunit.de/phpunit-5.6.0.phar -O /usr/bin/phpunit
  phpunit
  popd
  pushd conformance
  make test_php
  popd
}

build_php7.1_c() {
  ENABLE_CONFORMANCE_TEST=$1
  use_php 7.1
  wget https://phar.phpunit.de/phpunit-5.6.0.phar -O /usr/bin/phpunit
  cd php/tests && /bin/bash ./test.sh 7.1 && cd ../..
  if [ "$ENABLE_CONFORMANCE_TEST" = "true" ]
  then
    pushd conformance
    make test_php_c
    popd
  fi
}

build_php7.1_zts_c() {
  use_php_zts 7.1
  wget https://phar.phpunit.de/phpunit-5.6.0.phar -O /usr/bin/phpunit
  cd php/tests && /bin/bash ./test.sh 7.1-zts && cd ../..
  pushd conformance
  # make test_php_c
  popd
}

build_php_all_32() {
  build_php5.5
  build_php5.6
  build_php7.0
  build_php7.1
  build_php5.5_c
  build_php5.6_c
  build_php7.0_c
  build_php7.1_c $1
  build_php5.5_zts_c
  build_php5.6_zts_c
  build_php7.0_zts_c
  build_php7.1_zts_c
}

build_php_all() {
  build_php_all_32 true
  build_php_compatibility
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
            objectivec_ios |
            objectivec_ios_debug |
            objectivec_ios_release |
            objectivec_osx |
            objectivec_cocoapods_integration |
            python |
            python_cpp |
            python_compatibility |
            ruby23 |
            ruby24 |
            ruby25 |
            jruby |
            ruby_all |
            php5.5   |
            php5.5_c |
            php5.6   |
            php5.6_c |
            php7.0   |
            php7.0_c |
            php_compatibility |
            php7.1   |
            php7.1_c |
            php_all)
"
  exit 1
fi

set -e  # exit immediately on error
set -x  # display all commands
cd $(dirname $0)
eval "build_$1"
