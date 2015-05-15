## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign

# Build . before src so that our all-local and clean-local hooks kicks in at
# the right time.
SUBDIRS = . src

# Always include gtest in distributions.
DIST_SUBDIRS = $(subdirs) src conformance

# Build gtest before we build protobuf tests.  We don't add gtest to SUBDIRS
# because then "make check" would also build and run all of gtest's own tests,
# which takes a lot of time and is generally not useful to us.  Also, we don't
# want "make install" to recurse into gtest since we don't want to overwrite
# the installed version of gtest if there is one.
check-local:
	@echo "Making lib/libgtest.a lib/libgtest_main.a in gtest"
	@cd gtest && $(MAKE) $(AM_MAKEFLAGS) lib/libgtest.la lib/libgtest_main.la

# We would like to clean gtest when "make clean" is invoked.  But we have to
# be careful because clean-local is also invoked during "make distclean", but
# "make distclean" already recurses into gtest because it's listed among the
# DIST_SUBDIRS.  distclean will delete gtest/Makefile, so if we then try to
# cd to the directory again and "make clean" it will fail.  So, check that the
# Makefile exists before recursing.
clean-local:
	@if test -e gtest/Makefile; then \
	  echo "Making clean in gtest"; \
	  cd gtest && $(MAKE) $(AM_MAKEFLAGS) clean; \
	fi; \
	if test -e conformance/Makefile; then \
	  echo "Making clean in conformance"; \
	  cd conformance && $(MAKE) $(AM_MAKEFLAGS) clean; \
	fi

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = protobuf.pc protobuf-lite.pc

