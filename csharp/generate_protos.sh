#!/bin/bash
# Generates C# source files from .proto files.
# You first need to make sure protoc has been built (see instructions on
# building protoc in root of this repository)

set -e

# cd to repository root
pushd $(dirname $0)/..

# Protocol buffer compiler to use. If the PROTOC variable is set,
# use that. Otherwise, probe for expected locations under both
# Windows and Unix.
PROTOC_LOCATIONS=(
  "bazel-bin/protoc"
  "solution/Debug/protoc.exe"
  "cmake/build/Debug/protoc.exe"
  "cmake/build/Release/protoc.exe"
)
if [ -z "$PROTOC" ]; then
  for protoc in "${PROTOC_LOCATIONS[@]}"; do
    if [ -x "$protoc" ]; then
      PROTOC="$protoc"
    fi
  done
  if [ -z "$PROTOC" ]; then
    echo "Unable to find protocol buffer compiler."
    exit 1
  fi
fi

# descriptor.proto and well-known types
$PROTOC -Isrc --csharp_out=csharp/src/Google.Protobuf \
    --csharp_opt=base_namespace=Google.Protobuf \
    --csharp_opt=file_extension=.pb.cs \
    src/google/protobuf/descriptor.proto \
    src/google/protobuf/any.proto \
    src/google/protobuf/api.proto \
    src/google/protobuf/duration.proto \
    src/google/protobuf/empty.proto \
    src/google/protobuf/field_mask.proto \
    src/google/protobuf/source_context.proto \
    src/google/protobuf/struct.proto \
    src/google/protobuf/timestamp.proto \
    src/google/protobuf/type.proto \
    src/google/protobuf/wrappers.proto \
    src/google/protobuf/compiler/plugin.proto

# Test protos
# Note that this deliberately does *not* include old_extensions1.proto
# and old_extensions2.proto, which are generated with an older version
# of protoc.
$PROTOC -Isrc -I. \
    --experimental_allow_proto3_optional \
    --experimental_editions \
    --csharp_out=csharp/src/Google.Protobuf.Test.TestProtos \
    --csharp_opt=file_extension=.pb.cs \
    --descriptor_set_out=csharp/src/Google.Protobuf.Test/testprotos.pb \
    --include_source_info \
    --include_imports \
    conformance/test_protos/test_messages_edition2023.proto \
    csharp/protos/map_unittest_proto3.proto \
    csharp/protos/unittest_issues.proto \
    csharp/protos/unittest_custom_options_proto3.proto \
    csharp/protos/unittest_proto3.proto \
    csharp/protos/unittest_import_proto3.proto \
    csharp/protos/unittest_import_public_proto3.proto \
    csharp/protos/unittest.proto \
    csharp/protos/unittest_import.proto \
    csharp/protos/unittest_import_public.proto \
    csharp/protos/unittest_issue6936_a.proto \
    csharp/protos/unittest_issue6936_b.proto \
    csharp/protos/unittest_issue6936_c.proto \
    csharp/protos/unittest_selfreferential_options.proto \
    editions/golden/test_messages_proto3_editions.proto \
    editions/golden/test_messages_proto2_editions.proto \
    src/google/protobuf/unittest_well_known_types.proto \
    src/google/protobuf/test_messages_proto3.proto \
    src/google/protobuf/test_messages_proto2.proto \
    src/google/protobuf/unittest_features.proto \
    src/google/protobuf/unittest_legacy_features.proto \
    src/google/protobuf/unittest_proto3_optional.proto \
    src/google/protobuf/unittest_retention.proto

