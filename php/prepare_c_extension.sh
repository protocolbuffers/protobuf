
# wyhash has to live in the base third_party directory.
# We copy it into the ext/google/protobuf directory for the build
# (and for the release to PECL).
mkdir -p ../ext/google/protobuf/third_party/wyhash
cp ../../third_party/wyhash/* ../ext/google/protobuf/third_party/wyhash
