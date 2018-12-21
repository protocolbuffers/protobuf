FROM 32bit/debian:jessie

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

# Install php dependencies
RUN apt-get clean && apt-get update && apt-get install -y --force-yes \
  bison \
  php5 \
  libcurl4-openssl-dev \
  libssl-dev \
  libxml2-dev \
  unzip \
  zlib1g-dev \
  pkg-config \
  && apt-get clean

# Install other dependencies
RUN wget http://ftp.gnu.org/gnu/bison/bison-2.6.4.tar.gz -O /var/local/bison-2.6.4.tar.gz
RUN cd /var/local \
  && tar -zxvf bison-2.6.4.tar.gz \
  && cd /var/local/bison-2.6.4 \
  && ./configure \
  && make \
  && make install

# Install composer
RUN curl -sS https://getcomposer.org/installer | php
RUN mv composer.phar /usr/local/bin/composer

# Download php source code
RUN git clone https://github.com/php/php-src

# php 5.5
RUN cd php-src \
  && git checkout PHP-5.5.38 \
  && ./buildconf --force
RUN cd php-src \
  && ./configure \
  --enable-bcmath \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-5.5 \
  && make \
  && make install \
  && make clean
RUN cd php-src \
  && ./configure \
  --enable-maintainer-zts \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-5.5-zts \
  && make \
  && make install \
  && make clean

RUN wget -O phpunit https://phar.phpunit.de/phpunit-4.phar \
  && chmod +x phpunit \
  && cp phpunit /usr/local/php-5.5/bin \
  && mv phpunit /usr/local/php-5.5-zts/bin

# php 5.6
RUN cd php-src \
  && git checkout PHP-5.6.39 \
  && ./buildconf --force
RUN cd php-src \
  && ./configure \
  --enable-bcmath \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-5.6 \
  && make \
  && make install \
  && make clean
RUN cd php-src \
  && ./configure \
  --enable-maintainer-zts \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-5.6-zts \
  && make \
  && make install \
  && make clean

RUN wget -O phpunit https://phar.phpunit.de/phpunit-5.phar \
  && chmod +x phpunit \
  && cp phpunit /usr/local/php-5.6/bin \
  && mv phpunit /usr/local/php-5.6-zts/bin

# php 7.0
RUN cd php-src \
  && git checkout PHP-7.0.33 \
  && ./buildconf --force
RUN cd php-src \
  && ./configure \
  --enable-bcmath \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.0 \
  && make \
  && make install \
  && make clean
RUN cd php-src \
  && ./configure \
  --enable-maintainer-zts \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.0-zts \
  && make \
  && make install \
  && make clean

RUN wget -O phpunit https://phar.phpunit.de/phpunit-6.phar \
  && chmod +x phpunit \
  && cp phpunit /usr/local/php-7.0/bin \
  && mv phpunit /usr/local/php-7.0-zts/bin

# php 7.1
RUN cd php-src \
  && git checkout PHP-7.1.25 \
  && ./buildconf --force
RUN cd php-src \
  && ./configure \
  --enable-bcmath \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.1 \
  && make \
  && make install \
  && make clean
RUN cd php-src \
  && ./configure \
  --enable-maintainer-zts \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.1-zts \
  && make \
  && make install \
  && make clean

RUN wget -O phpunit https://phar.phpunit.de/phpunit-7.phar \
  && chmod +x phpunit \
  && cp phpunit /usr/local/php-7.1/bin \
  && mv phpunit /usr/local/php-7.1-zts/bin

# php 7.2
RUN cd php-src \
  && git checkout PHP-7.2.13 \
  && ./buildconf --force
RUN cd php-src \
  && ./configure \
  --enable-bcmath \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.2 \
  && make \
  && make install \
  && make clean
RUN cd php-src \
  && ./configure \
  --enable-maintainer-zts \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.2-zts \
  && make \
  && make install \
  && make clean

RUN wget -O phpunit https://phar.phpunit.de/phpunit-7.phar \
  && chmod +x phpunit \
  && cp phpunit /usr/local/php-7.2/bin \
  && mv phpunit /usr/local/php-7.2-zts/bin

# php 7.3
RUN cd php-src \
  && git checkout PHP-7.3.0 \
  && ./buildconf --force
RUN cd php-src \
  && ./configure \
  --enable-bcmath \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.3 \
  && make \
  && make install \
  && make clean
RUN cd php-src \
  && ./configure \
  --enable-maintainer-zts \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.3-zts \
  && make \
  && make install \
  && make clean

RUN wget -O phpunit https://phar.phpunit.de/phpunit-7.phar \
  && chmod +x phpunit \
  && cp phpunit /usr/local/php-7.3/bin \
  && mv phpunit /usr/local/php-7.3-zts/bin

# Install php dependencies
RUN apt-get clean && apt-get update && apt-get install -y --force-yes \
  valgrind \
  && apt-get clean
