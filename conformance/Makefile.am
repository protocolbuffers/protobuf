## Process this file with automake to produce Makefile.in

conformance_protoc_inputs =                                    \
  conformance.proto

well_known_type_protoc_inputs =                                \
  $(top_srcdir)/src/google/protobuf/any.proto                  \
  $(top_srcdir)/src/google/protobuf/duration.proto             \
  $(top_srcdir)/src/google/protobuf/field_mask.proto           \
  $(top_srcdir)/src/google/protobuf/struct.proto               \
  $(top_srcdir)/src/google/protobuf/timestamp.proto            \
  $(top_srcdir)/src/google/protobuf/wrappers.proto


protoc_outputs =                                               \
  conformance.pb.cc                                            \
  conformance.pb.h

other_language_protoc_outputs =                                \
  conformance_pb2.py                                           \
  com/google/protobuf/Any.java                                 \
  com/google/protobuf/AnyOrBuilder.java                        \
  com/google/protobuf/AnyProto.java                            \
  com/google/protobuf/BoolValue.java                           \
  com/google/protobuf/BoolValueOrBuilder.java                  \
  com/google/protobuf/BytesValue.java                          \
  com/google/protobuf/BytesValueOrBuilder.java                 \
  com/google/protobuf/conformance/Conformance.java             \
  com/google/protobuf/DoubleValue.java                         \
  com/google/protobuf/DoubleValueOrBuilder.java                \
  com/google/protobuf/Duration.java                            \
  com/google/protobuf/DurationOrBuilder.java                   \
  com/google/protobuf/DurationProto.java                       \
  com/google/protobuf/FieldMask.java                           \
  com/google/protobuf/FieldMaskOrBuilder.java                  \
  com/google/protobuf/FieldMaskProto.java                      \
  com/google/protobuf/FloatValue.java                          \
  com/google/protobuf/FloatValueOrBuilder.java                 \
  com/google/protobuf/Int32Value.java                          \
  com/google/protobuf/Int32ValueOrBuilder.java                 \
  com/google/protobuf/Int64Value.java                          \
  com/google/protobuf/Int64ValueOrBuilder.java                 \
  com/google/protobuf/ListValue.java                           \
  com/google/protobuf/ListValueOrBuilder.java                  \
  com/google/protobuf/NullValue.java                           \
  com/google/protobuf/StringValue.java                         \
  com/google/protobuf/StringValueOrBuilder.java                \
  com/google/protobuf/Struct.java                              \
  com/google/protobuf/StructOrBuilder.java                     \
  com/google/protobuf/StructProto.java                         \
  com/google/protobuf/Timestamp.java                           \
  com/google/protobuf/TimestampOrBuilder.java                  \
  com/google/protobuf/TimestampProto.java                      \
  com/google/protobuf/UInt32Value.java                         \
  com/google/protobuf/UInt32ValueOrBuilder.java                \
  com/google/protobuf/UInt64Value.java                         \
  com/google/protobuf/UInt64ValueOrBuilder.java                \
  com/google/protobuf/Value.java                               \
  com/google/protobuf/ValueOrBuilder.java                      \
  com/google/protobuf/WrappersProto.java                       \
  google/protobuf/any.pb.cc                                    \
  google/protobuf/any.pb.h                                     \
  google/protobuf/any_pb2.py                                   \
  google/protobuf/duration.pb.cc                               \
  google/protobuf/duration.pb.h                                \
  google/protobuf/duration_pb2.py                              \
  google/protobuf/field_mask.pb.cc                             \
  google/protobuf/field_mask.pb.h                              \
  google/protobuf/field_mask_pb2.py                            \
  google/protobuf/struct.pb.cc                                 \
  google/protobuf/struct.pb.h                                  \
  google/protobuf/struct_pb2.py                                \
  google/protobuf/timestamp.pb.cc                              \
  google/protobuf/timestamp.pb.h                               \
  google/protobuf/timestamp_pb2.py                             \
  google/protobuf/wrappers.pb.cc                               \
  google/protobuf/wrappers.pb.h                                \
  google/protobuf/wrappers_pb2.py

bin_PROGRAMS = conformance-test-runner conformance-cpp

# All source files excepet C++/Objective-C ones should be explicitly listed
# here because the autoconf tools don't include files of other languages
# automatically.
EXTRA_DIST =                  \
  ConformanceJava.java        \
  README.md                   \
  conformance.proto           \
  conformance_python.py       \
  failure_list_cpp.txt        \
  failure_list_java.txt       \
  failure_list_python.txt     \
  failure_list_python_cpp.txt \
  failure_list_python-post26.txt

