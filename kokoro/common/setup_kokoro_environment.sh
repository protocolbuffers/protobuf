#!/bin/bash

set -eux

# Upgrade to a supported gcc version
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get -y update && \
  sudo apt-get install --no-install-recommends  -y --fix-missing --option Acquire::Retries=10 --option Acquire::http::Timeout="1800" \
    gcc-7 g++-7
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 100 --slave /usr/bin/g++ g++ /usr/bin/g++-7
sudo update-alternatives --set gcc /usr/bin/gcc-7
