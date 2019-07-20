#!/bin/bash

# This script verifies that BUILD files and cmake files are in sync with src/Makefile.am

cp "BUILD" "BUILD.orginal"
cp "cmake/extract_includes.bat.in" "cmake/extract_includes.bat.in.orginal"
cp "cmake/libprotobuf-lite.cmake" "cmake/libprotobuf-lite.cmake.orginal"
cp "cmake/libprotobuf.cmake" "cmake/libprotobuf.cmake.orginal"
cp "cmake/libprotoc.cmake" "cmake/libprotoc.cmake.orginal"
cp "cmake/tests.cmake" "cmake/tests.cmake.orginal"

if [ "$(uname)" == "Linux" ]; then
  ./update_file_lists.sh
fi

diff "BUILD.orginal" "BUILD"
diff "cmake/extract_includes.bat.in.orginal" "cmake/extract_includes.bat.in"
diff "cmake/libprotobuf-lite.cmake.orginal" "cmake/libprotobuf-lite.cmake"
diff "cmake/libprotobuf.cmake.orginal" "cmake/libprotobuf.cmake"
diff "cmake/libprotoc.cmake.orginal" "cmake/libprotoc.cmake"
diff "cmake/tests.cmake.orginal" "cmake/tests.cmake"
