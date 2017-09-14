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
RUN echo 'deb http://ppa.launchpad.net/webupd8team/java/ubuntu trusty main' > /etc/apt/sources.list.d/webupd8team-java-trusty.list && \
  apt-key adv --keyserver keyserver.ubuntu.com --recv-keys EEA14886 && \
  echo "oracle-java7-installer shared/accepted-oracle-license-v1-1 select true" | debconf-set-selections

# Apt source for Mono
RUN echo "deb http://download.mono-project.com/repo/debian wheezy main" | tee /etc/apt/sources.list.d/mono-xamarin.list && \
  echo "deb http://download.mono-project.com/repo/debian wheezy-libjpeg62-compat main" | tee -a /etc/apt/sources.list.d/mono-xamarin.list && \
  apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF

# Apt source for php
RUN echo "deb http://ppa.launchpad.net/ondrej/php/ubuntu trusty main" | tee /etc/apt/sources.list.d/various-php.list && \
  apt-key adv --keyserver keyserver.ubuntu.com --recv-keys F4FCBB07

# Install dotnet SDK based on https://www.microsoft.com/net/core#debian
# (Ubuntu instructions need apt to support https)
RUN apt-get update && apt-get install -y --force-yes curl libunwind8 gettext && \
  curl -sSL -o dotnet.tar.gz https://go.microsoft.com/fwlink/?LinkID=847105 &&  \
  mkdir -p /opt/dotnet && tar zxf dotnet.tar.gz -C /opt/dotnet && \
  ln -s /opt/dotnet/dotnet /usr/local/bin

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
  # -- For C++ benchmarks --
  cmake \
  # -- For PHP --
  php5.6     \
  php5.6-dev \
  php5.6-xml \
  php7.0     \
  php7.0-dev \
  php7.0-xml \
  phpunit    \
  valgrind   \
  libxml2-dev \
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

# Install Ruby 2.1, Ruby 2.2 and JRuby 1.7
RUN /bin/bash -l -c "rvm install ruby-2.1"
RUN /bin/bash -l -c "rvm install ruby-2.2"
RUN /bin/bash -l -c "rvm install jruby-1.7"
RUN /bin/bash -l -c "echo 'gem: --no-ri --no-rdoc' > ~/.gemrc"
RUN /bin/bash -l -c "echo 'export PATH=/usr/local/rvm/bin:$PATH' >> ~/.bashrc"
RUN /bin/bash -l -c "gem install bundler --no-ri --no-rdoc"

##################
# Java dependencies

# This step requires compiling protoc. :(

ENV MAVEN_REPO /var/maven_local_repository
ENV MVN mvn --batch-mode

RUN cd /tmp && \
  git clone https://github.com/google/protobuf.git && \
  cd protobuf && \
  git reset --hard 129a6e2aca95dcfb6c3e717d7b9cca1f104fde39 && \
  ./autogen.sh && \
  ./configure && \
  make -j4 && \
  cd java && \
  $MVN install dependency:go-offline -Dmaven.repo.local=$MAVEN_REPO && \
  cd ../javanano && \
  $MVN install dependency:go-offline -Dmaven.repo.local=$MAVEN_REPO

##################
# PHP dependencies.
RUN wget http://am1.php.net/get/php-5.5.38.tar.bz2/from/this/mirror
RUN mv mirror php-5.5.38.tar.bz2
RUN tar -xvf php-5.5.38.tar.bz2
RUN cd php-5.5.38 && ./configure --enable-maintainer-zts --prefix=/usr/local/php-5.5-zts && \
    make && make install && cd ..
RUN cd php-5.5.38 && make clean && ./configure --enable-bcmath --prefix=/usr/local/php-5.5 && \
    make && make install && cd ..

