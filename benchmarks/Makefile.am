benchmarks_protoc_inputs_benchmark_wrapper =                               \
	benchmarks.proto

benchmarks_protoc_inputs =                                                 \
	datasets/google_message1/proto3/benchmark_message1_proto3.proto

benchmarks_protoc_inputs_proto2 =                                          \
	datasets/google_message1/proto2/benchmark_message1_proto2.proto          \
	datasets/google_message2/benchmark_message2.proto                        \
	datasets/google_message3/benchmark_message3.proto                        \
	datasets/google_message3/benchmark_message3_1.proto                      \
	datasets/google_message3/benchmark_message3_2.proto                      \
	datasets/google_message3/benchmark_message3_3.proto                      \
	datasets/google_message3/benchmark_message3_4.proto                      \
	datasets/google_message3/benchmark_message3_5.proto                      \
	datasets/google_message3/benchmark_message3_6.proto                      \
	datasets/google_message3/benchmark_message3_7.proto                      \
	datasets/google_message3/benchmark_message3_8.proto                      \
	datasets/google_message4/benchmark_message4.proto                        \
	datasets/google_message4/benchmark_message4_1.proto                      \
	datasets/google_message4/benchmark_message4_2.proto                      \
	datasets/google_message4/benchmark_message4_3.proto

make_tmp_dir:
	mkdir -p 'tmp/java/src/main/java'
	touch make_tmp_dir

if USE_EXTERNAL_PROTOC

protoc_middleman: make_tmp_dir $(benchmarks_protoc_inputs)
	$(PROTOC) -I$(srcdir) -I$(top_srcdir) --cpp_out=./cpp --java_out=./tmp $(benchmarks_protoc_inputs) $(benchmarks_protoc_inputs_benchmark_wrapper)
	touch protoc_middleman

protoc_middleman2: make_tmp_dir $(benchmarks_protoc_inputs_proto2)
	$(PROTOC) -I$(srcdir) -I$(top_srcdir) --cpp_out=./cpp --java_out=./tmp $(benchmarks_protoc_inputs_proto2)
	touch protoc_middleman2

else

# We have to cd to $(srcdir) before executing protoc because $(protoc_inputs) is
# relative to srcdir, which may not be the same as the current directory when
# building out-of-tree.
protoc_middleman: make_tmp_dir $(top_srcdir)/src/protoc$(EXEEXT) $(benchmarks_protoc_inputs) $(well_known_type_protoc_inputs) $(benchmarks_protoc_inputs_benchmark_wrapper)
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. -I$(top_srcdir)/src --cpp_out=$$oldpwd/cpp --java_out=$$oldpwd/tmp/java/src/main/java --python_out=$$oldpwd/tmp $(benchmarks_protoc_inputs) $(benchmarks_protoc_inputs_benchmark_wrapper) )
	touch protoc_middleman

protoc_middleman2:  make_tmp_dir $(top_srcdir)/src/protoc$(EXEEXT) $(benchmarks_protoc_inputs_proto2) $(well_known_type_protoc_inputs)
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. -I$(top_srcdir)/src --cpp_out=$$oldpwd/cpp --java_out=$$oldpwd/tmp/java/src/main/java --python_out=$$oldpwd/tmp $(benchmarks_protoc_inputs_proto2) )
	touch protoc_middleman2

endif

all_data = $$(find . -type f -name "dataset.*.pb" -not -path "./tmp/*")

############# CPP RULES ##############

benchmarks_protoc_outputs =                                                \
	cpp/benchmarks.pb.cc                                                     \
	cpp/datasets/google_message1/proto3/benchmark_message1_proto3.pb.cc

benchmarks_protoc_outputs_header =                                         \
	cpp/benchmarks.pb.h                                                      \
	cpp/datasets/google_message1/proto3/benchmark_message1_proto3.pb.h

