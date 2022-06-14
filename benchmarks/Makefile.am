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


# We have to cd to $(srcdir) before executing protoc because $(protoc_inputs) is
# relative to srcdir, which may not be the same as the current directory when
# building out-of-tree.
protoc_middleman: make_tmp_dir $(top_srcdir)/src/protoc$(EXEEXT) $(benchmarks_protoc_inputs) $(well_known_type_protoc_inputs) $(benchmarks_protoc_inputs_benchmark_wrapper)
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. -I$(top_srcdir)/src --cpp_out=$$oldpwd/cpp --java_out=$$oldpwd/tmp/java/src/main/java --python_out=$$oldpwd/tmp $(benchmarks_protoc_inputs) $(benchmarks_protoc_inputs_benchmark_wrapper) )
	touch protoc_middleman

protoc_middleman2:  make_tmp_dir $(top_srcdir)/src/protoc$(EXEEXT) $(benchmarks_protoc_inputs_proto2) $(well_known_type_protoc_inputs)
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I. -I$(top_srcdir)/src --cpp_out=$$oldpwd/cpp --java_out=$$oldpwd/tmp/java/src/main/java --python_out=$$oldpwd/tmp $(benchmarks_protoc_inputs_proto2) )
	touch protoc_middleman2

all_data = $$(find $$(cd $(srcdir) && pwd) -type f -name "dataset.*.pb" -not -path "$$(cd $(srcdir) && pwd)/tmp/*")

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
	cd $(top_srcdir) && git submodule update --init -r third_party/benchmark && \
		cd third_party/benchmark && cmake -DCMAKE_BUILD_TYPE=Release && make
	cd $$oldpwd
	touch initialize_submodule

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
cpp/benchmark-cpp_benchmark.$(OBJEXT): $(benchmarks_protoc_outputs) $(benchmarks_protoc_outputs_proto2) $(benchmarks_protoc_outputs_header) $(benchmarks_protoc_outputs_proto2_header) $(top_srcdir)/src/libprotobuf.la $(top_srcdir)/third_party/benchmark/src/libbenchmark.a
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
	java/src/main/java/com/google/protobuf/ProtoCaliperBenchmark.java

