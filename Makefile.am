## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign

# Build . before src so that our all-local and clean-local hooks kicks in at
# the right time.
SUBDIRS = . src

# Always include gmock in distributions.
DIST_SUBDIRS = $(subdirs) src conformance benchmarks

# Build gmock before we build protobuf tests.  We don't add gmock to SUBDIRS
# because then "make check" would also build and run all of gmock's own tests,
# which takes a lot of time and is generally not useful to us.  Also, we don't
# want "make install" to recurse into gmock since we don't want to overwrite
# the installed version of gmock if there is one.
check-local:
	@echo "Making lib/libgmock.a lib/libgmock_main.a in gmock"
	@cd gmock && $(MAKE) $(AM_MAKEFLAGS) lib/libgmock.la lib/libgmock_main.la
	@cd gmock/gtest && $(MAKE) $(AM_MAKEFLAGS) lib/libgtest.la lib/libgtest_main.la

# We would like to clean gmock when "make clean" is invoked.  But we have to
# be careful because clean-local is also invoked during "make distclean", but
# "make distclean" already recurses into gmock because it's listed among the
# DIST_SUBDIRS.  distclean will delete gmock/Makefile, so if we then try to
# cd to the directory again and "make clean" it will fail.  So, check that the
# Makefile exists before recursing.
clean-local:
	@if test -e gmock/Makefile; then \
	  echo "Making clean in gmock"; \
	  cd gmock && $(MAKE) $(AM_MAKEFLAGS) clean; \
	fi; \
	if test -e conformance/Makefile; then \
	  echo "Making clean in conformance"; \
	  cd conformance && $(MAKE) $(AM_MAKEFLAGS) clean; \
	fi; \
	if test -e benchmarks/Makefile; then \
	  echo "Making clean in benchmarks"; \
	  cd benchmarks && $(MAKE) $(AM_MAKEFLAGS) clean; \
	fi; \
	if test -e objectivec/DevTools; then \
	  echo "Cleaning any ObjC pyc files"; \
	  rm -f objectivec/DevTools/*.pyc; \
	fi

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = protobuf.pc protobuf-lite.pc

java_EXTRA_DIST=                                                                   \
  java/README.md                                                                   \
  java/core/generate-sources-build.xml                                             \
  java/core/generate-test-sources-build.xml                                        \
  java/core/pom.xml                                                                \
  java/core/src/main/java/com/google/protobuf/AbstractMessage.java                 \
  java/core/src/main/java/com/google/protobuf/AbstractMessageLite.java             \
  java/core/src/main/java/com/google/protobuf/AbstractParser.java                  \
  java/core/src/main/java/com/google/protobuf/AbstractProtobufList.java            \
  java/core/src/main/java/com/google/protobuf/BlockingRpcChannel.java              \
  java/core/src/main/java/com/google/protobuf/BlockingService.java                 \
  java/core/src/main/java/com/google/protobuf/BooleanArrayList.java                \
  java/core/src/main/java/com/google/protobuf/ByteBufferWriter.java                \
  java/core/src/main/java/com/google/protobuf/ByteOutput.java                      \
  java/core/src/main/java/com/google/protobuf/ByteString.java                      \
  java/core/src/main/java/com/google/protobuf/CodedInputStream.java                \
  java/core/src/main/java/com/google/protobuf/CodedOutputStream.java               \
  java/core/src/main/java/com/google/protobuf/Descriptors.java                     \
  java/core/src/main/java/com/google/protobuf/DoubleArrayList.java                 \
  java/core/src/main/java/com/google/protobuf/DynamicMessage.java                  \
  java/core/src/main/java/com/google/protobuf/ExperimentalApi.java                 \
  java/core/src/main/java/com/google/protobuf/Extension.java                       \
  java/core/src/main/java/com/google/protobuf/ExtensionLite.java                   \
  java/core/src/main/java/com/google/protobuf/ExtensionRegistry.java               \
  java/core/src/main/java/com/google/protobuf/ExtensionRegistryFactory.java        \
  java/core/src/main/java/com/google/protobuf/ExtensionRegistryLite.java           \
  java/core/src/main/java/com/google/protobuf/FieldSet.java                        \
  java/core/src/main/java/com/google/protobuf/FloatArrayList.java                  \
  java/core/src/main/java/com/google/protobuf/GeneratedMessage.java                \
  java/core/src/main/java/com/google/protobuf/GeneratedMessageLite.java            \
  java/core/src/main/java/com/google/protobuf/IntArrayList.java                    \
  java/core/src/main/java/com/google/protobuf/Internal.java                        \
  java/core/src/main/java/com/google/protobuf/InvalidProtocolBufferException.java  \
  java/core/src/main/java/com/google/protobuf/LazyField.java                       \
  java/core/src/main/java/com/google/protobuf/LazyFieldLite.java                   \
  java/core/src/main/java/com/google/protobuf/LazyStringArrayList.java             \
  java/core/src/main/java/com/google/protobuf/LazyStringList.java                  \
  java/core/src/main/java/com/google/protobuf/LongArrayList.java                   \
  java/core/src/main/java/com/google/protobuf/MapEntry.java                        \
  java/core/src/main/java/com/google/protobuf/MapEntryLite.java                    \
  java/core/src/main/java/com/google/protobuf/MapField.java                        \
  java/core/src/main/java/com/google/protobuf/MapFieldLite.java                    \
  java/core/src/main/java/com/google/protobuf/Message.java                         \
  java/core/src/main/java/com/google/protobuf/MessageLite.java                     \
  java/core/src/main/java/com/google/protobuf/MessageLiteOrBuilder.java            \
  java/core/src/main/java/com/google/protobuf/MessageLiteToString.java             \
  java/core/src/main/java/com/google/protobuf/MessageOrBuilder.java                \
  java/core/src/main/java/com/google/protobuf/MessageReflection.java               \
  java/core/src/main/java/com/google/protobuf/MutabilityOracle.java                \
  java/core/src/main/java/com/google/protobuf/NioByteString.java                   \
  java/core/src/main/java/com/google/protobuf/Parser.java                          \
  java/core/src/main/java/com/google/protobuf/ProtobufArrayList.java               \
  java/core/src/main/java/com/google/protobuf/ProtocolMessageEnum.java             \
  java/core/src/main/java/com/google/protobuf/ProtocolStringList.java              \
  java/core/src/main/java/com/google/protobuf/RepeatedFieldBuilder.java            \
  java/core/src/main/java/com/google/protobuf/RepeatedFieldBuilderV3.java          \
  java/core/src/main/java/com/google/protobuf/RopeByteString.java                  \
  java/core/src/main/java/com/google/protobuf/RpcCallback.java                     \
  java/core/src/main/java/com/google/protobuf/RpcChannel.java                      \
  java/core/src/main/java/com/google/protobuf/RpcController.java                   \
  java/core/src/main/java/com/google/protobuf/RpcUtil.java                         \
  java/core/src/main/java/com/google/protobuf/Service.java                         \
  java/core/src/main/java/com/google/protobuf/ServiceException.java                \
  java/core/src/main/java/com/google/protobuf/SingleFieldBuilder.java              \
  java/core/src/main/java/com/google/protobuf/SingleFieldBuilderV3.java            \
  java/core/src/main/java/com/google/protobuf/SmallSortedMap.java                  \
  java/core/src/main/java/com/google/protobuf/TextFormat.java                      \
  java/core/src/main/java/com/google/protobuf/TextFormatEscaper.java               \
  java/core/src/main/java/com/google/protobuf/TextFormatParseInfoTree.java         \
  java/core/src/main/java/com/google/protobuf/TextFormatParseLocation.java         \
  java/core/src/main/java/com/google/protobuf/UninitializedMessageException.java   \
  java/core/src/main/java/com/google/protobuf/UnknownFieldSet.java                 \
  java/core/src/main/java/com/google/protobuf/UnknownFieldSetLite.java             \
  java/core/src/main/java/com/google/protobuf/UnmodifiableLazyStringList.java      \
  java/core/src/main/java/com/google/protobuf/UnsafeByteOperations.java            \
  java/core/src/main/java/com/google/protobuf/UnsafeUtil.java                      \
  java/core/src/main/java/com/google/protobuf/Utf8.java                            \
  java/core/src/main/java/com/google/protobuf/WireFormat.java                      \
  java/core/src/test/java/com/google/protobuf/AbstractMessageTest.java             \
  java/core/src/test/java/com/google/protobuf/AnyTest.java                         \
  java/core/src/test/java/com/google/protobuf/BooleanArrayListTest.java            \
  java/core/src/test/java/com/google/protobuf/BoundedByteStringTest.java           \
  java/core/src/test/java/com/google/protobuf/ByteBufferWriterTest.java            \
  java/core/src/test/java/com/google/protobuf/ByteStringTest.java                  \
  java/core/src/test/java/com/google/protobuf/CheckUtf8Test.java                   \
  java/core/src/test/java/com/google/protobuf/CodedInputStreamTest.java            \
  java/core/src/test/java/com/google/protobuf/CodedOutputStreamTest.java           \
  java/core/src/test/java/com/google/protobuf/DeprecatedFieldTest.java             \
  java/core/src/test/java/com/google/protobuf/DescriptorsTest.java                 \
  java/core/src/test/java/com/google/protobuf/DoubleArrayListTest.java             \
  java/core/src/test/java/com/google/protobuf/DynamicMessageTest.java              \
  java/core/src/test/java/com/google/protobuf/EnumTest.java                        \
  java/core/src/test/java/com/google/protobuf/ExtensionRegistryFactoryTest.java    \
  java/core/src/test/java/com/google/protobuf/FieldPresenceTest.java               \
  java/core/src/test/java/com/google/protobuf/FloatArrayListTest.java              \
  java/core/src/test/java/com/google/protobuf/ForceFieldBuildersPreRun.java        \
  java/core/src/test/java/com/google/protobuf/GeneratedMessageTest.java            \
  java/core/src/test/java/com/google/protobuf/IntArrayListTest.java                \
  java/core/src/test/java/com/google/protobuf/IsValidUtf8Test.java                 \
  java/core/src/test/java/com/google/protobuf/IsValidUtf8TestUtil.java             \
  java/core/src/test/java/com/google/protobuf/LazyFieldLiteTest.java               \
  java/core/src/test/java/com/google/protobuf/LazyFieldTest.java                   \
  java/core/src/test/java/com/google/protobuf/LazyMessageLiteTest.java             \
  java/core/src/test/java/com/google/protobuf/LazyStringArrayListTest.java         \
  java/core/src/test/java/com/google/protobuf/LazyStringEndToEndTest.java          \
  java/core/src/test/java/com/google/protobuf/LiteEqualsAndHashTest.java           \
  java/core/src/test/java/com/google/protobuf/LiteTest.java                        \
  java/core/src/test/java/com/google/protobuf/LiteralByteStringTest.java           \
  java/core/src/test/java/com/google/protobuf/LongArrayListTest.java               \
  java/core/src/test/java/com/google/protobuf/MapForProto2LiteTest.java            \
  java/core/src/test/java/com/google/protobuf/MapForProto2Test.java                \
  java/core/src/test/java/com/google/protobuf/MapTest.java                         \
  java/core/src/test/java/com/google/protobuf/MessageTest.java                     \
  java/core/src/test/java/com/google/protobuf/NestedBuildersTest.java              \
  java/core/src/test/java/com/google/protobuf/NioByteStringTest.java               \
  java/core/src/test/java/com/google/protobuf/ParseExceptionsTest.java             \
  java/core/src/test/java/com/google/protobuf/ParserTest.java                      \
  java/core/src/test/java/com/google/protobuf/ProtobufArrayListTest.java           \
  java/core/src/test/java/com/google/protobuf/RepeatedFieldBuilderTest.java        \
  java/core/src/test/java/com/google/protobuf/RepeatedFieldBuilderV3Test.java      \
  java/core/src/test/java/com/google/protobuf/RopeByteStringSubstringTest.java     \
  java/core/src/test/java/com/google/protobuf/RopeByteStringTest.java              \
  java/core/src/test/java/com/google/protobuf/ServiceTest.java                     \
  java/core/src/test/java/com/google/protobuf/SingleFieldBuilderTest.java          \
  java/core/src/test/java/com/google/protobuf/SingleFieldBuilderV3Test.java        \
  java/core/src/test/java/com/google/protobuf/SmallSortedMapTest.java              \
  java/core/src/test/java/com/google/protobuf/TestBadIdentifiers.java              \
  java/core/src/test/java/com/google/protobuf/TestUtil.java                        \
  java/core/src/test/java/com/google/protobuf/TestUtilLite.java                    \
  java/core/src/test/java/com/google/protobuf/TextFormatParseInfoTreeTest.java     \
  java/core/src/test/java/com/google/protobuf/TextFormatParseLocationTest.java     \
  java/core/src/test/java/com/google/protobuf/TextFormatTest.java                  \
  java/core/src/test/java/com/google/protobuf/UnknownEnumValueTest.java            \
  java/core/src/test/java/com/google/protobuf/UnknownFieldSetLiteTest.java         \
  java/core/src/test/java/com/google/protobuf/UnknownFieldSetTest.java             \
  java/core/src/test/java/com/google/protobuf/UnmodifiableLazyStringListTest.java  \
  java/core/src/test/java/com/google/protobuf/WellKnownTypesTest.java              \
  java/core/src/test/java/com/google/protobuf/WireFormatTest.java                  \
  java/core/src/test/proto/com/google/protobuf/any_test.proto                      \
  java/core/src/test/proto/com/google/protobuf/field_presence_test.proto           \
  java/core/src/test/proto/com/google/protobuf/lazy_fields_lite.proto              \
  java/core/src/test/proto/com/google/protobuf/lite_equals_and_hash.proto          \
  java/core/src/test/proto/com/google/protobuf/map_for_proto2_lite_test.proto      \
  java/core/src/test/proto/com/google/protobuf/map_for_proto2_test.proto           \
  java/core/src/test/proto/com/google/protobuf/map_initialization_order_test.proto \
  java/core/src/test/proto/com/google/protobuf/map_test.proto                      \
  java/core/src/test/proto/com/google/protobuf/multiple_files_test.proto           \
  java/core/src/test/proto/com/google/protobuf/nested_builders_test.proto          \
  java/core/src/test/proto/com/google/protobuf/nested_extension.proto              \
  java/core/src/test/proto/com/google/protobuf/nested_extension_lite.proto         \
  java/core/src/test/proto/com/google/protobuf/non_nested_extension.proto          \
  java/core/src/test/proto/com/google/protobuf/non_nested_extension_lite.proto     \
  java/core/src/test/proto/com/google/protobuf/outer_class_name_test.proto         \
  java/core/src/test/proto/com/google/protobuf/outer_class_name_test2.proto        \
  java/core/src/test/proto/com/google/protobuf/outer_class_name_test3.proto        \
  java/core/src/test/proto/com/google/protobuf/test_bad_identifiers.proto          \
  java/core/src/test/proto/com/google/protobuf/test_check_utf8.proto               \
  java/core/src/test/proto/com/google/protobuf/test_check_utf8_size.proto          \
  java/core/src/test/proto/com/google/protobuf/test_custom_options.proto           \
  java/core/src/test/proto/com/google/protobuf/test_extra_interfaces.proto         \
  java/lite/generate-sources-build.xml                                             \
  java/lite/generate-test-sources-build.xml                                        \
  java/lite/pom.xml                                                                \
  java/pom.xml                                                                     \
  java/util/pom.xml                                                                \
  java/util/src/main/java/com/google/protobuf/util/Durations.java                  \
  java/util/src/main/java/com/google/protobuf/util/FieldMaskTree.java              \
  java/util/src/main/java/com/google/protobuf/util/FieldMaskUtil.java              \
  java/util/src/main/java/com/google/protobuf/util/JsonFormat.java                 \
  java/util/src/main/java/com/google/protobuf/util/TimeUtil.java                   \
  java/util/src/main/java/com/google/protobuf/util/Timestamps.java                 \
  java/util/src/test/java/com/google/protobuf/util/FieldMaskTreeTest.java          \
  java/util/src/test/java/com/google/protobuf/util/FieldMaskUtilTest.java          \
  java/util/src/test/java/com/google/protobuf/util/JsonFormatTest.java             \
  java/util/src/test/java/com/google/protobuf/util/TimeUtilTest.java               \
  java/util/src/test/proto/com/google/protobuf/util/json_test.proto

python_EXTRA_DIST=                                                           \
  python/MANIFEST.in                                                         \
  python/google/__init__.py                                                  \
  python/google/protobuf/__init__.py                                         \
  python/google/protobuf/descriptor.py                                       \
  python/google/protobuf/descriptor_database.py                              \
  python/google/protobuf/descriptor_pool.py                                  \
  python/google/protobuf/internal/__init__.py                                \
  python/google/protobuf/internal/_parameterized.py                          \
  python/google/protobuf/internal/any_test.proto                             \
  python/google/protobuf/internal/any_test.proto                             \
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
  python/google/protobuf/internal/file_options_test.proto                    \
  python/google/protobuf/internal/generator_test.py                          \
  python/google/protobuf/internal/import_test_package/__init__.py            \
  python/google/protobuf/internal/import_test_package/inner.proto            \
  python/google/protobuf/internal/import_test_package/outer.proto            \
  python/google/protobuf/internal/json_format_test.py                        \
  python/google/protobuf/internal/message_factory_test.py                    \
  python/google/protobuf/internal/message_listener.py                        \
  python/google/protobuf/internal/message_set_extensions.proto               \
  python/google/protobuf/internal/message_test.py                            \
  python/google/protobuf/internal/missing_enum_values.proto                  \
  python/google/protobuf/internal/more_extensions.proto                      \
  python/google/protobuf/internal/more_extensions_dynamic.proto              \
  python/google/protobuf/internal/more_messages.proto                        \
  python/google/protobuf/internal/packed_field_test.proto                    \
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
  python/google/protobuf/internal/well_known_types.py                        \
  python/google/protobuf/internal/well_known_types.py                        \
  python/google/protobuf/internal/well_known_types_test.py                   \
  python/google/protobuf/internal/well_known_types_test.py                   \
  python/google/protobuf/internal/wire_format.py                             \
  python/google/protobuf/internal/wire_format_test.py                        \
  python/google/protobuf/json_format.py                                      \
  python/google/protobuf/message.py                                          \
  python/google/protobuf/message_factory.py                                  \
  python/google/protobuf/proto_builder.py                                    \
  python/google/protobuf/pyext/README                                        \
  python/google/protobuf/pyext/__init__.py                                   \
  python/google/protobuf/pyext/cpp_message.py                                \
  python/google/protobuf/pyext/descriptor.cc                                 \
  python/google/protobuf/pyext/descriptor.h                                  \
  python/google/protobuf/pyext/descriptor_containers.cc                      \
  python/google/protobuf/pyext/descriptor_containers.h                       \
  python/google/protobuf/pyext/descriptor_database.cc                        \
  python/google/protobuf/pyext/descriptor_database.h                         \
  python/google/protobuf/pyext/descriptor_pool.cc                            \
  python/google/protobuf/pyext/descriptor_pool.h                             \
  python/google/protobuf/pyext/extension_dict.cc                             \
  python/google/protobuf/pyext/extension_dict.h                              \
  python/google/protobuf/pyext/map_container.cc                              \
  python/google/protobuf/pyext/map_container.h                               \
  python/google/protobuf/pyext/message.cc                                    \
  python/google/protobuf/pyext/message.h                                     \
  python/google/protobuf/pyext/message_module.cc                             \
  python/google/protobuf/pyext/proto2_api_test.proto                         \
  python/google/protobuf/pyext/python.proto                                  \
  python/google/protobuf/pyext/python_protobuf.h                             \
  python/google/protobuf/pyext/repeated_composite_container.cc               \
  python/google/protobuf/pyext/repeated_composite_container.h                \
  python/google/protobuf/pyext/repeated_scalar_container.cc                  \
  python/google/protobuf/pyext/repeated_scalar_container.h                   \
  python/google/protobuf/pyext/scoped_pyobject_ptr.h                         \
  python/google/protobuf/reflection.py                                       \
  python/google/protobuf/service.py                                          \
  python/google/protobuf/service_reflection.py                               \
  python/google/protobuf/symbol_database.py                                  \
  python/google/protobuf/text_encoding.py                                    \
  python/google/protobuf/text_format.py                                      \
  python/mox.py                                                              \
  python/setup.py                                                            \
  python/stubout.py                                                          \
  python/tox.ini                                                             \
  python/README.md

all_EXTRA_DIST=$(java_EXTRA_DIST) $(python_EXTRA_DIST)

EXTRA_DIST = $(@DIST_LANG@_EXTRA_DIST)   \
  autogen.sh                             \
  generate_descriptor_proto.sh           \
  README.md                              \
  LICENSE                                \
  CONTRIBUTORS.txt                       \
  CHANGES.txt                            \
  update_file_lists.sh                   \
  BUILD                                  \
  gmock.BUILD                            \
  WORKSPACE                              \
  cmake/CMakeLists.txt                   \
  cmake/README.md                        \
  cmake/examples.cmake                   \
  cmake/extract_includes.bat.in          \
  cmake/install.cmake                    \
  cmake/libprotobuf.cmake                \
  cmake/libprotobuf-lite.cmake           \
  cmake/libprotoc.cmake                  \
  cmake/protobuf-config-version.cmake.in \
  cmake/protobuf-config.cmake.in         \
  cmake/protobuf-module.cmake.in         \
  cmake/protobuf-options.cmake           \
  cmake/protoc.cmake                     \
  cmake/tests.cmake                      \
  editors/README.txt                     \
  editors/proto.vim                      \
  editors/protobuf-mode.el               \
  examples/CMakeLists.txt                \
  examples/README.txt                    \
  examples/Makefile                      \
  examples/addressbook.proto             \
  examples/add_person.cc                 \
  examples/add_person.go                 \
  examples/add_person_test.go            \
  examples/list_people.cc                \
  examples/list_people.go                \
  examples/AddPerson.java                \
  examples/CMakeLists.txt                \
  examples/ListPeople.java               \
  examples/add_person.py                 \
  examples/list_people.py                \
  examples/list_people_test.go           \
  protobuf.bzl                           \
  six.BUILD                              \
  util/python/BUILD

# Deletes all the files generated by autogen.sh.
MAINTAINERCLEANFILES =   \
  aclocal.m4             \
  ar-lib                 \
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
