FROM quay.io/pypa/manylinux2014_aarch64

RUN yum install -y libtool
RUN yum install -y gcc-c++
RUN /opt/python/cp36-cp36m/bin/pip install twine

COPY protobuf_optimized_pip.sh /