RUN wget http://am1.php.net/get/php-5.6.30.tar.bz2/from/this/mirror
RUN mv mirror php-5.6.30.tar.bz2
RUN tar -xvf php-5.6.30.tar.bz2
RUN cd php-5.6.30 && ./configure --enable-maintainer-zts --prefix=/usr/local/php-5.6-zts && \
    make && make install && cd ..
RUN cd php-5.6.30 && make clean && ./configure --enable-bcmath --prefix=/usr/local/php-5.6 && \
    make && make install && cd ..

RUN wget http://am1.php.net/get/php-7.0.18.tar.bz2/from/this/mirror
RUN mv mirror php-7.0.18.tar.bz2
RUN tar -xvf php-7.0.18.tar.bz2
RUN cd php-7.0.18 && ./configure --enable-maintainer-zts --prefix=/usr/local/php-7.0-zts && \
    make && make install && cd ..
RUN cd php-7.0.18 && make clean && ./configure --enable-bcmath --prefix=/usr/local/php-7.0 && \
    make && make install && cd ..

RUN wget http://am1.php.net/get/php-7.1.4.tar.bz2/from/this/mirror
RUN mv mirror php-7.1.4.tar.bz2
RUN tar -xvf php-7.1.4.tar.bz2
RUN cd php-7.1.4 && ./configure --enable-maintainer-zts --prefix=/usr/local/php-7.1-zts && \
    make && make install && cd ..
RUN cd php-7.1.4 && make clean && ./configure --enable-bcmath --prefix=/usr/local/php-7.1 && \
    make && make install && cd ..

RUN php -r "copy('https://getcomposer.org/installer', 'composer-setup.php');"
RUN php composer-setup.php
RUN mv composer.phar /usr/bin/composer
RUN php -r "unlink('composer-setup.php');"
RUN composer config -g -- disable-tls true
RUN composer config -g -- secure-http false
RUN cd /tmp && \
  rm -rf protobuf && \
  git clone https://github.com/google/protobuf.git && \
  cd protobuf && \
  git reset --hard 49b44bff2b6257a119f9c6a342d6151c736586b8 && \
  cd php && \
  ln -sfn /usr/local/php-5.5/bin/php /usr/bin/php && \
  ln -sfn /usr/local/php-5.5/bin/php-config /usr/bin/php-config && \
  ln -sfn /usr/local/php-5.5/bin/phpize /usr/bin/phpize && \
  composer install && \
  mv vendor /usr/local/vendor-5.5 && \
  ln -sfn /usr/local/php-5.6/bin/php /usr/bin/php && \
  ln -sfn /usr/local/php-5.6/bin/php-config /usr/bin/php-config && \
  ln -sfn /usr/local/php-5.6/bin/phpize /usr/bin/phpize && \
  composer install && \
  mv vendor /usr/local/vendor-5.6 && \
  ln -sfn /usr/local/php-7.0/bin/php /usr/bin/php && \
  ln -sfn /usr/local/php-7.0/bin/php-config /usr/bin/php-config && \
  ln -sfn /usr/local/php-7.0/bin/phpize /usr/bin/phpize && \
  composer install && \
  mv vendor /usr/local/vendor-7.0 && \
  ln -sfn /usr/local/php-7.1/bin/php /usr/bin/php && \
  ln -sfn /usr/local/php-7.1/bin/php-config /usr/bin/php-config && \
  ln -sfn /usr/local/php-7.1/bin/phpize /usr/bin/phpize && \
  composer install && \
  mv vendor /usr/local/vendor-7.1

##################
# Go dependencies.
RUN apt-get install -y  \
  # -- For go -- \
  golang

##################
# Javascript dependencies.
RUN apt-get install -y \
  # -- For javascript -- \
  npm

# On Debian/Ubuntu, nodejs binary is named 'nodejs' because the name 'node'
# is taken by another legacy binary. We don't have that legacy binary and
# npm expects the binary to be named 'node', so we just create a symbol
# link here.
RUN ln -s `which nodejs` /usr/bin/node

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
