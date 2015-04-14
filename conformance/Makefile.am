## Process this file with automake to produce Makefile.in

protoc_inputs =                                                \
  conformance.proto

protoc_outputs =                                               \
  conformance.pb.cc                                            \
  conformance.pb.h

bin_PROGRAMS = conformance-test-runner conformance-cpp

conformance_test_runner_LDADD = $(top_srcdir)/src/libprotobuf.la
conformance_test_runner_SOURCES = conformance_test.cc conformance_test_runner.cc
nodist_conformance_test_runner_SOURCES = conformance.pb.cc
conformance_test_runner_CPPFLAGS = -I$(top_srcdir)/src

conformance_cpp_LDADD = $(top_srcdir)/src/libprotobuf.la
conformance_cpp_SOURCES = conformance_cpp.cc
nodist_conformance_cpp_SOURCES = conformance.pb.cc
conformance_cpp_CPPFLAGS = -I$(top_srcdir)/src

if USE_EXTERNAL_PROTOC

unittest_proto_middleman: $(protoc_inputs)
	$(PROTOC) -I$(srcdir) --cpp_out=. $^
	touch unittest_proto_middleman

else

# We have to cd to $(srcdir) before executing protoc because $(protoc_inputs) is
# relative to srcdir, which may not be the same as the current directory when
# building out-of-tree.
unittest_proto_middleman: $(top_srcdir)/src/protoc$(EXEEXT) $(protoc_inputs)
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. --cpp_out=$$oldpwd $(protoc_inputs) )
	touch unittest_proto_middleman

endif

$(protoc_outputs): unittest_proto_middleman

BUILT_SOURCES = $(protoc_outputs)

CLEANFILES = $(protoc_outputs) unittest_proto_middleman

MAINTAINERCLEANFILES =   \
  Makefile.in

# Targets for actually running tests.
test_cpp: unittest_proto_middleman conformance-test-runner conformance-cpp
	./conformance-test-runner ./conformance-cpp