java_EXTRA_DIST=                                                             \
  java/src/main/java/com/google/protobuf/AbstractMessage.java                \
  java/src/main/java/com/google/protobuf/AbstractMessageLite.java            \
  java/src/main/java/com/google/protobuf/AbstractParser.java                 \
  java/src/main/java/com/google/protobuf/BlockingRpcChannel.java             \
  java/src/main/java/com/google/protobuf/BlockingService.java                \
  java/src/main/java/com/google/protobuf/BoundedByteString.java              \
  java/src/main/java/com/google/protobuf/ByteString.java                     \
  java/src/main/java/com/google/protobuf/CodedInputStream.java               \
  java/src/main/java/com/google/protobuf/CodedOutputStream.java              \
  java/src/main/java/com/google/protobuf/Descriptors.java                    \
  java/src/main/java/com/google/protobuf/DynamicMessage.java                 \
  java/src/main/java/com/google/protobuf/Extension.java                      \
  java/src/main/java/com/google/protobuf/ExtensionLite.java                  \
  java/src/main/java/com/google/protobuf/ExtensionRegistry.java              \
  java/src/main/java/com/google/protobuf/ExtensionRegistryLite.java          \
  java/src/main/java/com/google/protobuf/FieldSet.java                       \
  java/src/main/java/com/google/protobuf/GeneratedMessage.java               \
  java/src/main/java/com/google/protobuf/GeneratedMessageLite.java           \
  java/src/main/java/com/google/protobuf/Internal.java                       \
  java/src/main/java/com/google/protobuf/InvalidProtocolBufferException.java \
  java/src/main/java/com/google/protobuf/LazyField.java                      \
  java/src/main/java/com/google/protobuf/LazyFieldLite.java                  \
  java/src/main/java/com/google/protobuf/LazyStringArrayList.java            \
  java/src/main/java/com/google/protobuf/LazyStringList.java                 \
  java/src/main/java/com/google/protobuf/LiteralByteString.java              \
  java/src/main/java/com/google/protobuf/MapEntry.java                       \
  java/src/main/java/com/google/protobuf/MapEntryLite.java                   \
  java/src/main/java/com/google/protobuf/MapField.java                       \
  java/src/main/java/com/google/protobuf/MapFieldLite.java                   \
  java/src/main/java/com/google/protobuf/Message.java                        \
  java/src/main/java/com/google/protobuf/MessageLite.java                    \
  java/src/main/java/com/google/protobuf/MessageLiteOrBuilder.java           \
  java/src/main/java/com/google/protobuf/MessageOrBuilder.java               \
  java/src/main/java/com/google/protobuf/MessageReflection.java              \
  java/src/main/java/com/google/protobuf/Parser.java                         \
  java/src/main/java/com/google/protobuf/ProtocolMessageEnum.java            \
  java/src/main/java/com/google/protobuf/ProtocolStringList.java             \
  java/src/main/java/com/google/protobuf/RepeatedFieldBuilder.java           \
  java/src/main/java/com/google/protobuf/RopeByteString.java                 \
  java/src/main/java/com/google/protobuf/RpcCallback.java                    \
  java/src/main/java/com/google/protobuf/RpcChannel.java                     \
  java/src/main/java/com/google/protobuf/RpcController.java                  \
  java/src/main/java/com/google/protobuf/RpcUtil.java                        \
  java/src/main/java/com/google/protobuf/ServiceException.java               \
  java/src/main/java/com/google/protobuf/Service.java                        \
  java/src/main/java/com/google/protobuf/SingleFieldBuilder.java             \
  java/src/main/java/com/google/protobuf/SmallSortedMap.java                 \
  java/src/main/java/com/google/protobuf/TextFormat.java                     \
  java/src/main/java/com/google/protobuf/UninitializedMessageException.java  \
  java/src/main/java/com/google/protobuf/UnknownFieldSet.java                \
  java/src/main/java/com/google/protobuf/UnknownFieldSetLite.java            \
  java/src/main/java/com/google/protobuf/UnmodifiableLazyStringList.java     \
  java/src/main/java/com/google/protobuf/Utf8.java                           \
  java/src/main/java/com/google/protobuf/WireFormat.java                     \
  java/src/test/java/com/google/protobuf/AbstractMessageTest.java            \
  java/src/test/java/com/google/protobuf/BoundedByteStringTest.java          \
  java/src/test/java/com/google/protobuf/ByteStringTest.java                 \
  java/src/test/java/com/google/protobuf/CheckUtf8Test.java                  \
  java/src/test/java/com/google/protobuf/CodedInputStreamTest.java           \
  java/src/test/java/com/google/protobuf/CodedOutputStreamTest.java          \
  java/src/test/java/com/google/protobuf/DeprecatedFieldTest.java            \
  java/src/test/java/com/google/protobuf/DescriptorsTest.java                \
  java/src/test/java/com/google/protobuf/DynamicMessageTest.java             \
  java/src/test/java/com/google/protobuf/FieldPresenceTest.java              \
  java/src/test/java/com/google/protobuf/ForceFieldBuildersPreRun.java       \
  java/src/test/java/com/google/protobuf/GeneratedMessageTest.java           \
  java/src/test/java/com/google/protobuf/IsValidUtf8Test.java                \
  java/src/test/java/com/google/protobuf/IsValidUtf8TestUtil.java            \
  java/src/test/java/com/google/protobuf/LazyFieldLiteTest.java              \
  java/src/test/java/com/google/protobuf/LazyFieldTest.java                  \
  java/src/test/java/com/google/protobuf/LazyMessageLiteTest.java            \
  java/src/test/java/com/google/protobuf/LazyStringArrayListTest.java        \
  java/src/test/java/com/google/protobuf/LazyStringEndToEndTest.java         \
  java/src/test/java/com/google/protobuf/LiteEqualsAndHashTest.java          \
  java/src/test/java/com/google/protobuf/LiteralByteStringTest.java          \
  java/src/test/java/com/google/protobuf/LiteTest.java                       \
  java/src/test/java/com/google/protobuf/MapForProto2LiteTest.java           \
  java/src/test/java/com/google/protobuf/MapForProto2Test.java               \
  java/src/test/java/com/google/protobuf/MapTest.java                        \
  java/src/test/java/com/google/protobuf/MessageTest.java                    \
  java/src/test/java/com/google/protobuf/NestedBuildersTest.java             \
  java/src/test/java/com/google/protobuf/ParserTest.java                     \
  java/src/test/java/com/google/protobuf/RepeatedFieldBuilderTest.java       \
  java/src/test/java/com/google/protobuf/RopeByteStringSubstringTest.java    \
  java/src/test/java/com/google/protobuf/RopeByteStringTest.java             \
  java/src/test/java/com/google/protobuf/ServiceTest.java                    \
  java/src/test/java/com/google/protobuf/SingleFieldBuilderTest.java         \
  java/src/test/java/com/google/protobuf/SmallSortedMapTest.java             \
  java/src/test/java/com/google/protobuf/TestBadIdentifiers.java             \
  java/src/test/java/com/google/protobuf/TestUtil.java                       \
  java/src/test/java/com/google/protobuf/TextFormatTest.java                 \
  java/src/test/java/com/google/protobuf/UnknownEnumValueTest.java           \
  java/src/test/java/com/google/protobuf/UnknownFieldSetLiteTest.java        \
  java/src/test/java/com/google/protobuf/UnknownFieldSetTest.java            \
  java/src/test/java/com/google/protobuf/UnmodifiableLazyStringListTest.java \
  java/src/test/java/com/google/protobuf/WireFormatTest.java                 \
  java/src/test/java/com/google/protobuf/field_presence_test.proto           \
  java/src/test/java/com/google/protobuf/lazy_fields_lite.proto              \
  java/src/test/java/com/google/protobuf/lite_equals_and_hash.proto          \
  java/src/test/java/com/google/protobuf/map_for_proto2_lite_test.proto      \
  java/src/test/java/com/google/protobuf/map_for_proto2_test.proto           \
  java/src/test/java/com/google/protobuf/map_test.proto                      \
  java/src/test/java/com/google/protobuf/multiple_files_test.proto           \
  java/src/test/java/com/google/protobuf/nested_builders_test.proto          \
  java/src/test/java/com/google/protobuf/nested_extension_lite.proto         \
  java/src/test/java/com/google/protobuf/nested_extension.proto              \
  java/src/test/java/com/google/protobuf/non_nested_extension_lite.proto     \
  java/src/test/java/com/google/protobuf/non_nested_extension.proto          \
  java/src/test/java/com/google/protobuf/outer_class_name_test2.proto        \
  java/src/test/java/com/google/protobuf/outer_class_name_test3.proto        \
  java/src/test/java/com/google/protobuf/outer_class_name_test.proto         \
  java/src/test/java/com/google/protobuf/test_bad_identifiers.proto          \
  java/src/test/java/com/google/protobuf/test_check_utf8.proto               \
  java/src/test/java/com/google/protobuf/test_check_utf8_size.proto          \
  java/src/test/java/com/google/protobuf/test_custom_options.proto           \
  java/pom.xml                                                               \
  java/README.md

