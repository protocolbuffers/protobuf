FROM debian:stretch

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
  # Java dependencies
  maven \
  openjdk-8-jdk \
  && apt-get clean

# Install rvm
RUN gpg --keyserver hkp://keyserver.ubuntu.com --recv-keys \
    409B6B1796C275462A1703113804BB82D39DC0E3 \
    7D2BAF1CF37B13E2069D6956105BD0E739499BDB
RUN \curl -sSL https://raw.githubusercontent.com/rvm/rvm/master/binscripts/rvm-installer | bash -s master

RUN /bin/bash -l -c "rvm install 2.5.1"
RUN /bin/bash -l -c "rvm install 2.6.0"
RUN /bin/bash -l -c "rvm install 2.7.0"
RUN /bin/bash -l -c "rvm install 3.0.0"
RUN /bin/bash -l -c "rvm install 3.1.0"
RUN /bin/bash -l -c "rvm install jruby-9.2.20.1"
RUN /bin/bash -l -c "rvm install jruby-9.3.3.0"
RUN /bin/bash -l -c "rvm install jruby-9.3.4.0"

RUN /bin/bash -l -c "echo 'gem: --no-ri --no-rdoc' > ~/.gemrc"
RUN /bin/bash -l -c "echo 'export PATH=/usr/local/rvm/bin:$PATH' >> ~/.bashrc"
