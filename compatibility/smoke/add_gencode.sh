#!/bin/bash
set -e -x

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ] || { [ "$#" -eq 2 ] && [ "$2" != "lite" ]; }; then
	echo "Usage: $0 VERSION [lite]"
	echo "  VERSION: a version with no v prefix (e.g. 32.1)"
	echo "  lite: also generate lite gencode into vVERSION/lite"
  exit 1
fi

VER="$1"
LITE="$2"
TMP_DIR="/tmp/protoc-$VER/"

# Cleanup the tmp dir on either clean or dirty exit
trap "rm -r \"$TMP_DIR\"" EXIT

cd /tmp/
wget -O protoc-$VER.zip "https://github.com/protocolbuffers/protobuf/releases/download/v$VER/protoc-$VER-linux-x86_64.zip"
unzip protoc-$VER.zip -d "protoc-$VER/"
cd -

PROTOC="/tmp/protoc-$VER/bin/protoc"

$PROTOC --version

mkdir -p v$VER
$PROTOC proto3_gencode_test.proto --java_out=v$VER

cp CHECKED_IN_GENCODE_BUILD.bazel.template v$VER/BUILD.bazel

if [ "$LITE" == "lite" ]; then
	mkdir -p v$VER/lite
	$PROTOC proto3_gencode_test.proto --java_out=lite:v$VER/lite
	cp CHECKED_IN_LITE_GENCODE_BUILD.bazel.template v$VER/lite/BUILD.bazel

	# Append the new version at the end of the version list for tests.
	echo -e "stale_gencode_smoke_test(\"$VER\", also_lite = True)\n" >> BUILD.bazel
else
	# Append the new version at the end of the version list for tests.
	echo -e "stale_gencode_smoke_test(\"$VER\")\n" >> BUILD.bazel
fi
