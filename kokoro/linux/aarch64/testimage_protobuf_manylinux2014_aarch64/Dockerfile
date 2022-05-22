FROM dockcross/manylinux2014-aarch64:20210925-32768e3
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