javac_middleman: $(java_benchmark_testing_files) protoc_middleman protoc_middleman2
	cp -r $(srcdir)/java tmp
	mkdir -p tmp/java/lib
	cp $(top_srcdir)/java/core/target/*.jar tmp/java/lib/protobuf-java.jar
	cd tmp/java && mvn clean compile assembly:single -Dprotobuf.version=$(PACKAGE_VERSION) && cd ../..
	@touch javac_middleman

java-benchmark: javac_middleman
	@echo "Writing shortcut script java-benchmark..."
	@echo '#! /bin/bash' > java-benchmark
	@echo 'all_data=""' >> java-benchmark
	@echo 'conf=()' >> java-benchmark
	@echo 'data_files=""' >> java-benchmark
	@echo 'for arg in $$@; do if [[ $${arg:0:1} == "-" ]]; then conf+=($$arg); else data_files+="$$arg,"; fi; done' >> java-benchmark
	@echo 'java -cp '\"tmp/java/target/*:$(top_srcdir)/java/core/target/*:$(top_srcdir)/java/util/target/*\"" \\" >>java-benchmark
	@echo '   com.google.caliper.runner.CaliperMain com.google.protobuf.ProtoCaliperBenchmark -i runtime '"\\"  >> java-benchmark
	@echo '   -b serializeToByteArray,serializeToMemoryStream,deserializeFromByteArray,deserializeFromMemoryStream '"\\" >> java-benchmark
	@echo '   -DdataFile=$${data_files:0:-1} $${conf[*]}' >> java-benchmark
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

python_cpp_pkg_flags = `pkg-config --cflags --libs python3`

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
	@echo '#! /bin/bash' > python-pure-python-benchmark
	@echo export LD_LIBRARY_PATH=$(top_srcdir)/src/.libs >> python-pure-python-benchmark
	@echo export DYLD_LIBRARY_PATH=$(top_srcdir)/src/.libs >> python-pure-python-benchmark
	@echo export PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=\'python\' >> python-pure-python-benchmark
	@echo cp $(srcdir)/python/py_benchmark.py tmp >> python-pure-python-benchmark
	@echo python3 tmp/py_benchmark.py '$$@' >> python-pure-python-benchmark
	@chmod +x python-pure-python-benchmark

python-cpp-reflection-benchmark: python_add_init
	@echo "Writing shortcut script python-cpp-reflection-benchmark..."
	@echo '#! /bin/bash' > python-cpp-reflection-benchmark
	@echo export LD_LIBRARY_PATH=$(top_srcdir)/src/.libs >> python-cpp-reflection-benchmark
	@echo export DYLD_LIBRARY_PATH=$(top_srcdir)/src/.libs >> python-cpp-reflection-benchmark
	@echo export PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=\'cpp\' >> python-cpp-reflection-benchmark
	@echo cp $(srcdir)/python/py_benchmark.py tmp >> python-cpp-reflection-benchmark
	@echo python3 tmp/py_benchmark.py '$$@' >> python-cpp-reflection-benchmark
	@chmod +x python-cpp-reflection-benchmark

python-cpp-generated-code-benchmark: python_add_init libbenchmark_messages.la
	@echo "Writing shortcut script python-cpp-generated-code-benchmark..."
	@echo '#! /bin/bash' > python-cpp-generated-code-benchmark
	@echo export LD_LIBRARY_PATH=$(top_srcdir)/src/.libs >> python-cpp-generated-code-benchmark
	@echo export DYLD_LIBRARY_PATH=$(top_srcdir)/src/.libs >> python-cpp-generated-code-benchmark
	@echo export PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=\'cpp\' >> python-cpp-generated-code-benchmark
	@echo cp $(srcdir)/python/py_benchmark.py tmp >> python-cpp-generated-code-benchmark
	@echo python3 tmp/py_benchmark.py --cpp_generated '$$@' >> python-cpp-generated-code-benchmark
	@chmod +x python-cpp-generated-code-benchmark

python-pure-python: python-pure-python-benchmark
	./python-pure-python-benchmark $(all_data)

python-cpp-reflection: python-cpp-reflection-benchmark
	./python-cpp-reflection-benchmark $(all_data)

python-cpp-generated-code: python-cpp-generated-code-benchmark
	./python-cpp-generated-code-benchmark $(all_data)

############# PYTHON RULES END ##############

############ UTIL RULES BEGIN ############

bin_PROGRAMS += protoc-gen-proto2_to_proto3 proto3-data-stripper

protoc_gen_proto2_to_proto3_LDADD = $(top_srcdir)/src/libprotobuf.la $(top_srcdir)/src/libprotoc.la
protoc_gen_proto2_to_proto3_SOURCES = util/protoc-gen-proto2_to_proto3.cc
protoc_gen_proto2_to_proto3_CPPFLAGS = -I$(top_srcdir)/src -I$(srcdir)/cpp -I$(srcdir)/util

proto3_data_stripper_LDADD = $(top_srcdir)/src/libprotobuf.la
proto3_data_stripper_SOURCES = util/proto3_data_stripper.cc
proto3_data_stripper_CPPFLAGS = -I$(top_srcdir)/src -I$(srcdir)/cpp -I$(srcdir)/util
util/proto3_data_stripper-proto3_data_stripper.$(OBJEXT): $(benchmarks_protoc_outputs) $(benchmarks_protoc_outputs_proto2) $(benchmarks_protoc_outputs_header) $(benchmarks_protoc_outputs_proto2_header)
nodist_proto3_data_stripper_SOURCES =                                      \
	$(benchmarks_protoc_outputs)                                             \
	$(benchmarks_protoc_outputs_proto2)                                      \
	$(benchmarks_protoc_outputs_proto2_header)                               \
	$(benchmarks_protoc_outputs_header)


############ UTIL RULES END ############

############ PROTO3 PREPARATION BEGIN #############

proto3_proto_middleman: protoc-gen-proto2_to_proto3
	mkdir -p "tmp/proto3_proto"
	oldpwd=`pwd` && ( cd $(srcdir) && $$oldpwd/../src/protoc$(EXEEXT) -I$(srcdir) -I$(top_srcdir) --plugin=protoc-gen-proto2_to_proto3 --proto2_to_proto3_out=$$oldpwd/tmp/proto3_proto $(benchmarks_protoc_inputs) $(benchmarks_protoc_inputs_benchmark_wrapper) $(benchmarks_protoc_inputs_proto2) )
	touch proto3_proto_middleman

full_srcdir = $$(cd $(srcdir) && pwd)
proto3_data = $$(for data in $(all_data); do echo $(full_srcdir)"/tmp/proto3_data$${data\#$(full_srcdir)}"; done | xargs)

generate_proto3_data: protoc_middleman protoc_middleman2 proto3-data-stripper
	mkdir -p `dirname $(proto3_data)`
	./proto3-data-stripper $(all_data) $(proto3_data)
	touch generate_proto3_data

############ PROTO3 PREPARATION END #############

############ PHP RULES BEGIN #################

proto3_middleman_php: proto3_proto_middleman
	mkdir -p "tmp/php"
	oldpwd=`pwd` && ( cd tmp/proto3_proto && $$oldpwd/../src/protoc$(EXEEXT) -I$(srcdir) -I$(top_srcdir) --php_out=$$oldpwd/tmp/php $(benchmarks_protoc_inputs) $(benchmarks_protoc_inputs_benchmark_wrapper) $(benchmarks_protoc_inputs_proto2) )
	touch proto3_middleman_php

php-benchmark: proto3_middleman_php generate_proto3_data
	mkdir -p "tmp/php/Google/Protobuf/Benchmark" && cp php/PhpBenchmark.php "tmp/php/Google/Protobuf/Benchmark"
	cp php/autoload.php "tmp/php"
	@echo "Writing shortcut script php-benchmark..."
	@echo '#! /bin/bash' > php-benchmark
	@echo 'export PROTOBUF_PHP_SRCDIR="$$(cd $(top_srcdir) && pwd)/php/src"' >> php-benchmark
	@echo 'cd tmp/php' >> php-benchmark
	@echo 'export CURRENT_DIR=$$(pwd)' >> php-benchmark
	@echo 'php -d auto_prepend_file="autoload.php" -d include_path="$$(pwd)" Google/Protobuf/Benchmark/PhpBenchmark.php $$@' >> php-benchmark
	@echo 'cd ../..' >> php-benchmark
	@chmod +x php-benchmark

php: php-benchmark proto3_middleman_php
	./php-benchmark --behavior_prefix="php" $(proto3_data)

php_c_extension:
	cd $(top_srcdir)/php/ext/google/protobuf && phpize && ./configure CFLAGS='-O3' && make -j8

php-c-benchmark: proto3_middleman_php generate_proto3_data php_c_extension php_c_extension
	mkdir -p "tmp/php/Google/Protobuf/Benchmark" && cp php/PhpBenchmark.php "tmp/php/Google/Protobuf/Benchmark"
	cp php/autoload.php "tmp/php"
	@echo "Writing shortcut script php-c-benchmark..."
	@echo '#! /bin/bash' > php-c-benchmark
	@echo 'export PROTOBUF_PHP_SRCDIR="$$(cd $(top_srcdir) && pwd)/php/src"' >> php-c-benchmark
	@echo 'export PROTOBUF_PHP_EXTDIR="$$PROTOBUF_PHP_SRCDIR/../ext/google/protobuf/modules"' >> php-c-benchmark
	@echo 'cd tmp/php' >> php-c-benchmark
	@echo 'export CURRENT_DIR=$$(pwd)' >> php-c-benchmark
	@echo 'php -d auto_prepend_file="autoload.php" -d include_path="$$(pwd)" -d extension="$$PROTOBUF_PHP_EXTDIR/protobuf.so" Google/Protobuf/Benchmark/PhpBenchmark.php $$@' >> php-c-benchmark
	@echo 'cd ../..' >> php-c-benchmark
	@chmod +x php-c-benchmark

php_c: php-c-benchmark proto3_middleman_php
	./php-c-benchmark --behavior_prefix="php_c" $(proto3_data)


############ PHP RULES END #################

EXTRA_DIST =									\
	$(benchmarks_protoc_inputs_benchmark_wrapper)				\
	$(benchmarks_protoc_inputs)						\
	$(benchmarks_protoc_inputs_proto2)					\
	google_size.proto

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
	proto3_proto_middleman                                                   \
	generate_proto3_data                                                     \
	php-benchmark                                                            \
	php-c-benchmark                                                          \
	proto3_middleman_php

clean-local:
	-rm -rf tmp/*

