# docker build -t "google/protobuf:dev" --build-arg PROTOC_VERSION=3.0.2
FROM debian:jessie
ARG PROTOC_VERSION="3.0.2"
RUN \
  apt-get update && apt-get install -y \
  autoconf automake libtool curl g++ git make unzip \
  && rm -rf /var/lib/apt/lists/*
# Start the container with the current directory mounted into is 
COPY . /tmp/protobuf
#RUN cd /tmp && git clone https://github.com/google/protobuf.git && \
  #  cd /tmp/protobuf && git checkout v${PROTOC_VERSION} && \
RUN cd /tmp/protobuf && \
    ./autogen.sh && \
    ./configure && \
    make && make check && make install && ldconfig && make clean
