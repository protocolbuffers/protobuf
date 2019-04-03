FROM ubuntu:latest

RUN apt-get update && apt-get install -y gnupg

# Install dependencies.  We start with the basic ones require to build protoc
# and the C++ build
RUN apt-get clean && apt-get update && apt-get install -y --force-yes \
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


##################
# Javascript dependencies.
RUN apt-get install -y \
  # -- For javascript and closure compiler -- \
  npm \
  default-jre
