#!/bin/sh

set -xe

pushd /tmp

curl -O https://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz
gunzip autoconf-2.69.tar.gz
tar xvf autoconf-2.69.tar

pushd autoconf-2.69

./configure
make
make install

popd

curl -O https://ftp.gnu.org/gnu/automake/automake-1.15.tar.gz
tar xvzf automake-1.15.tar.gz

pushd automake-1.15

./configure
make
make install

popd

curl -O https://ftp.gnu.org/gnu/libtool/libtool-2.4.6.tar.gz
tar xzvf libtool-2.4.6.tar.gz

pushd libtool-2.4.6

./configure
make
make install

popd

popd

export SED=$(which sed)

./autogen.sh
./configure --disable-shared "CFLAGS=-fPIC" "CXXFLAGS=-fPIC"
make
make check

cd python
PYTHON_VERSIONS="cp27-cp27m cp34-cp34m cp35-cp35m cp36-cp36m"
for PYVER in $PYTHON_VERSIONS
do
	/opt/python/$PYVER/bin/python setup.py bdist_wheel --cpp_implementation --compile_static_extension
done
