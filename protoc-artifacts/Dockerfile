FROM debian:stretch-slim

# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=863199
RUN mkdir -p /usr/share/man/man1

RUN apt-get update && \
    apt-get install -y build-essential autoconf automake libtool wget \
        openjdk-8-jdk-headless maven gcc-6-multilib g++-6-multilib && \
    # APT doesn't like to install packages from different architectures in the same command
    apt-get install -y g++-aarch64-linux-gnu && \
    apt-get install -y g++-powerpc64le-linux-gnu
