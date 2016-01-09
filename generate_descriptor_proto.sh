#!/bin/sh

# Run this script to regenerate descriptor.pb.{h,cc} after the protocol
# compiler changes.  Since these files are compiled into the protocol compiler
# itself, they cannot be generated automatically by a make rule.  "make check"
# will fail if these files do not match what the protocol compiler would
# generate.
#
# HINT:  Flags passed to generate_descriptor_proto.sh will be passed directly
#   to make when building protoc.  This is particularly useful for passing
#   -j4 to run 4 jobs simultaneously.

if test ! -e src/google/protobuf/stubs/common.h; then
  cat >&2 << __EOF__
Could not find source code.  Make sure you are running this script from the
root of the distribution tree.
__EOF__
  exit 1
fi

if test ! -e src/Makefile; then
  cat >&2 << __EOF__
Could not find src/Makefile.  You must run ./configure (and perhaps
./autogen.sh) first.
__EOF__
  exit 1
fi

cd src

declare -a RUNTIME_PROTO_FILES=(\
  google/protobuf/any.proto \
  google/protobuf/api.proto \
  google/protobuf/descriptor.proto \
  google/protobuf/duration.proto \
  google/protobuf/empty.proto \
  google/protobuf/field_mask.proto \
  google/protobuf/source_context.proto \
  google/protobuf/struct.proto \
  google/protobuf/timestamp.proto \
  google/protobuf/type.proto \
  google/protobuf/wrappers.proto)

CORE_PROTO_IS_CORRECT=0
PROCESS_ROUND=1
echo "Updating descriptor protos..."
while [ $CORE_PROTO_IS_CORRECT -ne 1 ]
do
  echo "Round $PROCESS_ROUND"
  CORE_PROTO_IS_CORRECT=1
  for PROTO_FILE in ${RUNTIME_PROTO_FILES[@]}; do
    BASE_NAME=${PROTO_FILE%.*}
    cp ${BASE_NAME}.pb.h ${BASE_NAME}.pb.h.tmp
    cp ${BASE_NAME}.pb.cc ${BASE_NAME}.pb.cc.tmp
  done
  cp google/protobuf/compiler/plugin.pb.h google/protobuf/compiler/plugin.pb.h.tmp
  cp google/protobuf/compiler/plugin.pb.cc google/protobuf/compiler/plugin.pb.cc.tmp

  make $@ protoc &&
    ./protoc --cpp_out=dllexport_decl=LIBPROTOBUF_EXPORT:. ${RUNTIME_PROTO_FILES[@]} && \
    ./protoc --cpp_out=dllexport_decl=LIBPROTOC_EXPORT:. google/protobuf/compiler/plugin.proto

  for PROTO_FILE in ${RUNTIME_PROTO_FILES[@]}; do
    BASE_NAME=${PROTO_FILE%.*}
    diff ${BASE_NAME}.pb.h ${BASE_NAME}.pb.h.tmp > /dev/null
    if test $? -ne 0; then
      CORE_PROTO_IS_CORRECT=0
    fi
    diff ${BASE_NAME}.pb.cc ${BASE_NAME}.pb.cc.tmp > /dev/null
    if test $? -ne 0; then
      CORE_PROTO_IS_CORRECT=0
    fi
  done

  diff google/protobuf/compiler/plugin.pb.h google/protobuf/compiler/plugin.pb.h.tmp > /dev/null
  if test $? -ne 0; then
    CORE_PROTO_IS_CORRECT=0
  fi
  diff google/protobuf/compiler/plugin.pb.cc google/protobuf/compiler/plugin.pb.cc.tmp > /dev/null
  if test $? -ne 0; then
    CORE_PROTO_IS_CORRECT=0
  fi

  for PROTO_FILE in ${RUNTIME_PROTO_FILES[@]}; do
    BASE_NAME=${PROTO_FILE%.*}
    rm ${BASE_NAME}.pb.h.tmp
    rm ${BASE_NAME}.pb.cc.tmp
  done
  rm google/protobuf/compiler/plugin.pb.h.tmp
  rm google/protobuf/compiler/plugin.pb.cc.tmp

  PROCESS_ROUND=$((PROCESS_ROUND + 1))
done
cd ..

if test -x objectivec/generate_descriptors_proto.sh; then
  echo "Generating messages for objc."
  objectivec/generate_descriptors_proto.sh $@
fi
