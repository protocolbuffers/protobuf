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
  re2c \
  sqlite3 \
  vim \
  libonig-dev \
  libsqlite3-dev \
  && apt-get clean

# Install php dependencies
RUN apt-get clean && apt-get update && apt-get install -y --force-yes \
  php5 \
  libcurl4-openssl-dev \
  libgmp-dev \
  libgmp3-dev \
  libssl-dev \
  libxml2-dev \
  unzip \
  zlib1g-dev \
  pkg-config \
  && apt-get clean

# Install other dependencies
RUN ln -sf /usr/include/x86_64-linux-gnu/gmp.h /usr/include/gmp.h
RUN wget https://ftp.gnu.org/gnu/bison/bison-3.0.1.tar.gz -O /var/local/bison-3.0.1.tar.gz
RUN cd /var/local \
  && tar -zxvf bison-3.0.1.tar.gz \
  && cd /var/local/bison-3.0.1 \
  && ./configure \
  && make \
  && make install

# Install composer
RUN curl -sS https://getcomposer.org/installer | php
RUN mv composer.phar /usr/local/bin/composer

# Download php source code
RUN git clone https://github.com/php/php-src

# php 8.0
RUN cd php-src \
  && git checkout php-8.0.0 \
  && ./buildconf --force
RUN cd php-src \
  && ./configure \
  --enable-bcmath \
  --enable-mbstring \
  --with-gmp \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-8.0 \
  && make \
  && make install \
  && make clean
RUN cd php-src \
  && ./configure \
  --enable-bcmath \
  --enable-mbstring \
  --enable-maintainer-zts \
  --with-gmp \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-8.0-zts \
  && make \
  && make install \
  && make clean

RUN wget -O phpunit https://phar.phpunit.de/phpunit-9.phar \
  && chmod +x phpunit \
  && cp phpunit /usr/local/php-8.0/bin \
  && mv phpunit /usr/local/php-8.0-zts/bin

# Install php dependencies
RUN apt-get clean && apt-get update && apt-get install -y --force-yes \
  valgrind \
  && apt-get clean