# We can safely ignore the unused import warning as the
# purpose of the test is to work with the dependencies
$PROTOC -Isrc -I. -Icsharp/protos \
    --experimental_allow_proto3_optional \
    --experimental_editions \
    --csharp_out=csharp/src/Google.Protobuf.Test.TestProtos/UnittestDeepDependencies \
    --csharp_opt=file_extension=.pb.cs \
    csharp/protos/unittest_deep_dependencies/file1.proto \
    csharp/protos/unittest_deep_dependencies/file2.proto \
    csharp/protos/unittest_deep_dependencies/file3.proto \
    csharp/protos/unittest_deep_dependencies/file4.proto \
    csharp/protos/unittest_deep_dependencies/file5.proto \
    csharp/protos/unittest_deep_dependencies/file6.proto \
    csharp/protos/unittest_deep_dependencies/file7.proto \
    csharp/protos/unittest_deep_dependencies/file8.proto \
    csharp/protos/unittest_deep_dependencies/file9.proto \
    csharp/protos/unittest_deep_dependencies/file10.proto \
    csharp/protos/unittest_deep_dependencies/file11.proto \
    csharp/protos/unittest_deep_dependencies/file12.proto \
    csharp/protos/unittest_deep_dependencies/file13.proto \
    csharp/protos/unittest_deep_dependencies/file14.proto \
    csharp/protos/unittest_deep_dependencies/file15.proto \
    csharp/protos/unittest_deep_dependencies/file16.proto \
    csharp/protos/unittest_deep_dependencies/file17.proto \
    csharp/protos/unittest_deep_dependencies/file18.proto \
    csharp/protos/unittest_deep_dependencies/file19.proto \
    csharp/protos/unittest_deep_dependencies/file20.proto \
    csharp/protos/unittest_deep_dependencies/file21.proto \
    csharp/protos/unittest_deep_dependencies/file22.proto \
    csharp/protos/unittest_deep_dependencies/file23.proto \
    csharp/protos/unittest_deep_dependencies/file24.proto \
    csharp/protos/unittest_deep_dependencies/file25.proto \
    csharp/protos/unittest_deep_dependencies/file26.proto \
    csharp/protos/unittest_deep_dependencies/file27.proto \
    csharp/protos/unittest_deep_dependencies/file28.proto \
    csharp/protos/unittest_deep_dependencies/file29.proto \
    csharp/protos/unittest_deep_dependencies/file30.proto \
    csharp/protos/unittest_deep_dependencies/file31.proto \
    csharp/protos/unittest_deep_dependencies/file32.proto \
    csharp/protos/unittest_deep_dependencies/file33.proto \
    csharp/protos/unittest_deep_dependencies/file34.proto \
    csharp/protos/unittest_deep_dependencies/file35.proto \
    csharp/protos/unittest_deep_dependencies/file36.proto \
    csharp/protos/unittest_deep_dependencies/file37.proto \
    csharp/protos/unittest_deep_dependencies/file38.proto \
    csharp/protos/unittest_deep_dependencies/file39.proto \
    csharp/protos/unittest_deep_dependencies/file40.proto \
    csharp/protos/unittest_deep_dependencies/file41.proto \
    csharp/protos/unittest_deep_dependencies/file42.proto \
    csharp/protos/unittest_deep_dependencies/file43.proto \
    csharp/protos/unittest_deep_dependencies/file44.proto \
    csharp/protos/unittest_deep_dependencies/file45.proto \
    csharp/protos/unittest_deep_dependencies/file46.proto \
    csharp/protos/unittest_deep_dependencies/file47.proto \
    csharp/protos/unittest_deep_dependencies/file48.proto \
    csharp/protos/unittest_deep_dependencies/file49.proto \
    csharp/protos/unittest_deep_dependencies/file50.proto \
    csharp/protos/unittest_deep_dependencies/file51.proto \
    csharp/protos/unittest_deep_dependencies/file52.proto \
    csharp/protos/unittest_deep_dependencies/file53.proto \
    csharp/protos/unittest_deep_dependencies/file54.proto \
    csharp/protos/unittest_deep_dependencies/file55.proto \
    csharp/protos/unittest_deep_dependencies/file56.proto \
    csharp/protos/unittest_deep_dependencies/file57.proto \
    csharp/protos/unittest_deep_dependencies/file58.proto \
    csharp/protos/unittest_deep_dependencies/file59.proto \
    csharp/protos/unittest_deep_dependencies/file60.proto \
    csharp/protos/unittest_deep_dependencies/file61.proto \
    csharp/protos/unittest_deep_dependencies/file62.proto \
    csharp/protos/unittest_deep_dependencies/file63.proto \
    csharp/protos/unittest_deep_dependencies/file64.proto \
    csharp/protos/unittest_deep_dependencies/file65.proto \
    csharp/protos/unittest_deep_dependencies/file66.proto \
    csharp/protos/unittest_deep_dependencies/file67.proto \
    csharp/protos/unittest_deep_dependencies/file68.proto \
    csharp/protos/unittest_deep_dependencies/file69.proto \
    csharp/protos/unittest_deep_dependencies/file70.proto \
    csharp/protos/unittest_deep_dependencies/file71.proto \
    csharp/protos/unittest_deep_dependencies/file72.proto \
    csharp/protos/unittest_deep_dependencies/file73.proto \
    csharp/protos/unittest_deep_dependencies/file74.proto \
    csharp/protos/unittest_deep_dependencies/file75.proto \
    csharp/protos/unittest_deep_dependencies/file76.proto \
    csharp/protos/unittest_deep_dependencies/file77.proto \
    csharp/protos/unittest_deep_dependencies/file78.proto \
    csharp/protos/unittest_deep_dependencies/file79.proto \
    csharp/protos/unittest_deep_dependencies/file80.proto \
    csharp/protos/unittest_deep_dependencies/file81.proto \
    csharp/protos/unittest_deep_dependencies/file82.proto \
    csharp/protos/unittest_deep_dependencies/file83.proto \
    csharp/protos/unittest_deep_dependencies/file84.proto \
    csharp/protos/unittest_deep_dependencies/file85.proto \
    csharp/protos/unittest_deep_dependencies/file86.proto \
    csharp/protos/unittest_deep_dependencies/file87.proto \
    csharp/protos/unittest_deep_dependencies/file88.proto \
    csharp/protos/unittest_deep_dependencies/file89.proto \
    csharp/protos/unittest_deep_dependencies/file90.proto \
    csharp/protos/unittest_deep_dependencies/file91.proto \
    csharp/protos/unittest_deep_dependencies/file92.proto \
    csharp/protos/unittest_deep_dependencies/file93.proto \
    csharp/protos/unittest_deep_dependencies/file94.proto \
    csharp/protos/unittest_deep_dependencies/file95.proto \
    csharp/protos/unittest_deep_dependencies/file96.proto \
    csharp/protos/unittest_deep_dependencies/file97.proto \
    csharp/protos/unittest_deep_dependencies/file98.proto \
    csharp/protos/unittest_deep_dependencies/file99.proto \
    csharp/protos/unittest_deep_dependencies/file100.proto \
    csharp/protos/unittest_deep_dependencies/file101.proto \
    csharp/protos/unittest_deep_dependencies/file102.proto \
    csharp/protos/unittest_deep_dependencies/file103.proto \
    csharp/protos/unittest_deep_dependencies/file104.proto \
    csharp/protos/unittest_deep_dependencies/file105.proto \
    csharp/protos/unittest_deep_dependencies/file106.proto \
    csharp/protos/unittest_deep_dependencies/file107.proto \
    csharp/protos/unittest_deep_dependencies/file108.proto \
    csharp/protos/unittest_deep_dependencies/file109.proto \
    csharp/protos/unittest_deep_dependencies/file110.proto \
    csharp/protos/unittest_deep_dependencies/file111.proto \
    csharp/protos/unittest_deep_dependencies/file112.proto \
    csharp/protos/unittest_deep_dependencies/file113.proto \
    csharp/protos/unittest_deep_dependencies/file114.proto \
    csharp/protos/unittest_deep_dependencies/file115.proto \
    csharp/protos/unittest_deep_dependencies/file116.proto \
    csharp/protos/unittest_deep_dependencies/file117.proto \
    csharp/protos/unittest_deep_dependencies/file118.proto \
    csharp/protos/unittest_deep_dependencies/file119.proto \
    csharp/protos/unittest_deep_dependencies/file120.proto \
    csharp/protos/unittest_deep_dependencies/file121.proto \
    csharp/protos/unittest_deep_dependencies/file122.proto \
    csharp/protos/unittest_deep_dependencies/file123.proto \
    csharp/protos/unittest_deep_dependencies/file124.proto \
    csharp/protos/unittest_deep_dependencies/file125.proto \
    csharp/protos/unittest_deep_dependencies/file126.proto \
    csharp/protos/unittest_deep_dependencies/file127.proto \
    csharp/protos/unittest_deep_dependencies/file128.proto \
    csharp/protos/unittest_deep_dependencies/file129.proto \
    csharp/protos/unittest_deep_dependencies/file130.proto \
    csharp/protos/unittest_deep_dependencies/file131.proto \
    csharp/protos/unittest_deep_dependencies/file132.proto \
    csharp/protos/unittest_deep_dependencies/file133.proto \
    csharp/protos/unittest_deep_dependencies/file134.proto \
    csharp/protos/unittest_deep_dependencies/file135.proto \
    csharp/protos/unittest_deep_dependencies/file136.proto \
    csharp/protos/unittest_deep_dependencies/file137.proto \
    csharp/protos/unittest_deep_dependencies/file138.proto \
    csharp/protos/unittest_deep_dependencies/file139.proto \
    csharp/protos/unittest_deep_dependencies/file140.proto \
    csharp/protos/unittest_deep_dependencies/file141.proto \
    csharp/protos/unittest_deep_dependencies/file142.proto \
    csharp/protos/unittest_deep_dependencies/file143.proto \
    csharp/protos/unittest_deep_dependencies/file144.proto \
    csharp/protos/unittest_deep_dependencies/file145.proto \
    csharp/protos/unittest_deep_dependencies/file146.proto \
    csharp/protos/unittest_deep_dependencies/file147.proto \
    csharp/protos/unittest_deep_dependencies/file148.proto \
    csharp/protos/unittest_deep_dependencies/file149.proto \
    csharp/protos/unittest_deep_dependencies/file150.proto \
    csharp/protos/unittest_deep_dependencies/file151.proto \
    csharp/protos/unittest_deep_dependencies/file152.proto \
    csharp/protos/unittest_deep_dependencies/file153.proto \
    csharp/protos/unittest_deep_dependencies/file154.proto \
    csharp/protos/unittest_deep_dependencies/file155.proto \
    csharp/protos/unittest_deep_dependencies/file156.proto \
    csharp/protos/unittest_deep_dependencies/file157.proto \
    csharp/protos/unittest_deep_dependencies/file158.proto \
    csharp/protos/unittest_deep_dependencies/file159.proto \
    csharp/protos/unittest_deep_dependencies/file160.proto \
    csharp/protos/unittest_deep_dependencies/file161.proto \
    csharp/protos/unittest_deep_dependencies/file162.proto \
    csharp/protos/unittest_deep_dependencies/file163.proto \
    csharp/protos/unittest_deep_dependencies/file164.proto \
    csharp/protos/unittest_deep_dependencies/file165.proto \
    csharp/protos/unittest_deep_dependencies/file166.proto \
    csharp/protos/unittest_deep_dependencies/file167.proto \
    csharp/protos/unittest_deep_dependencies/file168.proto \
    csharp/protos/unittest_deep_dependencies/file169.proto \
    csharp/protos/unittest_deep_dependencies/file170.proto \
    csharp/protos/unittest_deep_dependencies/file171.proto \
    csharp/protos/unittest_deep_dependencies/file172.proto \
    csharp/protos/unittest_deep_dependencies/file173.proto \
    csharp/protos/unittest_deep_dependencies/file174.proto \
    csharp/protos/unittest_deep_dependencies/file175.proto \
    csharp/protos/unittest_deep_dependencies/file176.proto \
    csharp/protos/unittest_deep_dependencies/file177.proto \
    csharp/protos/unittest_deep_dependencies/file178.proto \
    csharp/protos/unittest_deep_dependencies/file179.proto \
    csharp/protos/unittest_deep_dependencies/file180.proto \
    csharp/protos/unittest_deep_dependencies/file181.proto \
    csharp/protos/unittest_deep_dependencies/file182.proto \
    csharp/protos/unittest_deep_dependencies/file183.proto \
    csharp/protos/unittest_deep_dependencies/file184.proto \
    csharp/protos/unittest_deep_dependencies/file185.proto \
    csharp/protos/unittest_deep_dependencies/file186.proto \
    csharp/protos/unittest_deep_dependencies/file187.proto \
    csharp/protos/unittest_deep_dependencies/file188.proto \
    csharp/protos/unittest_deep_dependencies/file189.proto \
    csharp/protos/unittest_deep_dependencies/file190.proto \
    csharp/protos/unittest_deep_dependencies/file191.proto \
    csharp/protos/unittest_deep_dependencies/file192.proto \
    csharp/protos/unittest_deep_dependencies/file193.proto \
    csharp/protos/unittest_deep_dependencies/file194.proto \
    csharp/protos/unittest_deep_dependencies/file195.proto \
    csharp/protos/unittest_deep_dependencies/file196.proto \
    csharp/protos/unittest_deep_dependencies/file197.proto \
    csharp/protos/unittest_deep_dependencies/file198.proto \
    csharp/protos/unittest_deep_dependencies/file199.proto \
    csharp/protos/unittest_deep_dependencies/file200.proto \
    csharp/protos/unittest_deep_dependencies/file201.proto \
    csharp/protos/unittest_deep_dependencies/file202.proto \
    csharp/protos/unittest_deep_dependencies/file203.proto \
    csharp/protos/unittest_deep_dependencies/file204.proto \
    csharp/protos/unittest_deep_dependencies/file205.proto \
    csharp/protos/unittest_deep_dependencies/file206.proto \
    csharp/protos/unittest_deep_dependencies/file207.proto \
    csharp/protos/unittest_deep_dependencies/file208.proto \
    csharp/protos/unittest_deep_dependencies/file209.proto \
    csharp/protos/unittest_deep_dependencies/file210.proto \
    csharp/protos/unittest_deep_dependencies/file211.proto \
    csharp/protos/unittest_deep_dependencies/file212.proto \
    csharp/protos/unittest_deep_dependencies/file213.proto \
    csharp/protos/unittest_deep_dependencies/file214.proto \
    csharp/protos/unittest_deep_dependencies/file215.proto \
    csharp/protos/unittest_deep_dependencies/file216.proto \
    csharp/protos/unittest_deep_dependencies/file217.proto \
    csharp/protos/unittest_deep_dependencies/file218.proto \
    csharp/protos/unittest_deep_dependencies/file219.proto \
    csharp/protos/unittest_deep_dependencies/file220.proto \
    csharp/protos/unittest_deep_dependencies/file221.proto \
    csharp/protos/unittest_deep_dependencies/file222.proto \
    csharp/protos/unittest_deep_dependencies/file223.proto \
    csharp/protos/unittest_deep_dependencies/file224.proto \
    csharp/protos/unittest_deep_dependencies/file225.proto \
    csharp/protos/unittest_deep_dependencies/file226.proto \
    csharp/protos/unittest_deep_dependencies/file227.proto \
    csharp/protos/unittest_deep_dependencies/file228.proto \
    csharp/protos/unittest_deep_dependencies/file229.proto \
    csharp/protos/unittest_deep_dependencies/file230.proto \
    csharp/protos/unittest_deep_dependencies/file231.proto \
    csharp/protos/unittest_deep_dependencies/file232.proto \
    csharp/protos/unittest_deep_dependencies/file233.proto \
    csharp/protos/unittest_deep_dependencies/file234.proto \
    csharp/protos/unittest_deep_dependencies/file235.proto \
    csharp/protos/unittest_deep_dependencies/file236.proto \
    csharp/protos/unittest_deep_dependencies/file237.proto \
    csharp/protos/unittest_deep_dependencies/file238.proto \
    csharp/protos/unittest_deep_dependencies/file239.proto \
    csharp/protos/unittest_deep_dependencies/file240.proto \
    csharp/protos/unittest_deep_dependencies/file241.proto \
    csharp/protos/unittest_deep_dependencies/file242.proto \
    csharp/protos/unittest_deep_dependencies/file243.proto \
    csharp/protos/unittest_deep_dependencies/file244.proto \
    csharp/protos/unittest_deep_dependencies/file245.proto \
    csharp/protos/unittest_deep_dependencies/file246.proto \
    csharp/protos/unittest_deep_dependencies/file247.proto \
    csharp/protos/unittest_deep_dependencies/file248.proto \
    csharp/protos/unittest_deep_dependencies/file249.proto \
    csharp/protos/unittest_deep_dependencies/file250.proto \
    csharp/protos/unittest_deep_dependencies/file251.proto \
    csharp/protos/unittest_deep_dependencies/file252.proto \
    csharp/protos/unittest_deep_dependencies/file253.proto \
    csharp/protos/unittest_deep_dependencies/file254.proto \
    csharp/protos/unittest_deep_dependencies/file255.proto \
    csharp/protos/unittest_deep_dependencies/file256.proto \
    csharp/protos/unittest_deep_dependencies/file257.proto \
    csharp/protos/unittest_deep_dependencies/file258.proto \
    csharp/protos/unittest_deep_dependencies/file259.proto \
    csharp/protos/unittest_deep_dependencies/file260.proto \
    csharp/protos/unittest_deep_dependencies/file261.proto \
    csharp/protos/unittest_deep_dependencies/file262.proto \
    csharp/protos/unittest_deep_dependencies/file263.proto \
    csharp/protos/unittest_deep_dependencies/file264.proto \
    csharp/protos/unittest_deep_dependencies/file265.proto \
    csharp/protos/unittest_deep_dependencies/file266.proto \
    csharp/protos/unittest_deep_dependencies/file267.proto \
    csharp/protos/unittest_deep_dependencies/file268.proto \
    csharp/protos/unittest_deep_dependencies/file269.proto \
    csharp/protos/unittest_deep_dependencies/file270.proto \
    csharp/protos/unittest_deep_dependencies/file271.proto \
    csharp/protos/unittest_deep_dependencies/file272.proto \
    csharp/protos/unittest_deep_dependencies/file273.proto \
    csharp/protos/unittest_deep_dependencies/file274.proto \
    csharp/protos/unittest_deep_dependencies/file275.proto \
    csharp/protos/unittest_deep_dependencies/file276.proto \
    csharp/protos/unittest_deep_dependencies/file277.proto \
    csharp/protos/unittest_deep_dependencies/file278.proto \
    csharp/protos/unittest_deep_dependencies/file279.proto \
    csharp/protos/unittest_deep_dependencies/file280.proto \
    csharp/protos/unittest_deep_dependencies/file281.proto \
    csharp/protos/unittest_deep_dependencies/file282.proto \
    csharp/protos/unittest_deep_dependencies/file283.proto \
    csharp/protos/unittest_deep_dependencies/file284.proto \
    csharp/protos/unittest_deep_dependencies/file285.proto \
    csharp/protos/unittest_deep_dependencies/file286.proto \
    csharp/protos/unittest_deep_dependencies/file287.proto \
    csharp/protos/unittest_deep_dependencies/file288.proto \
    csharp/protos/unittest_deep_dependencies/file289.proto \
    csharp/protos/unittest_deep_dependencies/file290.proto \
    csharp/protos/unittest_deep_dependencies/file291.proto \
    csharp/protos/unittest_deep_dependencies/file292.proto \
    csharp/protos/unittest_deep_dependencies/file293.proto \
    csharp/protos/unittest_deep_dependencies/file294.proto \
    csharp/protos/unittest_deep_dependencies/file295.proto \
    csharp/protos/unittest_deep_dependencies/file296.proto \
    csharp/protos/unittest_deep_dependencies/file297.proto \
    csharp/protos/unittest_deep_dependencies/file298.proto \
    csharp/protos/unittest_deep_dependencies/file299.proto \
    csharp/protos/unittest_deep_dependencies/file300.proto \
    csharp/protos/unittest_deep_dependencies/file301.proto \
    csharp/protos/unittest_deep_dependencies/file302.proto \
    csharp/protos/unittest_deep_dependencies/file303.proto \
    csharp/protos/unittest_deep_dependencies/file304.proto \
    csharp/protos/unittest_deep_dependencies/file305.proto \
    csharp/protos/unittest_deep_dependencies/file306.proto \
    csharp/protos/unittest_deep_dependencies/file307.proto \
    csharp/protos/unittest_deep_dependencies/file308.proto \
    csharp/protos/unittest_deep_dependencies/file309.proto \
    csharp/protos/unittest_deep_dependencies/file310.proto \
    csharp/protos/unittest_deep_dependencies/file311.proto \
    csharp/protos/unittest_deep_dependencies/file312.proto \
    csharp/protos/unittest_deep_dependencies/file313.proto \
    csharp/protos/unittest_deep_dependencies/file314.proto \
    csharp/protos/unittest_deep_dependencies/file315.proto \
    csharp/protos/unittest_deep_dependencies/file316.proto \
    csharp/protos/unittest_deep_dependencies/file317.proto \
    csharp/protos/unittest_deep_dependencies/file318.proto \
    csharp/protos/unittest_deep_dependencies/file319.proto \
    csharp/protos/unittest_deep_dependencies/file320.proto \
    csharp/protos/unittest_deep_dependencies/file321.proto \
    csharp/protos/unittest_deep_dependencies/file322.proto \
    csharp/protos/unittest_deep_dependencies/file323.proto \
    csharp/protos/unittest_deep_dependencies/file324.proto \
    csharp/protos/unittest_deep_dependencies/file325.proto \
    csharp/protos/unittest_deep_dependencies/file326.proto \
    csharp/protos/unittest_deep_dependencies/file327.proto \
    csharp/protos/unittest_deep_dependencies/file328.proto \
    csharp/protos/unittest_deep_dependencies/file329.proto \
    csharp/protos/unittest_deep_dependencies/file330.proto \
    csharp/protos/unittest_deep_dependencies/file331.proto \
    csharp/protos/unittest_deep_dependencies/file332.proto \
    csharp/protos/unittest_deep_dependencies/file333.proto \
    csharp/protos/unittest_deep_dependencies/file334.proto \
    csharp/protos/unittest_deep_dependencies/file335.proto \
    csharp/protos/unittest_deep_dependencies/file336.proto \
    csharp/protos/unittest_deep_dependencies/file337.proto \
    csharp/protos/unittest_deep_dependencies/file338.proto \
    csharp/protos/unittest_deep_dependencies/file339.proto \
    csharp/protos/unittest_deep_dependencies/file340.proto \
    csharp/protos/unittest_deep_dependencies/file341.proto \
    csharp/protos/unittest_deep_dependencies/file342.proto \
    csharp/protos/unittest_deep_dependencies/file343.proto \
    csharp/protos/unittest_deep_dependencies/file344.proto \
    csharp/protos/unittest_deep_dependencies/file345.proto \
    csharp/protos/unittest_deep_dependencies/file346.proto \
    csharp/protos/unittest_deep_dependencies/file347.proto \
    csharp/protos/unittest_deep_dependencies/file348.proto \
    csharp/protos/unittest_deep_dependencies/file349.proto \
    csharp/protos/unittest_deep_dependencies/file350.proto \
    csharp/protos/unittest_deep_dependencies/file351.proto \
    csharp/protos/unittest_deep_dependencies/file352.proto \
    csharp/protos/unittest_deep_dependencies/file353.proto \
    csharp/protos/unittest_deep_dependencies/file354.proto \
    csharp/protos/unittest_deep_dependencies/file355.proto \
    csharp/protos/unittest_deep_dependencies/file356.proto \
    csharp/protos/unittest_deep_dependencies/file357.proto \
    csharp/protos/unittest_deep_dependencies/file358.proto \
    csharp/protos/unittest_deep_dependencies/file359.proto \
    csharp/protos/unittest_deep_dependencies/file360.proto \
    csharp/protos/unittest_deep_dependencies/file361.proto \
    csharp/protos/unittest_deep_dependencies/file362.proto \
    csharp/protos/unittest_deep_dependencies/file363.proto \
    csharp/protos/unittest_deep_dependencies/file364.proto \
    csharp/protos/unittest_deep_dependencies/file365.proto \
    csharp/protos/unittest_deep_dependencies/file366.proto \
    csharp/protos/unittest_deep_dependencies/file367.proto \
    csharp/protos/unittest_deep_dependencies/file368.proto \
    csharp/protos/unittest_deep_dependencies/file369.proto \
    csharp/protos/unittest_deep_dependencies/file370.proto \
    csharp/protos/unittest_deep_dependencies/file371.proto \
    csharp/protos/unittest_deep_dependencies/file372.proto \
    csharp/protos/unittest_deep_dependencies/file373.proto \
    csharp/protos/unittest_deep_dependencies/file374.proto \
    csharp/protos/unittest_deep_dependencies/file375.proto \
    csharp/protos/unittest_deep_dependencies/file376.proto \
    csharp/protos/unittest_deep_dependencies/file377.proto \
    csharp/protos/unittest_deep_dependencies/file378.proto \
    csharp/protos/unittest_deep_dependencies/file379.proto \
    csharp/protos/unittest_deep_dependencies/file380.proto \
    csharp/protos/unittest_deep_dependencies/file381.proto \
    csharp/protos/unittest_deep_dependencies/file382.proto \
    csharp/protos/unittest_deep_dependencies/file383.proto \
    csharp/protos/unittest_deep_dependencies/file384.proto \
    csharp/protos/unittest_deep_dependencies/file385.proto \
    csharp/protos/unittest_deep_dependencies/file386.proto \
    csharp/protos/unittest_deep_dependencies/file387.proto \
    csharp/protos/unittest_deep_dependencies/file388.proto \
    csharp/protos/unittest_deep_dependencies/file389.proto \
    csharp/protos/unittest_deep_dependencies/file390.proto \
    csharp/protos/unittest_deep_dependencies/file391.proto \
    csharp/protos/unittest_deep_dependencies/file392.proto \
    csharp/protos/unittest_deep_dependencies/file393.proto

# AddressBook sample protos
$PROTOC -Iexamples -Isrc --csharp_out=csharp/src/AddressBook \
    --csharp_opt=file_extension=.pb.cs \
    examples/addressbook.proto

# Conformance tests
$PROTOC -I. --csharp_out=csharp/src/Google.Protobuf.Conformance \
    --csharp_opt=file_extension=.pb.cs \
    conformance/conformance.proto
