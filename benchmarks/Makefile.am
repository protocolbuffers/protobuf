
benchmarks_protoc_inputs =                                     \
  benchmarks.proto                                             \
  benchmark_messages_proto3.proto

benchmarks_protoc_inputs_proto2 =                              \
  benchmark_messages_proto2.proto

benchmarks_protoc_outputs =                                    \
  benchmarks.pb.cc                                             \
  benchmarks.pb.h                                              \
  benchmark_messages_proto3.pb.cc                              \
  benchmark_messages_proto3.pb.h

benchmarks_protoc_outputs_proto2 =                             \
  benchmark_messages_proto2.pb.cc                              \
  benchmark_messages_proto2.pb.h

bin_PROGRAMS = generate-datasets

generate_datasets_LDADD = $(top_srcdir)/src/libprotobuf.la
generate_datasets_SOURCES = generate_datasets.cc
generate_datasets_CPPFLAGS = -I$(top_srcdir)/src -I$(srcdir)
nodist_generate_datasets_SOURCES =                             \
  $(benchmarks_protoc_outputs)                                 \
  $(benchmarks_protoc_outputs_proto2)

# Explicit deps because BUILT_SOURCES are only done before a "make all/check"
# so a direct "make test_cpp" could fail if parallel enough.
# See: https://www.gnu.org/software/automake/manual/html_node/Built-Sources-Example.html#Recording-Dependencies-manually
generate_datasets-generate_datasets.$(OBJEXT): benchmarks.pb.h

$(benchmarks_protoc_outputs): protoc_middleman
$(benchmarks_protoc_outputs_proto2): protoc_middleman2

CLEANFILES =                                                   \
  $(benchmarks_protoc_outputs)                                 \
  $(benchmarks_protoc_outputs_proto2)                          \
  protoc_middleman                                             \
  protoc_middleman2                                            \
  dataset.*

MAINTAINERCLEANFILES =   \
  Makefile.in

if USE_EXTERNAL_PROTOC

protoc_middleman: $(benchmarks_protoc_inputs)
	$(PROTOC) -I$(srcdir) -I$(top_srcdir) --cpp_out=. $(benchmarks_protoc_inputs)
	touch protoc_middleman

protoc_middleman2: $(benchmarks_protoc_inputs_proto2)
	$(PROTOC) -I$(srcdir) -I$(top_srcdir) --cpp_out=. $(benchmarks_protoc_inputs_proto2)
	touch protoc_middleman2

else

# We have to cd to $(srcdir) before executing protoc because $(protoc_inputs) is
# relative to srcdir, which may not be the same as the current directory when
# building out-of-tree.
protoc_middleman: $(top_srcdir)/src/protoc$(EXEEXT) $(benchmarks_protoc_inputs) $(well_known_type_protoc_inputs)
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. -I$(top_srcdir)/src --cpp_out=$$oldpwd $(benchmarks_protoc_inputs) )
	touch protoc_middleman

protoc_middleman2: $(top_srcdir)/src/protoc$(EXEEXT) $(benchmarks_protoc_inputs_proto2) $(well_known_type_protoc_inputs)
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. -I$(top_srcdir)/src --cpp_out=$$oldpwd $(benchmarks_protoc_inputs_proto2) )
	touch protoc_middleman

endif
