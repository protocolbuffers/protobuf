FROM debian:stretch-slim

# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=863199
RUN mkdir -p /usr/share/man/man1

RUN apt-get update && apt-get install -y build-essential autoconf automake libtool wget openjdk-8-jdk-headless maven \
    gcc-multilib g++-multilib g++-aarch64-linux-gnu g++-powerpc64le-linux-gnu
