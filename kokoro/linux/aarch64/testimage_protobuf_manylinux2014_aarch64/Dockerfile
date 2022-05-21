# Pin the dockcross image since newer versions of the image break the build
FROM dockcross/manylinux2014-aarch64:20210803-41e5c69

RUN curl -sSL -o bazel.repo \
    https://copr.fedorainfracloud.org/coprs/vbatts/bazel/repo/epel-7/vbatts-bazel-epel-7.repo \
    && cp bazel.repo /etc/yum.repos.d/
RUN yum -y update && \
    yum -y install bazel4 clang gcc-c++ llvm-devel zlib-devel zlib-static glibc-devel glibc-static  pkgconfig && \
    yum -y clean packages