benchmarks_protoc_outputs_proto2_header =                                  \
	cpp/datasets/google_message1/proto2/benchmark_message1_proto2.pb.h       \
	cpp/datasets/google_message2/benchmark_message2.pb.h                     \
	cpp/datasets/google_message3/benchmark_message3.pb.h                     \
	cpp/datasets/google_message3/benchmark_message3_1.pb.h                   \
	cpp/datasets/google_message3/benchmark_message3_2.pb.h                   \
	cpp/datasets/google_message3/benchmark_message3_3.pb.h                   \
	cpp/datasets/google_message3/benchmark_message3_4.pb.h                   \
	cpp/datasets/google_message3/benchmark_message3_5.pb.h                   \
	cpp/datasets/google_message3/benchmark_message3_6.pb.h                   \
	cpp/datasets/google_message3/benchmark_message3_7.pb.h                   \
	cpp/datasets/google_message3/benchmark_message3_8.pb.h                   \
	cpp/datasets/google_message4/benchmark_message4.pb.h                     \
	cpp/datasets/google_message4/benchmark_message4_1.pb.h                   \
	cpp/datasets/google_message4/benchmark_message4_2.pb.h                   \
	cpp/datasets/google_message4/benchmark_message4_3.pb.h

benchmarks_protoc_outputs_proto2 =                                         \
	cpp/datasets/google_message1/proto2/benchmark_message1_proto2.pb.cc      \
	cpp/datasets/google_message2/benchmark_message2.pb.cc                    \
	cpp/datasets/google_message3/benchmark_message3.pb.cc                    \
	cpp/datasets/google_message3/benchmark_message3_1.pb.cc                  \
	cpp/datasets/google_message3/benchmark_message3_2.pb.cc                  \
	cpp/datasets/google_message3/benchmark_message3_3.pb.cc                  \
	cpp/datasets/google_message3/benchmark_message3_4.pb.cc                  \
	cpp/datasets/google_message3/benchmark_message3_5.pb.cc                  \
	cpp/datasets/google_message3/benchmark_message3_6.pb.cc                  \
	cpp/datasets/google_message3/benchmark_message3_7.pb.cc                  \
	cpp/datasets/google_message3/benchmark_message3_8.pb.cc                  \
	cpp/datasets/google_message4/benchmark_message4.pb.cc                    \
	cpp/datasets/google_message4/benchmark_message4_1.pb.cc                  \
	cpp/datasets/google_message4/benchmark_message4_2.pb.cc                  \
	cpp/datasets/google_message4/benchmark_message4_3.pb.cc


$(benchmarks_protoc_outputs): protoc_middleman
$(benchmarks_protoc_outputs_header): protoc_middleman
$(benchmarks_protoc_outputs_proto2): protoc_middleman2
$(benchmarks_protoc_outputs_proto2_header): protoc_middleman2

initialize_submodule:
	oldpwd=`pwd`
	cd $(top_srcdir)/third_party
	git submodule update --init -r
	cd $(top_srcdir)/third_party/benchmark && cmake -DCMAKE_BUILD_TYPE=Release && make
	cd $$oldpwd
	touch initialize_submodule
	
$(top_srcdir)/src/libprotobuf.la: initialize_submodule
$(top_srcdir)/third_party/benchmark/src/libbenchmark.a: initialize_submodule

AM_CXXFLAGS = $(NO_OPT_CXXFLAGS) $(PROTOBUF_OPT_FLAG) -Wall -Wwrite-strings -Woverloaded-virtual -Wno-sign-compare

bin_PROGRAMS = cpp-benchmark

cpp_benchmark_LDADD = $(top_srcdir)/src/libprotobuf.la $(top_srcdir)/third_party/benchmark/src/libbenchmark.a
cpp_benchmark_SOURCES = cpp/cpp_benchmark.cc
cpp_benchmark_CPPFLAGS = -I$(top_srcdir)/src -I$(srcdir)/cpp -I$(top_srcdir)/third_party/benchmark/include
# Explicit deps because BUILT_SOURCES are only done before a "make all/check"
# so a direct "make test_cpp" could fail if parallel enough.
# See: https://www.gnu.org/software/automake/manual/html_node/Built-Sources-Example.html#Recording-Dependencies-manually
cpp/cpp_benchmark-cpp_benchmark.$(OBJEXT): $(benchmarks_protoc_outputs) $(benchmarks_protoc_outputs_proto2) $(benchmarks_protoc_outputs_header) $(benchmarks_protoc_outputs_proto2_header) $(top_srcdir)/src/libprotobuf.la $(top_srcdir)/third_party/benchmark/src/libbenchmark.a
nodist_cpp_benchmark_SOURCES =                                             \
	$(benchmarks_protoc_outputs)                                             \
	$(benchmarks_protoc_outputs_proto2)                                      \
	$(benchmarks_protoc_outputs_proto2_header)                               \
	$(benchmarks_protoc_outputs_header)

