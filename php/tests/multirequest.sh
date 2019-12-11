#!/bin/bash
set -e

nohup php  -d protobuf.keep_descriptor_pool_after_request=1 -dextension=../ext/google/protobuf/modules/protobuf.so -S localhost:8001 multirequest.php 2>&1 &

wget http://localhost:8001/multirequest.result -O multirequest.result
wget http://localhost:8001/multirequest.result -O multirequest.result

PID=`ps | grep "php" | awk '{print $1}'`
echo $PID

if [[ -z "$PID" ]]
then
  echo "Failed"
  exit 1
else
  kill $PID
fi
