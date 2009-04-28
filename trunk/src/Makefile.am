## Process this file with automake to produce Makefile.in

if HAVE_ZLIB
GZCHECKPROGRAMS = zcgzip zcgunzip
GZHEADERS = google/protobuf/io/gzip_stream.h
GZTESTS = google/protobuf/io/gzip_stream_unittest.sh
else
GZCHECKPROGRAMS =
GZHEADERS =
GZTESTS =
endif

if GCC
# These are good warnings to turn on by default
AM_CXXFLAGS = $(PTHREAD_CFLAGS) -Wall -Wwrite-strings -Woverloaded-virtual -Wno-sign-compare
else
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
endif

AM_LDFLAGS = $(PTHREAD_CFLAGS)

# If I say "dist_include_DATA", automake complains that $(includedir) is not
# a "legitimate" directory for DATA.  Screw you, automake.
protodir = $(includedir)
nobase_dist_proto_DATA = google/protobuf/descriptor.proto

# Not sure why these don't get cleaned automatically.
clean-local:
	rm -f *.loT

CLEANFILES = $(protoc_outputs) unittest_proto_middleman

MAINTAINERCLEANFILES =   \
  Makefile.in

nobase_include_HEADERS =                                       \
  google/protobuf/stubs/common.h                               \
  google/protobuf/stubs/once.h                                 \
  google/protobuf/descriptor.h                                 \
  google/protobuf/descriptor.pb.h                              \
  google/protobuf/descriptor_database.h                        \
  google/protobuf/dynamic_message.h                            \
  google/protobuf/extension_set.h                              \
  google/protobuf/generated_message_reflection.h               \
  google/protobuf/message.h                                    \
  google/protobuf/reflection_ops.h                             \
  google/protobuf/repeated_field.h                             \
  google/protobuf/service.h                                    \
  google/protobuf/text_format.h                                \
  google/protobuf/unknown_field_set.h                          \
  google/protobuf/wire_format.h                                \
  google/protobuf/wire_format_inl.h                            \
  google/protobuf/io/coded_stream.h                            \
  $(GZHEADERS)                                                 \
  google/protobuf/io/printer.h                                 \
  google/protobuf/io/tokenizer.h                               \
  google/protobuf/io/zero_copy_stream.h                        \
  google/protobuf/io/zero_copy_stream_impl.h                   \
  google/protobuf/compiler/code_generator.h                    \
  google/protobuf/compiler/command_line_interface.h            \
  google/protobuf/compiler/importer.h                          \
  google/protobuf/compiler/parser.h                            \
  google/protobuf/compiler/cpp/cpp_generator.h                 \
  google/protobuf/compiler/java/java_generator.h               \
  google/protobuf/compiler/python/python_generator.h

lib_LTLIBRARIES = libprotobuf.la libprotoc.la

libprotobuf_la_LIBADD = $(PTHREAD_LIBS)
libprotobuf_la_LDFLAGS = -version-info 3:0:0
libprotobuf_la_SOURCES =                                       \
  google/protobuf/stubs/common.cc                              \
  google/protobuf/stubs/once.cc                                \
  google/protobuf/stubs/hash.cc                                \
  google/protobuf/stubs/hash.h                                 \
  google/protobuf/stubs/map-util.cc                            \
  google/protobuf/stubs/map-util.h                             \
  google/protobuf/stubs/stl_util-inl.cc                        \
  google/protobuf/stubs/stl_util-inl.h                         \
  google/protobuf/stubs/substitute.cc                          \
  google/protobuf/stubs/substitute.h                           \
  google/protobuf/stubs/strutil.cc                             \
  google/protobuf/stubs/strutil.h                              \
  google/protobuf/stubs/structurally_valid.cc                  \
  google/protobuf/descriptor.cc                                \
  google/protobuf/descriptor.pb.cc                             \
  google/protobuf/descriptor_database.cc                       \
  google/protobuf/dynamic_message.cc                           \
  google/protobuf/extension_set.cc                             \
  google/protobuf/generated_message_reflection.cc              \
  google/protobuf/message.cc                                   \
  google/protobuf/reflection_ops.cc                            \
  google/protobuf/repeated_field.cc                            \
  google/protobuf/service.cc                                   \
  google/protobuf/text_format.cc                               \
  google/protobuf/unknown_field_set.cc                         \
  google/protobuf/wire_format.cc                               \
  google/protobuf/io/coded_stream.cc                           \
  google/protobuf/io/gzip_stream.cc                            \
  google/protobuf/io/printer.cc                                \
  google/protobuf/io/tokenizer.cc                              \
  google/protobuf/io/zero_copy_stream.cc                       \
  google/protobuf/io/zero_copy_stream_impl.cc                  \
  google/protobuf/compiler/importer.cc                         \
  google/protobuf/compiler/parser.cc

