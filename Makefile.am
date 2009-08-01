## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign

# Build . before src so that our all-local and clean-local hooks kicks in at
# the right time.
SUBDIRS = . src

# Always include gtest in distributions.
DIST_SUBDIRS = $(subdirs) src

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
	fi

EXTRA_DIST =                                                                 \
  autogen.sh                                                                 \
  generate_descriptor_proto.sh                                               \
  README.txt                                                                 \
  INSTALL.txt                                                                \
  COPYING.txt                                                                \
  CONTRIBUTORS.txt                                                           \
  CHANGES.txt                                                                \
  editors/README.txt                                                         \
  editors/proto.vim                                                          \
  editors/protobuf-mode.el                                                   \
  vsprojects/config.h                                                        \
  vsprojects/extract_includes.bat                                            \
  vsprojects/libprotobuf.vcproj                                              \
  vsprojects/libprotobuf-lite.vcproj                                         \
  vsprojects/libprotoc.vcproj                                                \
  vsprojects/protobuf.sln                                                    \
  vsprojects/protoc.vcproj                                                   \
  vsprojects/readme.txt                                                      \
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
  examples/list_people.py                                                    \
  java/src/main/java/com/google/protobuf/AbstractMessage.java                \
  java/src/main/java/com/google/protobuf/AbstractMessageLite.java            \
  java/src/main/java/com/google/protobuf/BlockingRpcChannel.java             \
  java/src/main/java/com/google/protobuf/BlockingService.java                \
  java/src/main/java/com/google/protobuf/ByteString.java                     \
  java/src/main/java/com/google/protobuf/CodedInputStream.java               \
  java/src/main/java/com/google/protobuf/CodedOutputStream.java              \
  java/src/main/java/com/google/protobuf/Descriptors.java                    \
  java/src/main/java/com/google/protobuf/DynamicMessage.java                 \
  java/src/main/java/com/google/protobuf/ExtensionRegistry.java              \
  java/src/main/java/com/google/protobuf/ExtensionRegistryLite.java          \
  java/src/main/java/com/google/protobuf/FieldSet.java                       \
  java/src/main/java/com/google/protobuf/GeneratedMessage.java               \
  java/src/main/java/com/google/protobuf/GeneratedMessageLite.java           \
  java/src/main/java/com/google/protobuf/Internal.java                       \
  java/src/main/java/com/google/protobuf/InvalidProtocolBufferException.java \
  java/src/main/java/com/google/protobuf/Message.java                        \
  java/src/main/java/com/google/protobuf/MessageLite.java                    \
  java/src/main/java/com/google/protobuf/ProtocolMessageEnum.java            \
  java/src/main/java/com/google/protobuf/RpcCallback.java                    \
  java/src/main/java/com/google/protobuf/RpcChannel.java                     \
  java/src/main/java/com/google/protobuf/RpcController.java                  \
  java/src/main/java/com/google/protobuf/RpcUtil.java                        \
  java/src/main/java/com/google/protobuf/Service.java                        \
  java/src/main/java/com/google/protobuf/ServiceException.java               \
  java/src/main/java/com/google/protobuf/TextFormat.java                     \
  java/src/main/java/com/google/protobuf/UninitializedMessageException.java  \
  java/src/main/java/com/google/protobuf/UnknownFieldSet.java                \
  java/src/main/java/com/google/protobuf/WireFormat.java                     \
  java/src/test/java/com/google/protobuf/AbstractMessageTest.java            \
  java/src/test/java/com/google/protobuf/CodedInputStreamTest.java           \
  java/src/test/java/com/google/protobuf/CodedOutputStreamTest.java          \
  java/src/test/java/com/google/protobuf/DescriptorsTest.java                \
  java/src/test/java/com/google/protobuf/DynamicMessageTest.java             \
  java/src/test/java/com/google/protobuf/GeneratedMessageTest.java           \
  java/src/test/java/com/google/protobuf/LiteTest.java                       \
  java/src/test/java/com/google/protobuf/MessageTest.java                    \
  java/src/test/java/com/google/protobuf/ServiceTest.java                    \
  java/src/test/java/com/google/protobuf/TestUtil.java                       \
  java/src/test/java/com/google/protobuf/TextFormatTest.java                 \
  java/src/test/java/com/google/protobuf/UnknownFieldSetTest.java            \
  java/src/test/java/com/google/protobuf/WireFormatTest.java                 \
  java/src/test/java/com/google/protobuf/multiple_files_test.proto           \
  java/pom.xml                                                               \
  java/README.txt                                                            \
  python/google/protobuf/internal/generator_test.py                          \
  python/google/protobuf/internal/containers.py                              \
  python/google/protobuf/internal/decoder.py                                 \
  python/google/protobuf/internal/decoder_test.py                            \
  python/google/protobuf/internal/descriptor_test.py                         \
  python/google/protobuf/internal/encoder.py                                 \
  python/google/protobuf/internal/encoder_test.py                            \
  python/google/protobuf/internal/input_stream.py                            \
  python/google/protobuf/internal/input_stream_test.py                       \
  python/google/protobuf/internal/message_listener.py                        \
  python/google/protobuf/internal/message_test.py                            \
  python/google/protobuf/internal/more_extensions.proto                      \
  python/google/protobuf/internal/more_messages.proto                        \
  python/google/protobuf/internal/output_stream.py                           \
  python/google/protobuf/internal/output_stream_test.py                      \
  python/google/protobuf/internal/reflection_test.py                         \
  python/google/protobuf/internal/service_reflection_test.py                 \
  python/google/protobuf/internal/test_util.py                               \
  python/google/protobuf/internal/text_format_test.py                        \
  python/google/protobuf/internal/type_checkers.py                           \
  python/google/protobuf/internal/wire_format.py                             \
  python/google/protobuf/internal/wire_format_test.py                        \
  python/google/protobuf/internal/__init__.py                                \
  python/google/protobuf/descriptor.py                                       \
  python/google/protobuf/message.py                                          \
  python/google/protobuf/reflection.py                                       \
  python/google/protobuf/service.py                                          \
  python/google/protobuf/service_reflection.py                               \
  python/google/protobuf/text_format.py                                      \
  python/google/protobuf/__init__.py                                         \
  python/google/__init__.py                                                  \
  python/ez_setup.py                                                         \
  python/setup.py                                                            \
  python/mox.py                                                              \
  python/stubout.py                                                          \
  python/README.txt

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