javanano_EXTRA_DIST=                                                                      \
  javanano/src/main/java/com/google/protobuf/nano/CodedOutputByteBufferNano.java          \
  javanano/src/main/java/com/google/protobuf/nano/FieldData.java                          \
  javanano/src/main/java/com/google/protobuf/nano/FieldArray.java                         \
  javanano/src/main/java/com/google/protobuf/nano/WireFormatNano.java                     \
  javanano/src/main/java/com/google/protobuf/nano/Extension.java                          \
  javanano/src/main/java/com/google/protobuf/nano/CodedInputByteBufferNano.java           \
  javanano/src/main/java/com/google/protobuf/nano/UnknownFieldData.java                   \
  javanano/src/main/java/com/google/protobuf/nano/MessageNano.java                        \
  javanano/src/main/java/com/google/protobuf/nano/InternalNano.java                       \
  javanano/src/main/java/com/google/protobuf/nano/InvalidProtocolBufferNanoException.java \
  javanano/src/main/java/com/google/protobuf/nano/MapFactories.java                       \
  javanano/src/main/java/com/google/protobuf/nano/ExtendableMessageNano.java              \
  javanano/src/main/java/com/google/protobuf/nano/MessageNanoPrinter.java                 \
  javanano/src/test/java/com/google/protobuf/nano/unittest_accessors_nano.proto           \
  javanano/src/test/java/com/google/protobuf/nano/unittest_enum_class_nano.proto          \
  javanano/src/test/java/com/google/protobuf/nano/unittest_reference_types_nano.proto     \
  javanano/src/test/java/com/google/protobuf/nano/unittest_extension_repeated_nano.proto  \
  javanano/src/test/java/com/google/protobuf/nano/unittest_has_nano.proto                 \
  javanano/src/test/java/com/google/protobuf/nano/unittest_nano.proto                     \
  javanano/src/test/java/com/google/protobuf/nano/unittest_multiple_nameclash_nano.proto  \
  javanano/src/test/java/com/google/protobuf/nano/unittest_single_nano.proto              \
  javanano/src/test/java/com/google/protobuf/nano/NanoTest.java                           \
  javanano/src/test/java/com/google/protobuf/nano/unittest_simple_nano.proto              \
  javanano/src/test/java/com/google/protobuf/nano/unittest_import_nano.proto              \
  javanano/src/test/java/com/google/protobuf/nano/unittest_repeated_merge_nano.proto      \
  javanano/src/test/java/com/google/protobuf/nano/unittest_extension_nano.proto           \
  javanano/src/test/java/com/google/protobuf/nano/unittest_repeated_packables_nano.proto  \
  javanano/src/test/java/com/google/protobuf/nano/unittest_extension_singular_nano.proto  \
  javanano/src/test/java/com/google/protobuf/nano/unittest_recursive_nano.proto           \
  javanano/src/test/java/com/google/protobuf/nano/unittest_extension_packed_nano.proto    \
  javanano/src/test/java/com/google/protobuf/nano/unittest_enum_validity_nano.proto       \
  javanano/src/test/java/com/google/protobuf/nano/unittest_stringutf8_nano.proto          \
  javanano/src/test/java/com/google/protobuf/nano/unittest_multiple_nano.proto            \
  javanano/src/test/java/com/google/protobuf/nano/unittest_enum_class_multiple_nano.proto \
  javanano/src/test/java/com/google/protobuf/nano/map_test.proto                          \
  javanano/README.md                                                                      \
  javanano/pom.xml

