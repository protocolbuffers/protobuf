FROM debian:bullseye

# Install dependencies.  We start with the basic ones require to build protoc
# and the C++ build
RUN apt-get update && apt-get install -y \
  autoconf \
  autotools-dev \
  build-essential \
  bzip2 \
  ccache \
  cmake \
  curl \
  gcc \
  git \
  libabsl-dev \
  libc6 \
  libc6-dbg \
  libc6-dev \
  libgmock-dev \
  libgtest-dev \
  libtool \
  make \
  parallel \
  pkg-config \
  time \
  wget \
  && apt-get clean

# Install dependencies for TC malloc
RUN apt-get install -y \
  google-perftools \
  libgoogle-perftools4 \
  libgoogle-perftools-dev
