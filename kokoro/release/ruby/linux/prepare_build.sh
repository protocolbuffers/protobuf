#!/bin/bash

# Move docker's storage location to scratch disk so we don't run out of space.
echo 'DOCKER_OPTS="${DOCKER_OPTS} --graph=/tmpfs/docker"' | sudo tee --append /etc/default/docker
# Use container registry mirror for pulling docker images (should make downloads faster)
# See https://cloud.google.com/container-registry/docs/using-dockerhub-mirroring
echo 'DOCKER_OPTS="${DOCKER_OPTS} --registry-mirror=https://mirror.gcr.io"' | sudo tee --append /etc/default/docker
sudo service docker restart

# Download Docker images from DockerHub
DOCKERHUB_ORGANIZATION=protobuftesting
DOCKERFILE_DIR=kokoro/linux/dockerfile/release/ruby_rake_compiler
DOCKERFILE_PREFIX=$(basename $DOCKERFILE_DIR)
export RAKE_COMPILER_DOCK_IMAGE=${DOCKERHUB_ORGANIZATION}/${DOCKERFILE_PREFIX}_$(sha1sum $DOCKERFILE_DIR/Dockerfile | cut -f1 -d\ )

# All artifacts come here
mkdir artifacts
export ARTIFACT_DIR=$(pwd)/artifacts
