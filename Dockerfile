# Building Protobuf via Docker
# ----------------------------
#
# Create the build container:
# ```bash
# docker build -t protobuf .
# ```
#
# Run the build container (mounts repo to `/protobuf`, which is the work dir):
# ```bash
# docker run -it --rm --name protobuf -v `pwd`:/protobuf -w /protobuf protobuf
# ```

FROM ubuntu:xenial

RUN apt-get update && \
    apt-get install -yq autoconf automake libtool curl make g++ unzip

CMD ./autogen.sh && \
    ./configure && \
    make && \
    make check && \
    make install && \
    ldconfig
