# Despite the name of this image, we are no longer on stretch.
# We should consider renaming this image, and/or evaluating what
# software versions we actually need.
FROM debian:bullseye

# Install dependencies.  We start with the basic ones required to build protoc
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
  libgtest-dev \
  libtool \
  make \
  parallel \
  pkg-config \
  time \
  wget \
  # Java dependencies
  maven \
  openjdk-11-jdk \
  openjdk-17-jdk \
  # Required for the gtest build.
  python2 \
  # Python dependencies
  python3-dev \
  python3-setuptools \
  python3-pip \
  python3-venv \
  && apt-get clean
