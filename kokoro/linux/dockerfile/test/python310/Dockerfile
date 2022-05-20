FROM python:3.10-buster

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
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

# Install Python libraries.
RUN python -m pip install --no-cache-dir --upgrade \
  pip \
  setuptools \
  tox \
  wheel