libprotoc_la_LIBADD = $(PTHREAD_LIBS) libprotobuf.la
libprotoc_la_LDFLAGS = -version-info 3:0:0
libprotoc_la_SOURCES =                                         \
  google/protobuf/compiler/code_generator.cc                   \
  google/protobuf/compiler/command_line_interface.cc           \
  google/protobuf/compiler/cpp/cpp_enum.cc                     \
  google/protobuf/compiler/cpp/cpp_enum.h                      \
  google/protobuf/compiler/cpp/cpp_enum_field.cc               \
  google/protobuf/compiler/cpp/cpp_enum_field.h                \
  google/protobuf/compiler/cpp/cpp_extension.cc                \
  google/protobuf/compiler/cpp/cpp_extension.h                 \
  google/protobuf/compiler/cpp/cpp_field.cc                    \
  google/protobuf/compiler/cpp/cpp_field.h                     \
  google/protobuf/compiler/cpp/cpp_file.cc                     \
  google/protobuf/compiler/cpp/cpp_file.h                      \
  google/protobuf/compiler/cpp/cpp_generator.cc                \
  google/protobuf/compiler/cpp/cpp_helpers.cc                  \
  google/protobuf/compiler/cpp/cpp_helpers.h                   \
  google/protobuf/compiler/cpp/cpp_message.cc                  \
  google/protobuf/compiler/cpp/cpp_message.h                   \
  google/protobuf/compiler/cpp/cpp_message_field.cc            \
  google/protobuf/compiler/cpp/cpp_message_field.h             \
  google/protobuf/compiler/cpp/cpp_primitive_field.cc          \
  google/protobuf/compiler/cpp/cpp_primitive_field.h           \
  google/protobuf/compiler/cpp/cpp_service.cc                  \
  google/protobuf/compiler/cpp/cpp_service.h                   \
  google/protobuf/compiler/cpp/cpp_string_field.cc             \
  google/protobuf/compiler/cpp/cpp_string_field.h              \
  google/protobuf/compiler/java/java_enum.cc                   \
  google/protobuf/compiler/java/java_enum.h                    \
  google/protobuf/compiler/java/java_enum_field.cc             \
  google/protobuf/compiler/java/java_enum_field.h              \
  google/protobuf/compiler/java/java_extension.cc              \
  google/protobuf/compiler/java/java_extension.h               \
  google/protobuf/compiler/java/java_field.cc                  \
  google/protobuf/compiler/java/java_field.h                   \
  google/protobuf/compiler/java/java_file.cc                   \
  google/protobuf/compiler/java/java_file.h                    \
  google/protobuf/compiler/java/java_generator.cc              \
  google/protobuf/compiler/java/java_helpers.cc                \
  google/protobuf/compiler/java/java_helpers.h                 \
  google/protobuf/compiler/java/java_message.cc                \
  google/protobuf/compiler/java/java_message.h                 \
  google/protobuf/compiler/java/java_message_field.cc          \
  google/protobuf/compiler/java/java_message_field.h           \
  google/protobuf/compiler/java/java_primitive_field.cc        \
  google/protobuf/compiler/java/java_primitive_field.h         \
  google/protobuf/compiler/java/java_service.cc                \
  google/protobuf/compiler/java/java_service.h                 \
  google/protobuf/compiler/python/python_generator.cc

bin_PROGRAMS = protoc
protoc_LDADD = $(PTHREAD_LIBS) libprotobuf.la libprotoc.la
protoc_SOURCES = google/protobuf/compiler/main.cc

# Tests ==============================================================

protoc_inputs =                                                \
  google/protobuf/unittest.proto                               \
  google/protobuf/unittest_empty.proto                         \
  google/protobuf/unittest_import.proto                        \
  google/protobuf/unittest_mset.proto                          \
  google/protobuf/unittest_optimize_for.proto                  \
  google/protobuf/unittest_embed_optimize_for.proto            \
  google/protobuf/unittest_custom_options.proto                \
  google/protobuf/compiler/cpp/cpp_test_bad_identifiers.proto

EXTRA_DIST =                                                   \
  $(protoc_inputs)                                             \
  solaris/libstdc++.la                                         \
  google/protobuf/io/gzip_stream.h                             \
  google/protobuf/io/gzip_stream_unittest.sh                   \
  google/protobuf/testdata/golden_message                      \
  google/protobuf/testdata/golden_packed_fields_message        \
  google/protobuf/testdata/text_format_unittest_data.txt       \
  google/protobuf/testdata/text_format_unittest_extensions_data.txt  \
  google/protobuf/package_info.h                               \
  google/protobuf/io/package_info.h                            \
  google/protobuf/compiler/package_info.h

protoc_outputs =                                               \
  google/protobuf/unittest.pb.cc                               \
  google/protobuf/unittest.pb.h                                \
  google/protobuf/unittest_empty.pb.cc                         \
  google/protobuf/unittest_empty.pb.h                          \
  google/protobuf/unittest_import.pb.cc                        \
  google/protobuf/unittest_import.pb.h                         \
  google/protobuf/unittest_mset.pb.cc                          \
  google/protobuf/unittest_mset.pb.h                           \
  google/protobuf/unittest_optimize_for.pb.cc                  \
  google/protobuf/unittest_optimize_for.pb.h                   \
  google/protobuf/unittest_embed_optimize_for.pb.cc            \
  google/protobuf/unittest_embed_optimize_for.pb.h             \
  google/protobuf/unittest_custom_options.pb.cc                \
  google/protobuf/unittest_custom_options.pb.h                 \
  google/protobuf/compiler/cpp/cpp_test_bad_identifiers.pb.cc  \
  google/protobuf/compiler/cpp/cpp_test_bad_identifiers.pb.h

