#!/bin/bash

set -ex

cd $(dirname $0)/../../..
git_root=$(pwd)
cd kokoro/linux/dockerfile

DOCKERHUB_ORGANIZATION=protobuftesting

for DOCKERFILE_DIR in test/* release/*
do
  # Generate image name based on Dockerfile checksum. That works well as long
  # as can count on dockerfiles being written in a way that changing the logical
  # contents of the docker image always changes the SHA (e.g. using "ADD file"
  # cmd in the dockerfile in not ok as contents of the added file will not be
  # reflected in the SHA).
  DOCKER_IMAGE_NAME=$(basename $DOCKERFILE_DIR)_$(sha1sum $DOCKERFILE_DIR/Dockerfile | cut -f1 -d\ )

  echo $DOCKER_IMAGE_NAME
  # skip the image if it already exists in the repo
  curl --silent -f -lSL https://registry.hub.docker.com/v2/repositories/${DOCKERHUB_ORGANIZATION}/${DOCKER_IMAGE_NAME}/tags/latest > /dev/null \
      && continue

  docker build -t ${DOCKERHUB_ORGANIZATION}/${DOCKER_IMAGE_NAME} ${DOCKERFILE_DIR}

  # "docker login" needs to be run in advance
  docker push ${DOCKERHUB_ORGANIZATION}/${DOCKER_IMAGE_NAME}
done
