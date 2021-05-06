#!/bin/bash

set -e

# Download protoc 3.0.0 from Maven if it is not already present.
OLD_PROTOC_URL=https://repo1.maven.org/maven2/com/google/protobuf/protoc/3.0.0/protoc-3.0.0-linux-x86_64.exe
if [ ! -f protoc ]; then
  wget $OLD_PROTOC_URL -O protoc
  chmod +x protoc
fi

pushd ../..
npm install && npm test
popd

old_protoc=./protoc
new_protoc=../../../src/protoc

# The protos in group 2 have some dependencies on protos in group 1. The tests
# will verify that the generated code for one group can be regenerated
# independently of the other group in a compatible way.
#
# Note: these lists of protos duplicate the lists in gulpfile.js. Ideally we
# should find a good way of having a single source of truth for this.
group1_protos="data.proto test3.proto test5.proto commonjs/test6/test6.proto testbinary.proto testempty.proto test.proto"
group2_protos="proto3_test.proto test2.proto test4.proto commonjs/test7/test7.proto"

# We test the following cases:
#
# Case 1: build groups 1 and 2 with the old protoc
# Case 2: build group 1 with new protoc but group 2 with old protoc
# Case 3: build group 1 with old protoc but group 2 with new protoc
#
# In each case, we use the current runtime.

#
# CommonJS tests
#
mkdir -p commonjs_out{1,2,3}
# Case 1
$old_protoc --js_out=import_style=commonjs,binary:commonjs_out1 -I ../../../src -I commonjs -I . $group1_protos
$old_protoc --js_out=import_style=commonjs,binary:commonjs_out1 -I ../../../src -I commonjs -I . $group2_protos
# Case 2
$new_protoc --js_out=import_style=commonjs,binary:commonjs_out2 -I ../../../src -I commonjs -I . $group1_protos
$old_protoc --js_out=import_style=commonjs,binary:commonjs_out2 -I ../../../src -I commonjs -I . $group2_protos
# Case 3
$old_protoc --js_out=import_style=commonjs,binary:commonjs_out3 -I ../../../src -I commonjs -I . $group1_protos
$new_protoc --js_out=import_style=commonjs,binary:commonjs_out3 -I ../../../src -I commonjs -I . $group2_protos

mkdir -p commonjs_out/binary
for file in *_test.js binary/*_test.js; do
  node commonjs/rewrite_tests_for_commonjs.js < "$file" > "commonjs_out/$file"
done
cp commonjs/{jasmine.json,import_test.js} commonjs_out/
mkdir -p commonjs_out/test_node_modules
../../node_modules/.bin/google-closure-compiler \
  --entry_point=commonjs/export_asserts.js \
  --js=commonjs/export_asserts.js \
  --js=../../node_modules/google-closure-library/closure/goog/**.js \
  --js=../../node_modules/google-closure-library/third_party/closure/goog/**.js \
  > commonjs_out/test_node_modules/closure_asserts_commonjs.js
../../node_modules/.bin/google-closure-compiler \
  --entry_point=commonjs/export_testdeps.js \
  --js=commonjs/export_testdeps.js \
  --js=../../binary/*.js \
  --js=!../../binary/*_test.js \
  --js=../../node_modules/google-closure-library/closure/goog/**.js \
  --js=../../node_modules/google-closure-library/third_party/closure/goog/**.js \
  > commonjs_out/test_node_modules/testdeps_commonjs.js
cp ../../google-protobuf.js commonjs_out/test_node_modules
cp -r ../../commonjs_out/node_modules commonjs_out

echo
echo "Running tests with CommonJS imports"
echo "-----------------------------------"
for i in 1 2 3; do
  cp -r commonjs_out/* "commonjs_out$i"
  pushd "commonjs_out$i"
  JASMINE_CONFIG_PATH=jasmine.json NODE_PATH=test_node_modules ../../../node_modules/.bin/jasmine
  popd
done

#
# Closure tests
#
$old_protoc --js_out=library=testproto_libs1,binary:.  -I ../../../src -I commonjs -I . $group1_protos
$old_protoc --js_out=library=testproto_libs2,binary:.  -I ../../../src -I commonjs -I . $group2_protos
$new_protoc --js_out=library=testproto_libs1_new,binary:.  -I ../../../src -I commonjs -I . $group1_protos
$new_protoc --js_out=library=testproto_libs2_new,binary:.  -I ../../../src -I commonjs -I . $group2_protos

echo
echo "Running tests with Closure-style imports"
echo "----------------------------------------"

# Case 1
JASMINE_CONFIG_PATH=jasmine1.json ../../node_modules/.bin/jasmine
# Case 2
JASMINE_CONFIG_PATH=jasmine2.json ../../node_modules/.bin/jasmine
# Case 3
JASMINE_CONFIG_PATH=jasmine3.json ../../node_modules/.bin/jasmine

# Remove these files so that calcdeps.py does not get confused by them the next
# time this script runs.
rm testproto_libs[12]*
