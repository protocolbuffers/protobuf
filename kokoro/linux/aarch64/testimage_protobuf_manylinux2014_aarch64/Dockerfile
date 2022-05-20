# Pin the dockcross image since newer versions of the image break the build
FROM dockcross/manylinux2014-aarch64:20210803-41e5c69

RUN apt-get update && apt-get install -y bazel && apt-get clean