BUILT_SOURCES = $(protoc_outputs)

# This rule is a little weird.  The first prereq is the protoc executable
# and the rest are its inputs.  Therefore, $^ -- which expands to the
# list of prereqs -- is actually a valid command.  We have to place "./" in
# front of it in case protoc is in the current directory.  protoc allows
# flags to appear after input file names, so we happily stick the flags on
# the end.
#
# For reference, if we didn't have to worry about VPATH (i.e., building from
# a directory other than the package root), we could have just written this:
#   ./protoc$(EXEEXT) -I$(srcdir) --cpp_out=. $(protoc_inputs)
unittest_proto_middleman: protoc$(EXEEXT) $(protoc_inputs)
	./$^ -I$(srcdir) --cpp_out=.
	touch unittest_proto_middleman

$(protoc_outputs): unittest_proto_middleman

COMMON_TEST_SOURCES =                                          \
  google/protobuf/test_util.cc                                 \
  google/protobuf/test_util.h                                  \
  google/protobuf/testing/googletest.cc                        \
  google/protobuf/testing/googletest.h                         \
  google/protobuf/testing/file.cc                              \
  google/protobuf/testing/file.h

check_PROGRAMS = protobuf-test protobuf-lazy-descriptor-test $(GZCHECKPROGRAMS)
protobuf_test_LDADD = $(PTHREAD_LIBS) libprotobuf.la libprotoc.la \
                      $(top_builddir)/gtest/lib/libgtest.la       \
                      $(top_builddir)/gtest/lib/libgtest_main.la
protobuf_test_CPPFLAGS = -I$(top_srcdir)/gtest/include         \
                         -I$(top_builddir)/gtest/include
protobuf_test_SOURCES =                                        \
  google/protobuf/stubs/common_unittest.cc                     \
  google/protobuf/stubs/once_unittest.cc                       \
  google/protobuf/stubs/strutil_unittest.cc                    \
  google/protobuf/stubs/structurally_valid_unittest.cc         \
  google/protobuf/descriptor_database_unittest.cc              \
  google/protobuf/descriptor_unittest.cc                       \
  google/protobuf/dynamic_message_unittest.cc                  \
  google/protobuf/extension_set_unittest.cc                    \
  google/protobuf/generated_message_reflection_unittest.cc     \
  google/protobuf/message_unittest.cc                          \
  google/protobuf/reflection_ops_unittest.cc                   \
  google/protobuf/repeated_field_unittest.cc                   \
  google/protobuf/text_format_unittest.cc                      \
  google/protobuf/unknown_field_set_unittest.cc                \
  google/protobuf/wire_format_unittest.cc                      \
  google/protobuf/io/coded_stream_unittest.cc                  \
  google/protobuf/io/printer_unittest.cc                       \
  google/protobuf/io/tokenizer_unittest.cc                     \
  google/protobuf/io/zero_copy_stream_unittest.cc              \
  google/protobuf/compiler/command_line_interface_unittest.cc  \
  google/protobuf/compiler/importer_unittest.cc                \
  google/protobuf/compiler/parser_unittest.cc                  \
  google/protobuf/compiler/cpp/cpp_bootstrap_unittest.cc       \
  google/protobuf/compiler/cpp/cpp_unittest.cc                 \
  $(COMMON_TEST_SOURCES)
nodist_protobuf_test_SOURCES = $(protoc_outputs)

# Run cpp_unittest again with PROTOBUF_TEST_NO_DESCRIPTORS defined.
protobuf_lazy_descriptor_test_LDADD = $(PTHREAD_LIBS) libprotobuf.la \
                      $(top_builddir)/gtest/lib/libgtest.la       \
                      $(top_builddir)/gtest/lib/libgtest_main.la
protobuf_lazy_descriptor_test_CPPFLAGS = -I$(top_srcdir)/gtest/include    \
                                         -I$(top_builddir)/gtest/include  \
                                         -DPROTOBUF_TEST_NO_DESCRIPTORS
protobuf_lazy_descriptor_test_SOURCES =                        \
  google/protobuf/compiler/cpp/cpp_unittest.cc                 \
  $(COMMON_TEST_SOURCES)
nodist_protobuf_lazy_descriptor_test_SOURCES = $(protoc_outputs)

if HAVE_ZLIB
zcgzip_LDADD = $(PTHREAD_LIBS) libprotobuf.la
zcgzip_SOURCES = google/protobuf/testing/zcgzip.cc

zcgunzip_LDADD = $(PTHREAD_LIBS) libprotobuf.la
zcgunzip_SOURCES = google/protobuf/testing/zcgunzip.cc
endif

TESTS = protobuf-test protobuf-lazy-descriptor-test $(GZTESTS)
