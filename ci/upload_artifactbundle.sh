#!/bin/bash

# This script generates an artifactbundle for protoc. This artifactbundle
# is used by the Swift package manger. The script is run by a GitHub action
# when a new release is created.

set -ex

# We have to filter for the correct release. protoc releases are following the vX.X scheme
# but the repo also has language specific vX.X.X releases. $GITHUB_REF is in the format of refs/tags/<tag_name>
TAG=$GITHUB_REF
TAG="${TAG#???????????}"

if [[ ! "$TAG" =~ ^[0-9]+\.[0-9]+$ ]]; then
    echo "Error: $TAG does not match the expected pattern"
    exit 1
fi

AUTH="Authorization: token $GITHUB_TOKEN"

# Fetch all protoc release assets
curl -LJ --output protoc-$TAG-osx-x86_64.zip -H 'Accept: application/octet-stream' https://github.com/$GITHUB_REPOSITORY/releases/download/v$TAG/protoc-24.3-osx-x86_64.zip
curl -LJ --output protoc-$TAG-osx-aarch_64.zip -H 'Accept: application/octet-stream' https://github.com/$GITHUB_REPOSITORY/releases/download/v$TAG/protoc-24.3-osx-aarch_64.zip
curl -LJ --output protoc-$TAG-linux-aarch_64.zip -H 'Accept: application/octet-stream' https://github.com/$GITHUB_REPOSITORY/releases/download/v$TAG/protoc-24.3-linux-aarch_64.zip
curl -LJ --output protoc-$TAG-linux-x86_64.zip -H 'Accept: application/octet-stream' https://github.com/$GITHUB_REPOSITORY/releases/download/v$TAG/protoc-24.3-linux-x86_64.zip
curl -LJ --output protoc-$TAG-win64.zip -H 'Accept: application/octet-stream' https://github.com/$GITHUB_REPOSITORY/releases/download/v$TAG/protoc-24.3-win64.zip

# Unzip all assets
mkdir protoc-$TAG.artifactbundle
unzip -d protoc-$TAG.artifactbundle/protoc-$TAG-osx-x86_64 protoc-$TAG-osx-x86_64.zip
unzip -d protoc-$TAG.artifactbundle/protoc-$TAG-osx-aarch_64 protoc-$TAG-osx-aarch_64.zip
unzip -d protoc-$TAG.artifactbundle/protoc-$TAG-linux-aarch_64 protoc-$TAG-linux-aarch_64.zip
unzip -d protoc-$TAG.artifactbundle/protoc-$TAG-linux-x86_64 protoc-$TAG-linux-x86_64.zip
unzip -d protoc-$TAG.artifactbundle/protoc-$TAG-win64 protoc-$TAG-win64.zip

# Create info.json for artifactbundle
cat > protoc-$TAG.artifactbundle/info.json << EOF
{
    "schemaVersion": "1.0",
    "artifacts": {
        "protoc": {
            "type": "executable",
            "version": "$TAG",
            "variants": [
                {
                    "path": "protoc-$TAG-linux-x86_64/bin/protoc",
                    "supportedTriples": ["x86_64-unknown-linux-gnu"]
                },
                {
                    "path": "protoc-$TAG-linux-aarch_64/bin/protoc",
                    "supportedTriples": ["aarch64-unknown-linux-gnu", "arm64-unknown-linux-gnu", "aarch64-unknown-linux", "arm64-unknown-linux"]
                },
                {
                    "path": "protoc-$TAG-osx-x86_64/bin/protoc",
                    "supportedTriples": ["x86_64-apple-macosx"]
                },
                {
                    "path": "protoc-$TAG-osx-aarch_64/bin/protoc",
                    "supportedTriples": ["arm64-apple-macosx"]
                },
                {
                    "path": "protoc-$TAG-win64/bin/protoc.exe",
                    "supportedTriples": ["x86_64-unknown-windows"]
                },
            ]
        }
    }
}
EOF

# Zip artifactbundle
zip -r protoc-$TAG.artifactbundle.zip protoc-$TAG.artifactbundle

# Get asset upload url for tag
response=$(curl -sH "$AUTH" "$GITHUB_API_URL/repos/$GITHUB_REPOSITORY/releases/tags/v$TAG")
eval $(echo "$response" | grep -m 1 "id.:" | grep -w id | tr : = | tr -cd '[[:alnum:]]=')
[ "$id" ] || { echo "Error: Failed to get release id for tag: v$TAG"; echo "$response\n" >&2; exit 1; }

# Upload asset
curl --data-binary @protoc-$TAG.artifactbundle.zip -H "$AUTH" -H "Content-Type: application/octet-stream" "https://uploads.github.com/repos/$GITHUB_REPOSITORY/releases/$id/assets?name=protoc-$TAG.artifactbundle.zip"
