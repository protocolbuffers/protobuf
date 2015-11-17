## Process this file with automake to produce Makefile.in

protoc_inputs =                                                \
  conformance.proto

protoc_outputs =                                               \
  conformance.pb.cc                                            \
  conformance.pb.h

other_language_protoc_outputs =                                \
  conformance.rb                                               \
  com/google/protobuf/conformance/Conformance.java             \
  Conformance.pbobjc.h                                         \
  Conformance.pbobjc.m

bin_PROGRAMS = conformance-test-runner conformance-cpp

conformance_test_runner_LDADD = $(top_srcdir)/src/libprotobuf.la
conformance_test_runner_SOURCES = conformance_test.cc conformance_test_runner.cc
nodist_conformance_test_runner_SOURCES = conformance.pb.cc
conformance_test_runner_CPPFLAGS = -I$(top_srcdir)/src
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

if OBJC_CONFORMANCE_TEST

bin_PROGRAMS += conformance-objc

conformance_objc_SOURCES = conformance_objc.m ../objectivec/GPBProtocolBuffers.m
nodist_conformance_objc_SOURCES = Conformance.pbobjc.m
conformance_objc_CPPFLAGS = -I$(top_srcdir)/objectivec
conformance_objc_LDFLAGS = -framework Foundation
# Explicit dep beacuse BUILT_SOURCES are only done before a "make all/check"
# so a direct "make test_objc" could fail if parallel enough.
conformance_objc-conformance_objc.$(OBJEXT): Conformance.pbobjc.h

endif

if USE_EXTERNAL_PROTOC

protoc_middleman: $(protoc_inputs)
	$(PROTOC) -I$(srcdir) --cpp_out=. --java_out=. --ruby_out=. --objc_out=. $^
	touch protoc_middleman

else

# We have to cd to $(srcdir) before executing protoc because $(protoc_inputs) is
# relative to srcdir, which may not be the same as the current directory when
# building out-of-tree.
protoc_middleman: $(top_srcdir)/src/protoc$(EXEEXT) $(protoc_inputs)
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. --cpp_out=$$oldpwd --java_out=$$oldpwd --ruby_out=$$oldpwd --objc_out=$$oldpwd $(protoc_inputs) )
	touch protoc_middleman

endif

$(protoc_outputs): protoc_middleman

$(other_language_protoc_outputs): protoc_middleman

BUILT_SOURCES = $(protoc_outputs) $(other_language_protoc_outputs)

CLEANFILES = $(protoc_outputs) protoc_middleman javac_middleman conformance-java conformance-csharp $(other_language_protoc_outputs)

MAINTAINERCLEANFILES =   \
  Makefile.in

javac_middleman: ConformanceJava.java protoc_middleman $(other_language_protoc_outputs)
	javac -classpath ../java/target/classes ConformanceJava.java com/google/protobuf/conformance/Conformance.java
	@touch javac_middleman

conformance-java: javac_middleman
	@echo "Writing shortcut script conformance-java..."
	@echo '#! /bin/sh' > conformance-java
	@echo 'java -classpath .:../java/target/classes ConformanceJava "$$@"' >> conformance-java
	@chmod +x conformance-java

# Currently the conformance code is alongside the rest of the C#
# source, as it's easier to maintain there. We assume we've already
# built that, so we just need a script to run it.
conformance-csharp: $(other_language_protoc_outputs)
	@echo "Writing shortcut script conformance-csharp..."
	@echo '#! /bin/sh' > conformance-csharp
	@echo 'mono ../csharp/src/Google.Protobuf.Conformance/bin/Release/Google.Protobuf.Conformance.exe "$$@"' >> conformance-csharp
	@chmod +x conformance-csharp

# Targets for actually running tests.
test_cpp: protoc_middleman conformance-test-runner conformance-cpp
	./conformance-test-runner --failure_list failure_list_cpp.txt ./conformance-cpp

test_java: protoc_middleman conformance-test-runner conformance-java
	./conformance-test-runner ./conformance-java

test_csharp: protoc_middleman conformance-test-runner conformance-csharp
	./conformance-test-runner --failure_list failure_list_csharp.txt ./conformance-csharp

test_ruby: protoc_middleman conformance-test-runner $(other_language_protoc_outputs)
	RUBYLIB=../ruby/lib:. ./conformance-test-runner --failure_list failure_list_ruby.txt ./conformance_ruby.rb

if OBJC_CONFORMANCE_TEST

test_objc: protoc_middleman conformance-test-runner conformance-objc
	./conformance-test-runner --failure_list failure_list_objc.txt ./conformance-objc

endif
