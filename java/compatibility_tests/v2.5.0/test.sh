#!/bin/bash

set -ex

# Change to the script's directory.
cd $(dirname $0)

MAVEN_LOCAL_REPOSITORY=/var/maven_local_repository
MVN="mvn --batch-mode -e -X -Dhttps.protocols=TLSv1.2 -Dmaven.repo.local=$MAVEN_LOCAL_REPOSITORY"

# Version of the tests (i.e., the version of protobuf from where we extracted
# these tests).
TEST_VERSION=`grep "^  <version>.*</version>" pom.xml | sed "s|  <version>\(.*\)</version>|\1|"`

# The old version of protobuf that we are testing compatibility against. This
# is usually the same as TEST_VERSION (i.e., we use the tests extracted from
# that version to test compatibility of the newest runtime against it), but it
# is also possible to use this same test set to test the compatibiilty of the
# latest version against other versions.
case "$1" in
  ""|2.5.0)
    OLD_VERSION=2.5.0
    OLD_VERSION_PROTOC=https://github.com/xfxyjwf/protobuf-compiler-release/raw/master/v2.5.0/linux/protoc
    ;;
  2.6.1)
    OLD_VERSION=2.6.1
    OLD_VERSION_PROTOC=https://repo1.maven.org/maven2/com/google/protobuf/protoc/2.6.1-build2/protoc-2.6.1-build2-linux-x86_64.exe
    ;;
  3.0.0-beta-1)
    OLD_VERSION=3.0.0-beta-1
    OLD_VERSION_PROTOC=https://repo1.maven.org/maven2/com/google/protobuf/protoc/3.0.0-beta-1/protoc-3.0.0-beta-1-linux-x86_64.exe
    ;;
  3.0.0-beta-2)
    OLD_VERSION=3.0.0-beta-2
    OLD_VERSION_PROTOC=https://repo1.maven.org/maven2/com/google/protobuf/protoc/3.0.0-beta-2/protoc-3.0.0-beta-2-linux-x86_64.exe
    ;;
  3.0.0-beta-3)
    OLD_VERSION=3.0.0-beta-3
    OLD_VERSION_PROTOC=https://repo1.maven.org/maven2/com/google/protobuf/protoc/3.0.0-beta-3/protoc-3.0.0-beta-3-linux-x86_64.exe
    ;;
  3.0.0-beta-4)
    OLD_VERSION=3.0.0-beta-4
    OLD_VERSION_PROTOC=https://repo1.maven.org/maven2/com/google/protobuf/protoc/3.0.0-beta-4/protoc-3.0.0-beta-4-linux-x86_64.exe
    ;;
  *)
    echo "[ERROR]: Unknown version number: $1"
    exit 1
    ;;
esac

# Extract the latest protobuf version number.
VERSION_NUMBER=`grep "^  <version>.*</version>" ../../pom.xml | sed "s|  <version>\(.*\)</version>|\1|"`

echo "Running compatibility tests between $VERSION_NUMBER and $OLD_VERSION"

# Check protoc
[ -f ../../../src/protoc ] || {
  echo "[ERROR]: Please build protoc first."
  exit 1
}

# Build and install protobuf-java-$VERSION_NUMBER.jar
[ -f ../../core/target/protobuf-java-$VERSION_NUMBER.jar ] || {
  pushd ../..
  $MVN install -Dmaven.test.skip=true
  popd
}

# Download old version source for the compatibility test
[ -d protobuf ] || {
  git clone https://github.com/protocolbuffers/protobuf.git
  cd protobuf
  git reset --hard v$TEST_VERSION
  cd ..
}

# Download old version protoc compiler (for linux)
wget $OLD_VERSION_PROTOC -O protoc
chmod +x protoc

# Test source compatibility. In these tests we recompile everything against
# the new runtime (including old version generated code).

# Test A.1:
#   protos: use new version
#   more_protos: use old version
$MVN clean test \
  -Dprotobuf.test.source.path=$(pwd)/protobuf \
  -Dprotoc.path=$(pwd)/protoc \
  -Dprotos.protoc.path=$(pwd)/../../../src/protoc \
  -Dprotobuf.version=$VERSION_NUMBER

# Test A.2:
#   protos: use old version
#   more_protos: use new version
$MVN clean test \
  -Dprotobuf.test.source.path=$(pwd)/protobuf \
  -Dprotoc.path=$(pwd)/protoc \
  -Dmore_protos.protoc.path=$(pwd)/../../../src/protoc \
  -Dprotobuf.version=$VERSION_NUMBER

# Test binary compatibility. In these tests we run the old version compiled
# jar against the new runtime directly without recompile.

# Collect all test dependencies in a single jar file (except for protobuf) to
# make it easier to run binary compatibility test (where we will need to run
# the jar files directly).
cd deps
$MVN assembly:single
cd ..
cp -f deps/target/compatibility-test-deps-${TEST_VERSION}-jar-with-dependencies.jar deps.jar

# Build the old version of all 3 artifacts.
$MVN clean install -Dmaven.test.skip=true -Dprotoc.path=$(pwd)/protoc -Dprotobuf.version=$OLD_VERSION
cp -f protos/target/compatibility-protos-${TEST_VERSION}.jar protos.jar
cp -f more_protos/target/compatibility-more-protos-${TEST_VERSION}.jar more_protos.jar
cp -f tests/target/compatibility-tests-${TEST_VERSION}.jar tests.jar

# Collect the list of tests we need to run.
TESTS=`find tests -name "*Test.java" | sed "s|/|.|g;s/.java$//g;s/tests.src.main.java.//g"`

# Test B.1: run all the old artifacts against the new runtime. Note that we
# must run the test in the protobuf source tree because some of the tests need
# to read golden test data files.
cd protobuf
java -cp ../../../core/target/protobuf-java-$VERSION_NUMBER.jar:../protos.jar:../more_protos.jar:../tests.jar:../deps.jar org.junit.runner.JUnitCore $TESTS
cd ..

# Test B.2: update protos.jar only.
cd protos
$MVN clean package -Dmaven.test.skip=true -Dprotoc.path=$(pwd)/../../../../src/protoc -Dprotobuf.version=$VERSION_NUMBER
cd ..
cd protobuf
java -cp ../../../core/target/protobuf-java-$VERSION_NUMBER.jar:../protos/target/compatibility-protos-${TEST_VERSION}.jar:../more_protos.jar:../tests.jar:../deps.jar org.junit.runner.JUnitCore $TESTS
cd ..

# Test B.3: update more_protos.jar only.
cd more_protos
$MVN clean package -Dmaven.test.skip=true -Dprotoc.path=$(pwd)/../../../../src/protoc -Dprotobuf.version=$VERSION_NUMBER
cd ..
cd protobuf
java -cp ../../../core/target/protobuf-java-$VERSION_NUMBER.jar:../protos.jar:../more_protos/target/compatibility-more-protos-${TEST_VERSION}.jar:../tests.jar:../deps.jar org.junit.runner.JUnitCore $TESTS
cd ..
