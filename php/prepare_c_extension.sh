
cd $(dirname $0)

if [[ -f ext/google/protobuf/third_party/wyhash/wyhash.h && -z $(find ../third_party/wyhash -newer ext/google/protobuf/third_party) ]]; then
  # Generated protos are already present and up to date, so we can skip protoc.
  #
  # Protoc is very fast, but sometimes it is not available (like if we haven't
  # built it in Docker). Skipping it helps us proceed in this case.
  echo "wyhash is up to date, skipping."
  exit 0
fi

# wyhash has to live in the base third_party directory.
# We copy it into the ext/google/protobuf directory for the build
# (and for the release to PECL).
rm -rf ext/google/protobuf/third_party
mkdir -p ext/google/protobuf/third_party/wyhash
cp ../third_party/wyhash/* ext/google/protobuf/third_party/wyhash

echo "Copied wyhash from ../third_party -> ext/google/protobuf/third_party"
