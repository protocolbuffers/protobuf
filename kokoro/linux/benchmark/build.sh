#!/bin/bash
#
# Change to repo root
cd $(dirname $0)/../../..

set -ex

# Install openJDK 11 (required by the java benchmarks)
sudo apt-key adv --recv-keys --keyserver keyserver.ubuntu.com 78BD65473CB3BD13
sudo add-apt-repository ppa:openjdk-r/ppa
sudo apt-get update
sudo apt-get install -y openjdk-11-jdk-headless

# use java 11
sudo update-java-alternatives --set /usr/lib/jvm/java-1.11.0-openjdk-amd64
java -version

./tests.sh benchmark
