#!/bin/sh

# This script copies source file lists from src/Makefile.am to cmake files.

get_variable_value() {
  FILENAME=$1
  VARNAME=$2
  awk "
    BEGIN { start = 0; }
    /^$VARNAME =/ { start = 1; }
    { if (start) { print \$0; } }
    /\\\\\$/ { next; }
    { start = 0; }
  " $FILENAME \
    | sed "s/^$VARNAME =//" \
    | sed "s/[ \\]//g" \
    | grep -v "^\\$" \
    | grep -v "^$" \
    | LC_ALL=C sort | uniq
}

get_source_files() {
  get_variable_value $@ | grep "cc$"
}

get_proto_files() {
  get_variable_value $@ | grep "pb.cc$" | sed "s/pb.cc/proto/"
}

set_variable_value() {
  FILENAME=$1
  VARNAME=$2
  PREFIX=$3
  shift
  shift
  shift
  awk -v values="$*" -v prefix="$PREFIX" "
    BEGIN { start = 0; }
    /^set\\($VARNAME/ {
      start = 1;
      print \$0;
      len = split(values, vlist, \" \");
      for (i = 1; i <= len; ++i) {
        printf(\"  %s%s\\n\", prefix, vlist[i]);
      }
      next;
    }
    start && /^\\)/ {
      start = 0;
    }
    !start {
      print \$0;
    }
  " $FILENAME > /tmp/$$
  cp /tmp/$$ $FILENAME
}

sort_files() {
  for FILE in $@; do
    echo $FILE
  done | LC_ALL=C sort | uniq
}

MAKEFILE=../src/Makefile.am
CMAKE_DIR=.
EXTRACT_INCLUDES_BAT=extract_includes.bat.in

[ -f "$MAKEFILE" ] || {
  echo "Cannot find: $MAKEFILE"
  exit 1
}

[ -d "$CMAKE_DIR" ] || {
  echo "Cannot find: $CMAKE_DIR"
  exit 1
}

[ -f "$EXTRACT_INCLUDES_BAT" ] || {
  echo "Cannot find: $EXTRACT_INCLUDES_BAT"
  exit 1
}

# Extract file lists from src/Makefile.am
GZHEADERS=$(get_variable_value $MAKEFILE GZHEADERS)
HEADERS=$(get_variable_value $MAKEFILE nobase_include_HEADERS)
PUBLIC_HEADERS=$(sort_files $GZHEADERS $HEADERS)
LIBPROTOBUF_LITE_SOURCES=$(get_source_files $MAKEFILE libprotobuf_lite_la_SOURCES)
LIBPROTOBUF_SOURCES=$(get_source_files $MAKEFILE libprotobuf_la_SOURCES)
LIBPROTOC_SOURCES=$(get_source_files $MAKEFILE libprotoc_la_SOURCES)
LITE_PROTOS=$(get_proto_files $MAKEFILE protoc_lite_outputs)
PROTOS=$(get_proto_files $MAKEFILE protoc_outputs)
COMMON_TEST_SOURCES=$(get_source_files $MAKEFILE COMMON_TEST_SOURCES)
TEST_SOURCES=$(get_source_files $MAKEFILE protobuf_test_SOURCES)
LITE_TEST_SOURCES=$(get_source_files $MAKEFILE protobuf_lite_test_SOURCES)

# Replace file lists in cmake files.
COMMON_PREFIX="\${protobuf_source_dir}/src/"
set_variable_value $CMAKE_DIR/libprotobuf-lite.cmake libprotobuf_lite_files $COMMON_PREFIX $LIBPROTOBUF_LITE_SOURCES
set_variable_value $CMAKE_DIR/libprotobuf.cmake libprotobuf_files $COMMON_PREFIX $LIBPROTOBUF_SOURCES
set_variable_value $CMAKE_DIR/libprotoc.cmake libprotoc_files $COMMON_PREFIX $LIBPROTOC_SOURCES
set_variable_value $CMAKE_DIR/tests.cmake lite_test_protos "" $LITE_PROTOS
set_variable_value $CMAKE_DIR/tests.cmake tests_protos "" $PROTOS
set_variable_value $CMAKE_DIR/tests.cmake common_test_files $COMMON_PREFIX $COMMON_TEST_SOURCES
set_variable_value $CMAKE_DIR/tests.cmake tests_files $COMMON_PREFIX $TEST_SOURCES
set_variable_value $CMAKE_DIR/tests.cmake lite_test_files $COMMON_PREFIX $LITE_TEST_SOURCES

# Generate extract_includes.bat
echo "mkdir include" > $EXTRACT_INCLUDES_BAT
for HEADER in $PUBLIC_HEADERS; do
  HEADER_DIR=$(dirname $HEADER)
  while [ ! "$HEADER_DIR" = "." ]; do
    echo $HEADER_DIR | sed "s/\\//\\\\/g"
    HEADER_DIR=$(dirname $HEADER_DIR)
  done
done | sort | uniq | sed "s/^/mkdir include\\\\/" >> $EXTRACT_INCLUDES_BAT
for HEADER in $PUBLIC_HEADERS; do
  WINPATH=$(echo $HEADER | sed 's;/;\\;g')
  echo "copy \${PROTOBUF_SOURCE_WIN32_PATH}\\..\\src\\$WINPATH include\\$WINPATH" >> $EXTRACT_INCLUDES_BAT
done
# Add pbconfig.h.
echo "copy \${PROTOBUF_BINARY_WIN32_PATH}\\google\\protobuf\\stubs\\pbconfig.h include\\google\\protobuf\\stubs\\pbconfig.h" >> $EXTRACT_INCLUDES_BAT
