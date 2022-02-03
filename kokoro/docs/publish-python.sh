#!/bin/bash
# Adapted from https://github.com/googleapis/google-cloud-python/blob/master/.kokoro/publish-docs.sh

set -eo pipefail

# Disable buffering, so that the logs stream through.
export PYTHONUNBUFFERED=1

cd github/protobuf/python

# install package
sudo apt-get update
sudo apt-get -y install software-properties-common
sudo add-apt-repository universe
sudo apt-get update
sudo apt-get -y install unzip
wget https://github.com/protocolbuffers/protobuf/releases/download/v3.15.8/protoc-3.15.8-linux-x86_64.zip
unzip protoc-3.15.8-linux-x86_64.zip bin/protoc
mv bin/protoc ../src/protoc
python3 -m venv venv
source venv/bin/activate
python setup.py install

# install docs dependencies
python -m pip install -r docs/requirements.txt

# build docs
cd docs
make html
cd ..
deactivate

python3 -m pip install protobuf==3.15.8 gcp-docuploader

# install a json parser
sudo apt-get -y install jq

# create metadata
python3 -m docuploader create-metadata \
  --name=$(jq --raw-output '.name // empty' .repo-metadata.json) \
  --version=$(python3 setup.py --version) \
  --language=$(jq --raw-output '.language // empty' .repo-metadata.json) \
  --distribution-name=$(python3 setup.py --name) \
  --product-page=$(jq --raw-output '.product_documentation // empty' .repo-metadata.json) \
  --github-repository=$(jq --raw-output '.repo // empty' .repo-metadata.json) \
  --issue-tracker=$(jq --raw-output '.issue_tracker // empty' .repo-metadata.json)

cat docs.metadata

# upload docs
python3 -m docuploader upload docs/_build/html --metadata-file docs.metadata --staging-bucket docs-staging
