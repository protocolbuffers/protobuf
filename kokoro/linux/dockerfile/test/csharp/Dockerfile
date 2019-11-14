FROM debian:stretch

# Install dependencies.  We start with the basic ones require to build protoc
# and the C++ build
RUN apt-get update && apt-get install -y \
  autoconf \
  autotools-dev \
  build-essential \
  bzip2 \
  ccache \
  curl \
  gcc \
  git \
  libc6 \
  libc6-dbg \
  libc6-dev \
  libgtest-dev \
  libtool \
  make \
  parallel \
  time \
  wget \
  && apt-get clean

# dotnet SDK prerequisites
RUN apt-get update && apt-get install -y libunwind8 libicu57 && apt-get clean

# Install dotnet SDK via install script
RUN wget -q https://dot.net/v1/dotnet-install.sh && \
    chmod u+x dotnet-install.sh && \
    ./dotnet-install.sh --version 2.1.504 && \
    ln -s /root/.dotnet/dotnet /usr/local/bin

RUN wget -q www.nuget.org/NuGet.exe -O /usr/local/bin/nuget.exe

ENV DOTNET_SKIP_FIRST_TIME_EXPERIENCE true
