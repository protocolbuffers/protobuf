FROM debian:jessie

# Install dependencies.  We start with the basic ones require to build protoc
# and the C++ build
RUN apt-get update && apt-get install -y \
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
  && apt-get clean

# Apt source for Oracle Java 7 is no longer available in webupd8
# https://launchpad.net/~webupd8team/+archive/ubuntu/java

# Java dependencies
RUN apt-get install -y \
  # -- For all Java builds -- \
  maven \
  # -- For java_jdk7 -- \
  openjdk-7-jdk \
  && apt-get clean