objectivec_EXTRA_DIST=                                                       \
  objectivec/DevTools/generate_descriptors_proto.sh                          \
  objectivec/DevTools/pddm.py                                                \
  objectivec/DevTools/pddm_tests.py                                          \
  objectivec/google/protobuf/Descriptor.pbobjc.h                             \
  objectivec/google/protobuf/Descriptor.pbobjc.m                             \
  objectivec/google/protobuf/Duration.pbobjc.h                               \
  objectivec/google/protobuf/Duration.pbobjc.m                               \
  objectivec/google/protobuf/Timestamp.pbobjc.h                              \
  objectivec/google/protobuf/Timestamp.pbobjc.m                              \
  objectivec/GPBArray.h                                                      \
  objectivec/GPBArray.m                                                      \
  objectivec/GPBArray_PackagePrivate.h                                       \
  objectivec/GPBBootstrap.h                                                  \
  objectivec/GPBCodedInputStream.h                                           \
  objectivec/GPBCodedInputStream.m                                           \
  objectivec/GPBCodedInputStream_PackagePrivate.h                            \
  objectivec/GPBCodedOutputStream.h                                          \
  objectivec/GPBCodedOutputStream.m                                          \
  objectivec/GPBDescriptor.h                                                 \
  objectivec/GPBDescriptor.m                                                 \
  objectivec/GPBDescriptor_PackagePrivate.h                                  \
  objectivec/GPBDictionary.h                                                 \
  objectivec/GPBDictionary.m                                                 \
  objectivec/GPBDictionary_PackagePrivate.h                                  \
  objectivec/GPBExtensionField.h                                             \
  objectivec/GPBExtensionField.m                                             \
  objectivec/GPBExtensionField_PackagePrivate.h                              \
  objectivec/GPBExtensionRegistry.h                                          \
  objectivec/GPBExtensionRegistry.m                                          \
  objectivec/GPBExtensionRegistry_PackagePrivate.h                           \
  objectivec/GPBField.h                                                      \
  objectivec/GPBField.m                                                      \
  objectivec/GPBField_PackagePrivate.h                                       \
  objectivec/GPBMessage.h                                                    \
  objectivec/GPBMessage.m                                                    \
  objectivec/GPBMessage_PackagePrivate.h                                     \
  objectivec/GPBProtocolBuffers.h                                            \
  objectivec/GPBProtocolBuffers.m                                            \
  objectivec/GPBProtocolBuffers_RuntimeSupport.h                             \
  objectivec/GPBRootObject.h                                                 \
  objectivec/GPBRootObject.m                                                 \
  objectivec/GPBRootObject_PackagePrivate.h                                  \
  objectivec/GPBTypes.h                                                      \
  objectivec/GPBUnknownFieldSet.h                                            \
  objectivec/GPBUnknownFieldSet.m                                            \
  objectivec/GPBUnknownFieldSet_PackagePrivate.h                             \
  objectivec/GPBUtilities.h                                                  \
  objectivec/GPBUtilities.m                                                  \
  objectivec/GPBUtilities_PackagePrivate.h                                   \
  objectivec/GPBWellKnownTypes.h                                             \
  objectivec/GPBWellKnownTypes.m                                             \
  objectivec/GPBWireFormat.h                                                 \
  objectivec/GPBWireFormat.m                                                 \
  objectivec/ProtocolBuffers_iOS.xcodeproj/project.pbxproj                   \
  objectivec/ProtocolBuffers_iOS.xcodeproj/project.xcworkspace/contents.xcworkspacedata \
  objectivec/ProtocolBuffers_iOS.xcodeproj/project.xcworkspace/xcshareddata/WorkspaceSettings.xcsettings \
  objectivec/ProtocolBuffers_iOS.xcodeproj/xcshareddata/xcbaselines/8BBEA4A5147C727100C4ADB7.xcbaseline/FFE465CA-0E74-40E8-9F09-500B66B7DCB2.plist \
  objectivec/ProtocolBuffers_iOS.xcodeproj/xcshareddata/xcbaselines/8BBEA4A5147C727100C4ADB7.xcbaseline/Info.plist \
  objectivec/ProtocolBuffers_iOS.xcodeproj/xcshareddata/xcschemes/PerformanceTests.xcscheme \
  objectivec/ProtocolBuffers_iOS.xcodeproj/xcshareddata/xcschemes/ProtocolBuffers.xcscheme \
  objectivec/ProtocolBuffers_OSX.xcodeproj/project.pbxproj                   \
  objectivec/ProtocolBuffers_OSX.xcodeproj/project.xcworkspace/contents.xcworkspacedata \
  objectivec/ProtocolBuffers_OSX.xcodeproj/project.xcworkspace/xcshareddata/WorkspaceSettings.xcsettings \
  objectivec/ProtocolBuffers_OSX.xcodeproj/xcshareddata/xcschemes/PerformanceTests.xcscheme \
  objectivec/ProtocolBuffers_OSX.xcodeproj/xcshareddata/xcschemes/ProtocolBuffers.xcscheme \
  objectivec/Tests/Filter1.txt                                               \
  objectivec/Tests/Filter2.txt                                               \
  objectivec/Tests/golden_message                                            \
  objectivec/Tests/golden_packed_fields_message                              \
  objectivec/Tests/GPBARCUnittestProtos.m                                    \
  objectivec/Tests/GPBArrayTests.m                                           \
  objectivec/Tests/GPBCodedInputStreamTests.m                                \
  objectivec/Tests/GPBCodedOuputStreamTests.m                                \
  objectivec/Tests/GPBConcurrencyTests.m                                     \
  objectivec/Tests/GPBDescriptorTests.m                                      \
  objectivec/Tests/GPBDictionaryTests+Bool.m                                 \
  objectivec/Tests/GPBDictionaryTests+Int32.m                                \
  objectivec/Tests/GPBDictionaryTests+Int64.m                                \
  objectivec/Tests/GPBDictionaryTests+String.m                               \
  objectivec/Tests/GPBDictionaryTests+UInt32.m                               \
  objectivec/Tests/GPBDictionaryTests+UInt64.m                               \
  objectivec/Tests/GPBDictionaryTests.pddm                                   \
  objectivec/Tests/GPBFilteredMessageTests.m                                 \
  objectivec/Tests/GPBMessageTests+Merge.m                                   \
  objectivec/Tests/GPBMessageTests+Runtime.m                                 \
  objectivec/Tests/GPBMessageTests+Serialization.m                           \
  objectivec/Tests/GPBMessageTests.m                                         \
  objectivec/Tests/GPBPerfTests.m                                            \
  objectivec/Tests/GPBStringTests.m                                          \
  objectivec/Tests/GPBSwiftTests.swift                                       \
  objectivec/Tests/GPBTestUtilities.h                                        \
  objectivec/Tests/GPBTestUtilities.m                                        \
  objectivec/Tests/GPBUnittestProtos.m                                       \
  objectivec/Tests/GPBUnknownFieldSetTest.m                                  \
  objectivec/Tests/GPBUtilitiesTests.m                                       \
  objectivec/Tests/GPBWellKnownTypesTest.m                                   \
  objectivec/Tests/GPBWireFormatTests.m                                      \
  objectivec/Tests/iOSTestHarness/AppDelegate.m                              \
  objectivec/Tests/iOSTestHarness/en.lproj/InfoPlist.strings                 \
  objectivec/Tests/iOSTestHarness/Images.xcassets/AppIcon.appiconset/Contents.json \
  objectivec/Tests/iOSTestHarness/Images.xcassets/AppIcon.appiconset/iPad6.png \
  objectivec/Tests/iOSTestHarness/Images.xcassets/AppIcon.appiconset/iPad6@2x.png \
  objectivec/Tests/iOSTestHarness/Images.xcassets/AppIcon.appiconset/iPad7.png \
  objectivec/Tests/iOSTestHarness/Images.xcassets/AppIcon.appiconset/iPad7@2x.png \
  objectivec/Tests/iOSTestHarness/Images.xcassets/AppIcon.appiconset/iPhone6.png \
  objectivec/Tests/iOSTestHarness/Images.xcassets/AppIcon.appiconset/iPhone6@2x.png \
  objectivec/Tests/iOSTestHarness/Images.xcassets/AppIcon.appiconset/iPhone7@2x.png \
  objectivec/Tests/iOSTestHarness/Images.xcassets/AppIcon.appiconset/iPhone7@3x.png \
  objectivec/Tests/iOSTestHarness/Images.xcassets/LaunchImage.launchimage/Contents.json \
  objectivec/Tests/iOSTestHarness/Info.plist                                  \
  objectivec/Tests/iOSTestHarness/LaunchScreen.xib                            \
  objectivec/Tests/text_format_map_unittest_data.txt                          \
  objectivec/Tests/text_format_unittest_data.txt                              \
  objectivec/Tests/unittest_cycle.proto                                       \
  objectivec/Tests/unittest_filter.proto                                      \
  objectivec/Tests/unittest_name_mangling.proto                               \
  objectivec/Tests/unittest_objc.proto                                        \
  objectivec/Tests/unittest_runtime_proto2.proto                              \
  objectivec/Tests/unittest_runtime_proto3.proto                              \
  objectivec/Tests/UnitTests-Bridging-Header.h                                \
  objectivec/Tests/UnitTests-Info.plist                                       \
  Protobuf.podspec

