#!/bin/bash

# Override the default value set in configure.ac that has '-g' which produces
# huge binary.
export CXXFLAGS=-DNDEBUG

cd $(dirname "$0")/.. && ./configure --disable-shared && make &&
  cd src && (strip protoc || strip protoc.exe)
