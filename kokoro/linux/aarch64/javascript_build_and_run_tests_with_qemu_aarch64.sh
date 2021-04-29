#!/bin/bash

set -ex

# go to the repo root
cd $(dirname $0)/../../..

cd js
npm install
npm test