python_EXTRA_DIST=                                                           \
  python/google/protobuf/internal/api_implementation.cc                      \
  python/google/protobuf/internal/api_implementation.py                      \
  python/google/protobuf/internal/containers.py                              \
  python/google/protobuf/internal/decoder.py                                 \
  python/google/protobuf/internal/descriptor_database_test.py                \
  python/google/protobuf/internal/descriptor_pool_test.py                    \
  python/google/protobuf/internal/descriptor_pool_test1.proto                \
  python/google/protobuf/internal/descriptor_pool_test2.proto                \
  python/google/protobuf/internal/descriptor_test.py                         \
  python/google/protobuf/internal/encoder.py                                 \
  python/google/protobuf/internal/enum_type_wrapper.py                       \
  python/google/protobuf/internal/factory_test1.proto                        \
  python/google/protobuf/internal/factory_test2.proto                        \
  python/google/protobuf/internal/generator_test.py                          \
  python/google/protobuf/internal/message_factory_test.py                    \
  python/google/protobuf/internal/message_listener.py                        \
  python/google/protobuf/internal/message_test.py                            \
  python/google/protobuf/internal/missing_enum_values.proto                  \
  python/google/protobuf/internal/more_extensions.proto                      \
  python/google/protobuf/internal/more_extensions_dynamic.proto              \
  python/google/protobuf/internal/more_messages.proto                        \
  python/google/protobuf/internal/_parameterized.py                          \
  python/google/protobuf/internal/proto_builder_test.py                      \
  python/google/protobuf/internal/python_message.py                          \
  python/google/protobuf/internal/reflection_test.py                         \
  python/google/protobuf/internal/service_reflection_test.py                 \
  python/google/protobuf/internal/symbol_database_test.py                    \
  python/google/protobuf/internal/test_bad_identifiers.proto                 \
  python/google/protobuf/internal/test_util.py                               \
  python/google/protobuf/internal/text_encoding_test.py                      \
  python/google/protobuf/internal/text_format_test.py                        \
  python/google/protobuf/internal/type_checkers.py                           \
  python/google/protobuf/internal/unknown_fields_test.py                     \
  python/google/protobuf/internal/wire_format.py                             \
  python/google/protobuf/internal/wire_format_test.py                        \
  python/google/protobuf/internal/__init__.py                                \
  python/google/protobuf/internal/import_test_package/__init__.py            \
  python/google/protobuf/internal/import_test_package/inner.proto            \
  python/google/protobuf/internal/import_test_package/outer.proto            \
  python/google/protobuf/pyext/README                                        \
  python/google/protobuf/pyext/cpp_message.py                                \
  python/google/protobuf/pyext/descriptor.h                                  \
  python/google/protobuf/pyext/descriptor.cc                                 \
  python/google/protobuf/pyext/descriptor_pool.h                             \
  python/google/protobuf/pyext/descriptor_pool.cc                            \
  python/google/protobuf/pyext/descriptor_containers.h                       \
  python/google/protobuf/pyext/descriptor_containers.cc                      \
  python/google/protobuf/pyext/extension_dict.h                              \
  python/google/protobuf/pyext/extension_dict.cc                             \
  python/google/protobuf/pyext/message.h                                     \
  python/google/protobuf/pyext/message.cc                                    \
  python/google/protobuf/pyext/proto2_api_test.proto                         \
  python/google/protobuf/pyext/python.proto                                  \
  python/google/protobuf/pyext/python_protobuf.h                             \
  python/google/protobuf/pyext/repeated_composite_container.h                \
  python/google/protobuf/pyext/repeated_composite_container.cc               \
  python/google/protobuf/pyext/repeated_scalar_container.h                   \
  python/google/protobuf/pyext/repeated_scalar_container.cc                  \
  python/google/protobuf/pyext/scoped_pyobject_ptr.h                         \
  python/google/protobuf/pyext/__init__.py                                   \
  python/google/protobuf/descriptor.py                                       \
  python/google/protobuf/descriptor_database.py                              \
  python/google/protobuf/descriptor_pool.py                                  \
  python/google/protobuf/message.py                                          \
  python/google/protobuf/message_factory.py                                  \
  python/google/protobuf/proto_builder.py                                    \
  python/google/protobuf/reflection.py                                       \
  python/google/protobuf/service.py                                          \
  python/google/protobuf/service_reflection.py                               \
  python/google/protobuf/symbol_database.py                                  \
  python/google/protobuf/text_encoding.py                                    \
  python/google/protobuf/text_format.py                                      \
  python/google/protobuf/__init__.py                                         \
  python/google/__init__.py                                                  \
  python/ez_setup.py                                                         \
  python/setup.py                                                            \
  python/mox.py                                                              \
  python/stubout.py                                                          \
  python/README.md

