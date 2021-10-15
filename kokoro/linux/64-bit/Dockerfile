# This Dockerfile specifies the recipe for creating an image for the tests
# to run in.
#
# We install as many test dependencies here as we can, because these setup
# steps can be cached.  They do *not* run every time we run the build.
# The Docker image is only rebuilt when the Dockerfile (ie. this file)
# changes.

FROM debian:jessie

# Install dependencies.  We start with the basic ones require to build protoc
# and the C++ build
RUN apt-get update && apt-get install -y --force-yes \
  autoconf \
  autotools-dev \
  build-essential \
  bzip2 \
  ccache \
  curl \
  gcc \
  git \
  libc6 \
  libc6-dbg \
  libc6-dev \
  libgtest-dev \
  libtool \
  make \
  parallel \
  time \
  wget \
  # -- For all Java builds -- \
  maven \
  # -- For java_jdk7 -- \
  openjdk-7-jdk \
  # -- For C++ benchmarks --
  cmake \
  valgrind   \
  libxml2-dev \
  && apt-get clean

##################
# Prepare ccache

RUN ln -s /usr/bin/ccache /usr/local/bin/gcc
RUN ln -s /usr/bin/ccache /usr/local/bin/g++
RUN ln -s /usr/bin/ccache /usr/local/bin/cc
RUN ln -s /usr/bin/ccache /usr/local/bin/c++
RUN ln -s /usr/bin/ccache /usr/local/bin/clang
RUN ln -s /usr/bin/ccache /usr/local/bin/clang++

# Define the default command.
CMD ["bash"]
