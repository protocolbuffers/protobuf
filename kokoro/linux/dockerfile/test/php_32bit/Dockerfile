FROM i386/debian:jessie

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
  libsqlite3-dev \
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
RUN php -r "copy('https://getcomposer.org/installer', 'composer-setup.php');"
RUN php composer-setup.php
RUN mv composer.phar /usr/bin/composer
RUN php -r "unlink('composer-setup.php');"

# Download php source code
RUN git clone https://github.com/php/php-src

# php 5.6
RUN cd php-src \
  && git checkout PHP-5.6.39 \
  && ./buildconf --force
RUN cd php-src \
  && ./configure \
  --enable-bcmath \
  --enable-mbstring \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-5.6 \
  && make \
  && make install \
  && make clean

RUN wget -O phpunit https://phar.phpunit.de/phpunit-5.phar \
  && chmod +x phpunit \
  && mv phpunit /usr/local/php-5.6/bin

# php 7.0
RUN wget https://github.com/php/php-src/archive/php-7.0.33.tar.gz -O /var/local/php-7.0.33.tar.gz

RUN cd /var/local \
  && tar -zxvf php-7.0.33.tar.gz

RUN cd /var/local/php-src-php-7.0.33 \
  && ./buildconf --force \
  && ./configure \
  --enable-bcmath \
  --enable-mbstring \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.0 \
  && make \
  && make install \
  && make clean
RUN cd /var/local/php-src-php-7.0.33 \
  && ./buildconf --force \
  && ./configure \
  --enable-maintainer-zts \
  --enable-mbstring \
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
RUN wget https://github.com/php/php-src/archive/php-7.1.25.tar.gz -O /var/local/php-7.1.25.tar.gz

RUN cd /var/local \
  && tar -zxvf php-7.1.25.tar.gz

RUN cd /var/local/php-src-php-7.1.25 \
  && ./buildconf --force \
  && ./configure \
  --enable-bcmath \
  --enable-mbstring \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.1 \
  && make \
  && make install \
  && make clean
RUN cd /var/local/php-src-php-7.1.25 \
  && ./buildconf --force \
  && ./configure \
  --enable-maintainer-zts \
  --enable-mbstring \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.1-zts \
  && make \
  && make install \
  && make clean

RUN wget -O phpunit https://phar.phpunit.de/phpunit-7.5.0.phar \
  && chmod +x phpunit \
  && cp phpunit /usr/local/php-7.1/bin \
  && mv phpunit /usr/local/php-7.1-zts/bin

# php 7.2
RUN wget https://github.com/php/php-src/archive/php-7.2.13.tar.gz -O /var/local/php-7.2.13.tar.gz

RUN cd /var/local \
  && tar -zxvf php-7.2.13.tar.gz

RUN cd /var/local/php-src-php-7.2.13 \
  && ./buildconf --force \
  && ./configure \
  --enable-bcmath \
  --enable-mbstring \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.2 \
  && make \
  && make install \
  && make clean
RUN cd /var/local/php-src-php-7.2.13 \
  && ./buildconf --force \
  && ./configure \
  --enable-maintainer-zts \
  --enable-mbstring \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.2-zts \
  && make \
  && make install \
  && make clean

RUN wget -O phpunit https://phar.phpunit.de/phpunit-7.5.0.phar \
  && chmod +x phpunit \
  && cp phpunit /usr/local/php-7.2/bin \
  && mv phpunit /usr/local/php-7.2-zts/bin

# php 7.3
RUN wget https://github.com/php/php-src/archive/php-7.3.0.tar.gz -O /var/local/php-7.3.0.tar.gz

RUN cd /var/local \
  && tar -zxvf php-7.3.0.tar.gz

RUN cd /var/local/php-src-php-7.3.0 \
  && ./buildconf --force \
  && ./configure \
  --enable-bcmath \
  --enable-mbstring \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.3 \
  && make \
  && make install \
  && make clean
RUN cd /var/local/php-src-php-7.3.0 \
  && ./buildconf --force \
  && ./configure \
  --enable-maintainer-zts \
  --enable-mbstring \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.3-zts \
  && make \
  && make install \
  && make clean

RUN wget -O phpunit https://phar.phpunit.de/phpunit-7.5.0.phar \
  && chmod +x phpunit \
  && cp phpunit /usr/local/php-7.3/bin \
  && mv phpunit /usr/local/php-7.3-zts/bin

# php 7.4
RUN wget https://ftp.gnu.org/gnu/bison/bison-3.0.1.tar.gz -O /var/local/bison-3.0.1.tar.gz
RUN cd /var/local \
  && tar -zxvf bison-3.0.1.tar.gz \
  && cd /var/local/bison-3.0.1 \
  && ./configure \
  && make \
  && make install

RUN wget https://github.com/php/php-src/archive/php-7.4.0.tar.gz -O /var/local/php-7.4.0.tar.gz

RUN cd /var/local \
  && tar -zxvf php-7.4.0.tar.gz

RUN cd /var/local/php-src-php-7.4.0 \
  && ./buildconf --force \
  && ./configure \
  --enable-bcmath \
  --enable-mbstring \
  --disable-mbregex \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.4 \
  && make \
  && make install \
  && make clean
RUN cd /var/local/php-src-php-7.4.0 \
  && ./buildconf --force \
  && ./configure \
  --enable-maintainer-zts \
  --enable-mbstring \
  --disable-mbregex \
  --with-openssl \
  --with-zlib \
  --prefix=/usr/local/php-7.4-zts \
  && make \
  && make install \
  && make clean

RUN wget -O phpunit https://phar.phpunit.de/phpunit-8.phar \
  && chmod +x phpunit \
  && cp phpunit /usr/local/php-7.4/bin \
  && mv phpunit /usr/local/php-7.4-zts/bin

# Install php dependencies
RUN apt-get clean && apt-get update && apt-get install -y --force-yes \
  valgrind \
  && apt-get clean