ruby_EXTRA_DIST=                                                             \
  ruby/README.md                                                             \
  ruby/Rakefile                                                              \
  ruby/ext/google/protobuf_c/defs.c                                          \
  ruby/ext/google/protobuf_c/encode_decode.c                                 \
  ruby/ext/google/protobuf_c/extconf.rb                                      \
  ruby/ext/google/protobuf_c/map.c                                           \
  ruby/ext/google/protobuf_c/message.c                                       \
  ruby/ext/google/protobuf_c/protobuf.c                                      \
  ruby/ext/google/protobuf_c/protobuf.h                                      \
  ruby/ext/google/protobuf_c/repeated_field.c                                \
  ruby/ext/google/protobuf_c/storage.c                                       \
  ruby/ext/google/protobuf_c/upb.c                                           \
  ruby/ext/google/protobuf_c/upb.h                                           \
  ruby/google-protobuf.gemspec                                               \
  ruby/lib/google/protobuf.rb                                                \
  ruby/tests/basic.rb                                                        \
  ruby/tests/stress.rb                                                       \
  ruby/tests/generated_code.proto                                            \
  ruby/tests/generated_code.rb                                               \
  ruby/tests/generated_code_test.rb

all_EXTRA_DIST=$(java_EXTRA_DIST) $(javanano_EXTRA_DIST) $(objectivec_EXTRA_DIST) $(python_EXTRA_DIST) $(ruby_EXTRA_DIST)

