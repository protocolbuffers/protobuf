# We use an older version of dockcross image that has gcc4.9.4 because it was built
# before https://github.com/dockcross/dockcross/pull/449
# Thanks to that, wheel build with this image aren't actually
# compliant with manylinux2014, but only with manylinux_2_24
FROM dockcross/manylinux2014-aarch64:20200929-608e6ac
ENV AUDITWHEEL_PLAT manylinux_2_24_$AUDITWHEEL_ARCH

###################################
# Install Python build requirements
RUN /opt/python/cp35-cp35m/bin/pip install --upgrade cython
RUN /opt/python/cp36-cp36m/bin/pip install --upgrade cython
RUN /opt/python/cp37-cp37m/bin/pip install --upgrade cython
RUN /opt/python/cp38-cp38/bin/pip install --upgrade cython
RUN /opt/python/cp39-cp39/bin/pip install --upgrade cython
# the dockcross docker image sets variables like CC, CXX etc.
# to point to the crosscompilation toolchain, but doesn't set corresponding
# variables for the "strip" and "objcopy" tools.
# see https://github.com/dockcross/dockcross/blob/4349cb4999401cbf22a90f46f5052d29be240e50/manylinux2014-aarch64/Dockerfile.in#L23
ENV STRIP=${CROSS_ROOT}/bin/${CROSS_TRIPLE}-strip \
    OBJCOPY=${CROSS_ROOT}/bin/${CROSS_TRIPLE}-objcopy


#FROM dockcross/manylinux2014-aarch64:20210925-32768e3
# Pin the dockcross image since newer versions of the image break the build
#:20210803-41e5c69
#:20210925-32768e3

#RUN curl -sSL -o bazel.repo \
#    https://copr.fedorainfracloud.org/coprs/vbatts/bazel/repo/epel-7/vbatts-bazel-epel-7.repo \
#    && cp bazel.repo /etc/yum.repos.d/
#RUN yum -y update && \
#    #yum -y install bazel4 clang gcc-c++ llvm-devel zlib-devel zlib-static glibc-devel glibc-static  pkgconfig && \
#    yum -y clean packages

# Upgrade to gcc 4.9
#RUN curl ftp://ftp.mirrorservice.org/sites/sourceware.org/pub/gcc/releases/gcc-4.9.2/gcc-4.9.2.tar.bz2 -O \
#    && tar xvfj gcc-4.9.2.tar.bz2 \
#    && cd gcc-4.9.2 \
#    && ./configure --disable-multilib --enable-languages=c,c++ --host=aarch64 \
#    && make -j 4 \
#    && make install
