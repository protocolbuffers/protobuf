# This Dockerfile specifies the recipe for creating an image for the tests
# to run in.
#
# We install as many test dependencies here as we can, because these setup
# steps can be cached.  They do *not* run every time we run the build.
# The Docker image is only rebuilt when the Dockerfile (ie. this file)
# changes.

# Base Dockerfile for gRPC dev images
FROM debian:latest

# Apt source for old Python versions.
RUN echo 'deb http://ppa.launchpad.net/fkrull/deadsnakes/ubuntu trusty main' > /etc/apt/sources.list.d/deadsnakes.list && \
  apt-key adv --keyserver keyserver.ubuntu.com --recv-keys DB82666C

# Apt source for Oracle Java.
run echo 'deb http://ppa.launchpad.net/webupd8team/java/ubuntu trusty main' > /etc/apt/sources.list.d/webupd8team-java-trusty.list && \
  apt-key adv --keyserver keyserver.ubuntu.com --recv-keys EEA14886 && \
  echo "oracle-java7-installer shared/accepted-oracle-license-v1-1 select true" | debconf-set-selections

# Apt source for Mono
run echo "deb http://download.mono-project.com/repo/debian wheezy main" | tee /etc/apt/sources.list.d/mono-xamarin.list && \
  echo "deb http://download.mono-project.com/repo/debian wheezy-libjpeg62-compat main" | tee -a /etc/apt/sources.list.d/mono-xamarin.list && \
  apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF

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
  # -- For csharp --
  mono-devel \
  referenceassemblies-pcl \
  nunit \
  # -- For all Java builds -- \
  maven \
  # -- For java_jdk6 -- \
  #   oops! not in jessie. too old? openjdk-6-jdk \
  # -- For java_jdk7 -- \
  openjdk-7-jdk \
  # -- For java_oracle7 -- \
  oracle-java7-installer \
  # -- For python / python_cpp -- \
  python-setuptools \
  python-pip \
  python-dev \
  python2.6-dev \
  python3.3-dev \
  python3.4-dev \
  # -- For Ruby --
  ruby \
  && apt-get clean

##################
# C# dependencies

RUN wget www.nuget.org/NuGet.exe -O /usr/local/bin/nuget.exe

##################
# Python dependencies

# These packages exist in apt-get, but their versions are too old, so we have
# to get updates from pip.

RUN pip install pip --upgrade
RUN pip install virtualenv tox yattag


##################
# Ruby dependencies

# Install rvm
RUN gpg --keyserver hkp://keys.gnupg.net --recv-keys 409B6B1796C275462A1703113804BB82D39DC0E3
RUN \curl -sSL https://get.rvm.io | bash -s stable

# Install Ruby 2.1
RUN /bin/bash -l -c "rvm install ruby-2.1"
RUN /bin/bash -l -c "rvm use --default ruby-2.1"
RUN /bin/bash -l -c "echo 'gem: --no-ri --no-rdoc' > ~/.gemrc"
RUN /bin/bash -l -c "echo 'export PATH=/usr/local/rvm/bin:$PATH' >> ~/.bashrc"
RUN /bin/bash -l -c "echo 'rvm --default use ruby-2.1' >> ~/.bashrc"
RUN /bin/bash -l -c "gem install bundler --no-ri --no-rdoc"

##################
# Java dependencies

# This step requires compiling protoc. :(

ENV MAVEN_REPO /var/maven_local_repository
ENV MVN mvn --batch-mode

RUN cd /tmp && \
  git clone https://github.com/google/protobuf.git && \
  cd protobuf && \
  ./autogen.sh && \
  ./configure && \
  make -j6 && \
  cd java && \
  $MVN install dependency:go-offline -Dmaven.repo.local=$MAVEN_REPO -P lite && \
  $MVN install dependency:go-offline -Dmaven.repo.local=$MAVEN_REPO && \
  cd ../javanano && \
  $MVN install dependency:go-offline -Dmaven.repo.local=$MAVEN_REPO

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
