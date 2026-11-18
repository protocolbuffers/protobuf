You may need to update these source jars if you ever rename anything in the
associated .proto files. Here are the steps to do so:

 - `git checkout v3.25.0`
 - Apply the existing patches from the `patches/` subdirectory--you may want to
   copy the them to a temporary directory from `main` since they are not
   present at `v3.25.0`:
   `patch -p1 < patches/*.patch`
   These patches contain changes we have already had to make to the source
   jars.
 - Make any additional changes you need to make, and save them as a new patch
   file.
 - Rebuild the jars: `USE_BAZEL_VERSION=6.5.0 bazelisk build
   //java/core:generic_test_protos_java_proto
   //java/core:java_test_protos_java_proto
   //java/core:lite_test_protos_java_proto`
 - Copy the relevant src.jar files out of `bazel-bin/java/core` and
   `bazel-bin/src/google/protobuf`.
 - Submit the updated source jars and don't forget to include your new patch
   file.
