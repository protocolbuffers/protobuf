pushd ../src
../vsprojects/debug/protoc --csharp_out=../csharp/ProtocolBuffers/DescriptorProtos google/protobuf/descriptor.proto
../vsprojects/debug/protoc --csharp_out=../csharp/ProtocolBuffers.Test/TestProtos google/protobuf/unittest.proto
../vsprojects/debug/protoc --csharp_out=../csharp/ProtocolBuffers.Test/TestProtos google/protobuf/unittest_embed_optimize_for.proto
../vsprojects/debug/protoc --csharp_out=../csharp/ProtocolBuffers.Test/TestProtos google/protobuf/unittest_import.proto
../vsprojects/debug/protoc --csharp_out=../csharp/ProtocolBuffers.Test/TestProtos google/protobuf/unittest_mset.proto
../vsprojects/debug/protoc --csharp_out=../csharp/ProtocolBuffers.Test/TestProtos google/protobuf/unittest_optimize_for.proto
../vsprojects/debug/protoc -I ../java:. --csharp_out=../csharp/ProtocolBuffers.Test/TestProtos ../java/src/test/java/com/google/protobuf/multiple_files_test.proto
popd