conformance_test_runner_LDADD = $(top_srcdir)/src/libprotobuf.la
conformance_test_runner_SOURCES = conformance_test.h conformance_test.cc \
                                  conformance_test_runner.cc             \
                                  third_party/jsoncpp/json.h             \
                                  third_party/jsoncpp/jsoncpp.cpp
nodist_conformance_test_runner_SOURCES = conformance.pb.cc
conformance_test_runner_CPPFLAGS = -I$(top_srcdir)/src -I$(srcdir)
conformance_test_runner_CXXFLAGS = -std=c++11
# Explicit deps beacuse BUILT_SOURCES are only done before a "make all/check"
# so a direct "make test_cpp" could fail if parallel enough.
conformance_test_runner-conformance_test.$(OBJEXT): conformance.pb.h
conformance_test_runner-conformance_test_runner.$(OBJEXT): conformance.pb.h

conformance_cpp_LDADD = $(top_srcdir)/src/libprotobuf.la
conformance_cpp_SOURCES = conformance_cpp.cc
nodist_conformance_cpp_SOURCES = conformance.pb.cc
conformance_cpp_CPPFLAGS = -I$(top_srcdir)/src
# Explicit dep beacuse BUILT_SOURCES are only done before a "make all/check"
# so a direct "make test_cpp" could fail if parallel enough.
conformance_cpp-conformance_cpp.$(OBJEXT): conformance.pb.h

if USE_EXTERNAL_PROTOC

# Some implementations include pre-generated versions of well-known types.
protoc_middleman: $(conformance_protoc_inputs) $(well_known_type_protoc_inputs)
	$(PROTOC) -I$(srcdir) -I$(top_srcdir) --cpp_out=. --java_out=. --python_out=. $(conformance_protoc_inputs) $(well_known_type_protoc_inputs)
	touch protoc_middleman

else

# We have to cd to $(srcdir) before executing protoc because $(protoc_inputs) is
# relative to srcdir, which may not be the same as the current directory when
# building out-of-tree.
protoc_middleman: $(top_srcdir)/src/protoc$(EXEEXT) $(conformance_protoc_inputs) $(well_known_type_protoc_inputs)
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. -I$(top_srcdir)/src --cpp_out=$$oldpwd --java_out=$$oldpwd --ruby_out=$$oldpwd --objc_out=$$oldpwd --python_out=$$oldpwd $(conformance_protoc_inputs) )
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. -I$(top_srcdir)/src --cpp_out=$$oldpwd --java_out=$$oldpwd --ruby_out=$$oldpwd --python_out=$$oldpwd $(well_known_type_protoc_inputs) )
	## @mkdir -p lite
	## oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. -I$(top_srcdir)/src --java_out=lite:$$oldpwd/lite $(conformance_protoc_inputs) $(well_known_type_protoc_inputs) )
	touch protoc_middleman

endif

$(protoc_outputs): protoc_middleman

$(other_language_protoc_outputs): protoc_middleman

BUILT_SOURCES = $(protoc_outputs) $(other_language_protoc_outputs)

CLEANFILES = $(protoc_outputs) protoc_middleman javac_middleman conformance-java $(other_language_protoc_outputs)

MAINTAINERCLEANFILES =   \
  Makefile.in

javac_middleman: ConformanceJava.java protoc_middleman $(other_language_protoc_outputs)
	jar=`ls ../java/util/target/*jar-with-dependencies.jar` && javac -classpath ../java/target/classes:$$jar ConformanceJava.java com/google/protobuf/conformance/Conformance.java
	@touch javac_middleman

conformance-java: javac_middleman
	@echo "Writing shortcut script conformance-java..."
	@echo '#! /bin/sh' > conformance-java
	@jar=`ls ../java/util/target/*jar-with-dependencies.jar` && echo java -classpath .:../java/target/classes:$$jar ConformanceJava '$$@' >> conformance-java
	@chmod +x conformance-java

# Targets for actually running tests.
test_cpp: protoc_middleman conformance-test-runner conformance-cpp
	./conformance-test-runner --failure_list failure_list_cpp.txt ./conformance-cpp

test_java: protoc_middleman conformance-test-runner conformance-java
	./conformance-test-runner --failure_list failure_list_java.txt ./conformance-java

# These depend on library paths being properly set up.  The easiest way to
# run them is to just use "tox" from the python dir.
test_python: protoc_middleman conformance-test-runner
	./conformance-test-runner --failure_list failure_list_python.txt ./conformance_python.py

test_python_cpp: protoc_middleman conformance-test-runner
	./conformance-test-runner --failure_list failure_list_python_cpp.txt ./conformance_python.py
