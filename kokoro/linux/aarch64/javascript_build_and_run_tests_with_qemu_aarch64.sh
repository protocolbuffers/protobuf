#!/bin/bash

set -ex

# install the same version of node as in /tests.sh
NODE_VERSION=node-v12.16.3-linux-arm64
NODE_TGZ="$NODE_VERSION.tar.gz"
pushd /tmp
curl -OL https://nodejs.org/dist/v12.16.3/$NODE_TGZ
tar zxvf $NODE_TGZ
export PATH=$PATH:`pwd`/$NODE_VERSION/bin
popd

# go to the repo root
cd $(dirname $0)/../../..

cd js
npm install
npm test
