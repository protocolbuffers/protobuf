FROM debian:stretch

# Install dependencies.  We start with the basic ones required to build protoc
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
  # Java dependencies
  maven \
  openjdk-8-jdk \
  # Python dependencies
  python-setuptools \
  python-pip \
  virtualenv \
  && apt-get clean
