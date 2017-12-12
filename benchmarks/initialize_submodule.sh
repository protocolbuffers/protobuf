#! /bin/sh

oldpwd=`pwd`
cd "../third_party"
git submodule update --init -r
cd benchmark && cmake -DCMAKE_BUILD_TYPE=Release && make && cd ..
cd "$oldpwd"