cpp: protoc_middleman protoc_middleman2 cpp-benchmark initialize_submodule
	./cpp-benchmark $(all_data)

############ CPP RULES END ############

############# JAVA RULES ##############

java_benchmark_testing_files =                                      \
	java/src/main/java/com/google/protobuf/ProtoBench.java            \
	java/src/main/java/com/google/protobuf/ProtoCaliperBenchmark.java

javac_middleman: $(java_benchmark_testing_files) protoc_middleman protoc_middleman2
	cp -r java tmp && cd tmp/java &&  mvn clean compile assembly:single
	cd ../..
	@touch javac_middleman

java-benchmark: javac_middleman
	@echo "Writing shortcut script java-benchmark..."
	@echo '#! /bin/sh' > java-benchmark
	@echo 'java -cp '"tmp/java/target/*.jar"' com.google.protobuf.ProtoBench $$@' >> java-benchmark
	@chmod +x java-benchmark

java: protoc_middleman protoc_middleman2 java-benchmark
	./java-benchmark $(all_data)

############# JAVA RULES END ##############


############# PYTHON RULES ##############

python_add_init: protoc_middleman protoc_middleman2
	all_file=`find tmp -type f -regex '.*\.py'` &&                   \
	for file in $${all_file[@]}; do                                  \
		path="$${file%/*}";                                            \
		while true; do                                                 \
			touch "$$path/__init__.py" && chmod +x "$$path/__init__.py"; \
			if [[ $$path != *"/"* ]]; then break; fi;                    \
			path=$${path%/*};                                            \
		done                                                           \
	done

python_cpp_pkg_flags = `pkg-config --cflags --libs python`

lib_LTLIBRARIES = libbenchmark_messages.la
libbenchmark_messages_la_SOURCES = python/python_benchmark_messages.cc
libbenchmark_messages_la_LIBADD = $(top_srcdir)/src/.libs/libprotobuf.la
libbenchmark_messages_la_LDFLAGS = -version-info 1:0:0 -export-dynamic
libbenchmark_messages_la_CPPFLAGS = -I$(top_srcdir)/src -I$(srcdir)/cpp $(python_cpp_pkg_flags)
libbenchmark_messages_la-python_benchmark_messages.$(OBJEXT): $(benchmarks_protoc_outputs_header) $(benchmarks_protoc_outputs_proto2_header) $(benchmarks_protoc_outputs) $(benchmarks_protoc_outputs_proto2)
nodist_libbenchmark_messages_la_SOURCES =                         \
	$(benchmarks_protoc_outputs)                                    \
	$(benchmarks_protoc_outputs_proto2)                             \
	$(benchmarks_protoc_outputs_proto2_header)                      \
	$(benchmarks_protoc_outputs_header)

python-pure-python-benchmark: python_add_init
	@echo "Writing shortcut script python-pure-python-benchmark..."
	@echo '#! /bin/sh' > python-pure-python-benchmark
	@echo export LD_LIBRARY_PATH=$(top_srcdir)/src/libprotobuf.la >> python-pure-python-benchmark
	@echo export DYLD_LIBRARY_PATH=$(top_srcdir)/src/libprotobuf.la >> python-pure-python-benchmark
	@echo export PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=\'python\' >> python-pure-python-benchmark
	@echo cp python/py_benchmark.py tmp >> python-pure-python-benchmark
	@echo python tmp/py_benchmark.py '$$@' >> python-pure-python-benchmark
	@chmod +x python-pure-python-benchmark

python-cpp-reflection-benchmark: python_add_init
	@echo "Writing shortcut script python-cpp-reflection-benchmark..."
	@echo '#! /bin/sh' > python-cpp-reflection-benchmark
	@echo export LD_LIBRARY_PATH=$(top_srcdir)/src/libprotobuf.la >> python-cpp-reflection-benchmark
	@echo export DYLD_LIBRARY_PATH=$(top_srcdir)/src/libprotobuf.la >> python-cpp-reflection-benchmark
	@echo export PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=\'cpp\' >> python-cpp-reflection-benchmark
	@echo cp python/py_benchmark.py tmp >> python-cpp-reflection-benchmark
	@echo python tmp/py_benchmark.py '$$@' >> python-cpp-reflection-benchmark
	@chmod +x python-cpp-reflection-benchmark

python-cpp-generated-code-benchmark: python_add_init libbenchmark_messages.la
	@echo "Writing shortcut script python-cpp-generated-code-benchmark..."
	@echo '#! /bin/sh' > python-cpp-generated-code-benchmark
	@echo export LD_LIBRARY_PATH=$(top_srcdir)/src/libprotobuf.la >> python-cpp-generated-code-benchmark
	@echo export DYLD_LIBRARY_PATH=$(top_srcdir)/src/libprotobuf.la >> python-cpp-generated-code-benchmark
	@echo export PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=\'cpp\' >> python-cpp-generated-code-benchmark
	@echo cp python/py_benchmark.py tmp >> python-cpp-generated-code-benchmark
	@echo python tmp/py_benchmark.py --cpp_generated '$$@' >> python-cpp-generated-code-benchmark
	@chmod +x python-cpp-generated-code-benchmark

python-pure-python: python-pure-python-benchmark
	./python-pure-python-benchmark $(all_data)

python-cpp-reflection: python-cpp-reflection-benchmark
	./python-cpp-reflection-benchmark $(all_data)

python-cpp-generated-code: python-cpp-generated-code-benchmark
	./python-cpp-generated-code-benchmark $(all_data)

############# PYTHON RULES END ##############

############# GO RULES BEGIN ##############

benchmarks_protoc_inputs_proto2_message1 =                                 \
	datasets/google_message1/proto2/benchmark_message1_proto2.proto

benchmarks_protoc_inputs_proto2_message2 =                                 \
	datasets/google_message2/benchmark_message2.proto

benchmarks_protoc_inputs_proto2_message3 =                                 \
	datasets/google_message3/benchmark_message3.proto                        \
	datasets/google_message3/benchmark_message3_1.proto                      \
	datasets/google_message3/benchmark_message3_2.proto                      \
	datasets/google_message3/benchmark_message3_3.proto                      \
	datasets/google_message3/benchmark_message3_4.proto                      \
	datasets/google_message3/benchmark_message3_5.proto                      \
	datasets/google_message3/benchmark_message3_6.proto                      \
	datasets/google_message3/benchmark_message3_7.proto                      \
	datasets/google_message3/benchmark_message3_8.proto

benchmarks_protoc_inputs_proto2_message4 =                                 \
	datasets/google_message4/benchmark_message4.proto                        \
	datasets/google_message4/benchmark_message4_1.proto                      \
	datasets/google_message4/benchmark_message4_2.proto                      \
	datasets/google_message4/benchmark_message4_3.proto

if USE_EXTERNAL_PROTOC

go_protoc_middleman: make_tmp_dir $(benchmarks_protoc_inputs)
	$(PROTOC) -I$(srcdir) -I$(top_srcdir) --go_out=$$oldpwd/tmp $(benchmarks_protoc_inputs) 
	$(PROTOC) -I$(srcdir) -I$(top_srcdir) --go_out=$$oldpwd/tmp $(benchmarks_protoc_inputs_benchmark_wrapper)
	touch protoc_middleman

go_protoc_middleman2: make_tmp_dir $(benchmarks_protoc_inputs_proto2_message1) $(benchmarks_protoc_inputs_proto2_message2) $(benchmarks_protoc_inputs_proto2_message3) $(benchmarks_protoc_inputs_proto2_message4)
	$(PROTOC) -I$(srcdir) -I$(top_srcdir) --go_out=$$oldpwd/tmp $(benchmarks_protoc_inputs_proto2_message1)
	$(PROTOC) -I$(srcdir) -I$(top_srcdir) --go_out=$$oldpwd/tmp $(benchmarks_protoc_inputs_proto2_message2)
	$(PROTOC) -I$(srcdir) -I$(top_srcdir) --go_out=$$oldpwd/tmp $(benchmarks_protoc_inputs_proto2_message3)
	$(PROTOC) -I$(srcdir) -I$(top_srcdir) --go_out=$$oldpwd/tmp $(benchmarks_protoc_inputs_proto2_message4)
	touch protoc_middleman2

else

# We have to cd to $(srcdir) before executing protoc because $(protoc_inputs) is
# relative to srcdir, which may not be the same as the current directory when
# building out-of-tree.
go_protoc_middleman: make_tmp_dir $(top_srcdir)/src/protoc$(EXEEXT) $(benchmarks_protoc_inputs) $(well_known_type_protoc_inputs)
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. -I$(top_srcdir)/src --go_out=$$oldpwd/tmp $(benchmarks_protoc_inputs) )
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. -I$(top_srcdir)/src --go_out=$$oldpwd/tmp $(benchmarks_protoc_inputs_benchmark_wrapper) )
	touch protoc_middleman

go_protoc_middleman2:  make_tmp_dir $(top_srcdir)/src/protoc$(EXEEXT) $(benchmarks_protoc_inputs_proto2_message1) $(benchmarks_protoc_inputs_proto2_message2) $(benchmarks_protoc_inputs_proto2_message3) $(benchmarks_protoc_inputs_proto2_message4) $(well_known_type_protoc_inputs)
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. -I$(top_srcdir)/src --go_out=$$oldpwd/tmp $(benchmarks_protoc_inputs_proto2_message1) )
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. -I$(top_srcdir)/src --go_out=$$oldpwd/tmp $(benchmarks_protoc_inputs_proto2_message2) )
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. -I$(top_srcdir)/src --go_out=$$oldpwd/tmp $(benchmarks_protoc_inputs_proto2_message3) )
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. -I$(top_srcdir)/src --go_out=$$oldpwd/tmp $(benchmarks_protoc_inputs_proto2_message4) )
	touch protoc_middleman2

endif

go-benchmark: go_protoc_middleman go_protoc_middleman2 
	@echo "Writing shortcut script go-benchmark..."
	@echo '#! /bin/sh' > go-benchmark
	@echo 'cd go' >> go-benchmark
	@echo 'all_data=""' >> go-benchmark
	@echo 'conf=()' >> go-benchmark
	@echo 'data_files=()' >> go-benchmark
	@echo 'for arg in $$@; do if [[ $${arg:0:1} == "-" ]]; then conf+=($$arg); else data_files+=("../$$arg"); fi; done' >> go-benchmark
	@echo 'go test -bench=. $${conf[*]} -- $${data_files[*]}' >> go-benchmark
	@echo 'cd ..' >> go-benchmark
	@chmod +x go-benchmark

go: go_protoc_middleman go_protoc_middleman2 go-benchmark
	./go-benchmark $(all_data)

############# GO RULES END ##############


MAINTAINERCLEANFILES =                                                     \
	Makefile.in

CLEANFILES =                                                               \
	$(benchmarks_protoc_outputs)                                             \
	$(benchmarks_protoc_outputs_header)                                      \
	$(benchmarks_protoc_outputs_proto2)                                      \
	$(benchmarks_protoc_outputs_proto2_header)                               \
	initialize_submodule                                                     \
	make_tmp_dir                                                             \
	protoc_middleman                                                         \
	protoc_middleman2                                                        \
	javac_middleman                                                          \
	java-benchmark                                                           \
	python_cpp_proto_library                                                 \
	python-pure-python-benchmark                                             \
	python-cpp-reflection-benchmark                                          \
	python-cpp-generated-code-benchmark                                      \
	go-benchmark                                                             \
	go_protoc_middleman                                                      \
	go_protoc_middleman2

clean-local:
	-rm -rf tmp/*
