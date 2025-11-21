FROM debian:trixie

RUN apt-get update \
        && \
    apt-get install --no-install-recommends -y -V \
            build-essential \
            ca-certificates \
            cmake \
            git \
            libabsl-dev \
            make

ADD . ./

RUN cmake . \
        && \
    { make -j$(nproc) || make VERBOSE=1 ; }
