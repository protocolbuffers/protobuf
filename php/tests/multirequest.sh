#!/bin/bash
set -e

# Compile c extension
VERSION=7.4
PORT=12345

export PATH=/usr/local/php-$VERSION/bin:$PATH
export C_INCLUDE_PATH=/usr/local/php-$VERSION/include/php/main:/usr/local/php-$VERSION/include/php:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=/usr/local/php-$VERSION/include/php/main:/usr/local/php-$VERSION/include/php:$CPLUS_INCLUDE_PATH
/bin/bash ./compile_extension.sh $VERSION

nohup php -d protobuf.keep_descriptor_pool_after_request=1 -dextension=../ext/google/protobuf/modules/protobuf.so -S localhost:$PORT multirequest.php 2>&1 &

sleep 1

wget http://localhost:$PORT/multirequest.result -O multirequest.result
wget http://localhost:$PORT/multirequest.result -O multirequest.result

pushd ../ext/google/protobuf
phpize --clean
popd

PID=`ps | grep "php" | awk '{print $1}'`
echo $PID

if [[ -z "$PID" ]]
then
  echo "Failed"
  exit 1
else
  kill $PID
  echo "Succeeded"
fi
