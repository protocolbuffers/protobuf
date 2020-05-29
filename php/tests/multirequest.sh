#!/bin/bash

cd $(dirname $0)

set -e

PORT=12345

./compile_extension.sh

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
