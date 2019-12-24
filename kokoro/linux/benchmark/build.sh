#!/bin/bash

cd $(dirname $0)/../../..

# prepare php environments
sudo apt-get update && sudo apt-get install -y --force-yes php5
sudo ln -sf /usr/include/x86_64-linux-gnu/gmp.h /usr/include/gmp.h
mkdir php_temp
cd php_temp
curl -sS https://getcomposer.org/installer | php
sudo mv composer.phar /usr/local/bin/composer
git clone https://github.com/php/php-src
cd php-src && git checkout PHP-7.2.13 && ./buildconf --force
./configure \
	--enable-bcmatch \
	--with-gmp --with-openssl \
	--with-zlib  \
	--prefix=/usr/local/php-7.2 && \
make -j8 && sudo make install && make clean
wget -O phpunit https://phar.phpunit.de/phpunit-7.phar && \
	chmod +x phpunit && \
	sudo cp phpunit /usr/local/php-7.2/bin
sudo apt-get install -y --force-yes valgrind
cd ../..

./tests.sh benchmark