EXTRA_DIST = $(@DIST_LANG@_EXTRA_DIST)                                       \
  autogen.sh                                                                 \
  generate_descriptor_proto.sh                                               \
  README.md                                                                  \
  INSTALL.txt                                                                \
  LICENSE                                                                    \
  CONTRIBUTORS.txt                                                           \
  CHANGES.txt                                                                \
  config.h.include                                                           \
  editors/README.txt                                                         \
  editors/proto.vim                                                          \
  editors/protobuf-mode.el                                                   \
  vsprojects/config.h                                                        \
  vsprojects/google/protobuf/stubs/pbconfig.h                                \
  vsprojects/extract_includes.bat                                            \
  vsprojects/libprotobuf.vcproj                                              \
  vsprojects/libprotobuf-lite.vcproj                                         \
  vsprojects/libprotoc.vcproj                                                \
  vsprojects/protobuf.sln                                                    \
  vsprojects/protoc.vcproj                                                   \
  vsprojects/readme.txt                                                      \
  vsprojects/test_plugin.vcproj                                              \
  vsprojects/tests.vcproj                                                    \
  vsprojects/lite-test.vcproj                                                \
  vsprojects/convert2008to2005.sh                                            \
  examples/README.txt                                                        \
  examples/Makefile                                                          \
  examples/addressbook.proto                                                 \
  examples/add_person.cc                                                     \
  examples/list_people.cc                                                    \
  examples/AddPerson.java                                                    \
  examples/ListPeople.java                                                   \
  examples/add_person.py                                                     \
  examples/list_people.py

# Deletes all the files generated by autogen.sh.
MAINTAINERCLEANFILES =   \
  aclocal.m4             \
  config.guess           \
  config.sub             \
  configure              \
  depcomp                \
  install-sh             \
  ltmain.sh              \
  Makefile.in            \
  missing                \
  mkinstalldirs          \
  config.h.in            \
  stamp.h.in             \
  m4/ltsugar.m4          \
  m4/libtool.m4          \
  m4/ltversion.m4        \
  m4/lt~obsolete.m4      \
  m4/ltoptions.m4
