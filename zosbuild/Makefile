all: protoc

AR=ar
ARFLAGS=rc
RANLIB=ranlib
ifdef XLCPP
	CPP=$(XLCPP)
else
	CPP:=/bin/xlclang++
endif

CPPFLAGS_ALL=\
	     -F:llvmrt \
	     -+ \
	     -D_AE_BIMODAL=1 \
	     -D_ALL_SOURCE \
	     -D_ENHANCED_ASCII_EXT=0xFFFFFFFF \
	     -D_Export=extern \
	     -D_LARGE_TIME_API \
	     -D_OPEN_MSGQ_EXT \
	     -D_OPEN_SYS_FILE_EXT=1 \
	     -D_OPEN_SYS_SOCK_IPV6 \
	     -DPATH_MAX=1023 \
	     -D__static_assert=static_assert \
	     -D_UNIX03_SOURCE \
	     -D_UNIX03_THREADS \
	     -D_UNIX03_WITHDRAWN \
	     -D_XOPEN_SOURCE=600 \
	     -D_XOPEN_SOURCE_EXTENDED \
	     -q64 \
	     -qascii \
	     -qgonumber \
	     -qlongname \
	     -qlibansi \
	     -qFLOAT=IEEE \
	     -qtune=10 \
	     -qarch=9 \
	     -qasm \
	     -qasmlib=sys1.maclib:sys1.modgen \
	     -qrent \
	     -Wa,goff \
	     -Wl,REUS=NONE \
	     -O3 \
	     -D_LARGEFILE64_SOURCE=1
CPPFLAGS_COMPILE=\
	     -c \
	     -qmakedep
CPPFLAGS_SPEC=\
	      -DGOOGLE_PROTOBUF_CMAKE_BUILD \
	      -DHAVE_PTHREAD \
	      -DHAVE_ZLIB

ifdef XLC
	CC = $(XLC)
else
	CC:=/bin/xlclang
endif

CFLAGS=-O3 -D_LARGEFILE64_SOURCE=1 -qrent

ME:=$(firstword $(MAKEFILE_LIST))
ROOT_DIR:=$(shell dirname $(ME))/..

ifndef BIN_DIR
	BIN_DIR:=$(HOME)/local/bin
endif

ifndef SRC_DIR
	SRC_DIR:=$(ROOT_DIR)/src
endif

ZLIB_PATH=$(ROOT_DIR)/zosbuild/zlib
ZLIB_PATCH_FILE=$(ROOT_DIR)/zosbuild/zlib_patch_file

SRCZ=\
	$(ZLIB_PATH)/adler32.c \
	$(ZLIB_PATH)/crc32.c \
	$(ZLIB_PATH)/deflate.c \
	$(ZLIB_PATH)/infback.c \
	$(ZLIB_PATH)/inffast.c \
	$(ZLIB_PATH)/inflate.c \
	$(ZLIB_PATH)/inftrees.c \
	$(ZLIB_PATH)/trees.c \
	$(ZLIB_PATH)/zutil.c

SRCG=\
	$(ZLIB_PATH)/compress.c \
	$(ZLIB_PATH)/uncompr.c \
	$(ZLIB_PATH)/gzclose.c \
	$(ZLIB_PATH)/gzlib.c \
	$(ZLIB_PATH)/gzread.c \
	$(ZLIB_PATH)/gzwrite.c

SRC_LIBZ = $(SRCZ) $(SRCG)

OBJZ=$(notdir $(patsubst %.c,%.o,$(SRCZ)))
OBJG=$(notdir $(patsubst %.c,%.o,$(SRCG)))
OBJ_LIBZ = $(OBJZ) $(OBJG)


SRC_PROTOBUF=\
	     $(SRC_DIR)/google/protobuf/any.cc \
	     $(SRC_DIR)/google/protobuf/any.pb.cc \
	     $(SRC_DIR)/google/protobuf/any_lite.cc \
	     $(SRC_DIR)/google/protobuf/api.pb.cc \
	     $(SRC_DIR)/google/protobuf/arena.cc \
	     $(SRC_DIR)/google/protobuf/arenastring.cc \
	     $(SRC_DIR)/google/protobuf/atomic_store.cc \
	     $(SRC_DIR)/google/protobuf/compiler/importer.cc \
	     $(SRC_DIR)/google/protobuf/compiler/parser.cc \
	     $(SRC_DIR)/google/protobuf/descriptor.cc \
	     $(SRC_DIR)/google/protobuf/descriptor.pb.cc \
	     $(SRC_DIR)/google/protobuf/descriptor_database.cc \
	     $(SRC_DIR)/google/protobuf/duration.pb.cc \
	     $(SRC_DIR)/google/protobuf/dynamic_message.cc \
	     $(SRC_DIR)/google/protobuf/empty.pb.cc \
	     $(SRC_DIR)/google/protobuf/extension_set.cc \
	     $(SRC_DIR)/google/protobuf/extension_set_heavy.cc \
	     $(SRC_DIR)/google/protobuf/field_mask.pb.cc \
	     $(SRC_DIR)/google/protobuf/generated_enum_util.cc \
	     $(SRC_DIR)/google/protobuf/generated_message_reflection.cc \
	     $(SRC_DIR)/google/protobuf/generated_message_table_driven.cc \
	     $(SRC_DIR)/google/protobuf/generated_message_table_driven_lite.cc \
	     $(SRC_DIR)/google/protobuf/generated_message_util.cc \
	     $(SRC_DIR)/google/protobuf/implicit_weak_message.cc \
	     $(SRC_DIR)/google/protobuf/map.cc \
	     $(SRC_DIR)/google/protobuf/io/coded_stream.cc \
	     $(SRC_DIR)/google/protobuf/io/gzip_stream.cc \
	     $(SRC_DIR)/google/protobuf/io/io_win32.cc \
	     $(SRC_DIR)/google/protobuf/io/printer.cc \
	     $(SRC_DIR)/google/protobuf/io/strtod.cc \
	     $(SRC_DIR)/google/protobuf/io/tokenizer.cc \
	     $(SRC_DIR)/google/protobuf/io/zero_copy_stream.cc \
	     $(SRC_DIR)/google/protobuf/io/zero_copy_stream_impl.cc \
	     $(SRC_DIR)/google/protobuf/io/zero_copy_stream_impl_lite.cc \
	     $(SRC_DIR)/google/protobuf/map_field.cc \
	     $(SRC_DIR)/google/protobuf/message.cc \
	     $(SRC_DIR)/google/protobuf/message_lite.cc \
	     $(SRC_DIR)/google/protobuf/parse_context.cc \
	     $(SRC_DIR)/google/protobuf/reflection_ops.cc \
	     $(SRC_DIR)/google/protobuf/repeated_field.cc \
	     $(SRC_DIR)/google/protobuf/service.cc \
	     $(SRC_DIR)/google/protobuf/source_context.pb.cc \
	     $(SRC_DIR)/google/protobuf/struct.pb.cc \
	     $(SRC_DIR)/google/protobuf/stubs/bytestream.cc \
	     $(SRC_DIR)/google/protobuf/stubs/common.cc \
	     $(SRC_DIR)/google/protobuf/stubs/int128.cc \
	     $(SRC_DIR)/google/protobuf/stubs/status.cc \
	     $(SRC_DIR)/google/protobuf/stubs/statusor.cc \
	     $(SRC_DIR)/google/protobuf/stubs/stringpiece.cc \
	     $(SRC_DIR)/google/protobuf/stubs/stringprintf.cc \
	     $(SRC_DIR)/google/protobuf/stubs/structurally_valid.cc \
	     $(SRC_DIR)/google/protobuf/stubs/strutil.cc \
	     $(SRC_DIR)/google/protobuf/stubs/substitute.cc \
	     $(SRC_DIR)/google/protobuf/stubs/time.cc \
	     $(SRC_DIR)/google/protobuf/text_format.cc \
	     $(SRC_DIR)/google/protobuf/timestamp.pb.cc \
	     $(SRC_DIR)/google/protobuf/type.pb.cc \
	     $(SRC_DIR)/google/protobuf/unknown_field_set.cc \
	     $(SRC_DIR)/google/protobuf/util/delimited_message_util.cc \
	     $(SRC_DIR)/google/protobuf/util/field_comparator.cc \
	     $(SRC_DIR)/google/protobuf/util/field_mask_util.cc \
	     $(SRC_DIR)/google/protobuf/util/internal/datapiece.cc \
	     $(SRC_DIR)/google/protobuf/util/internal/default_value_objectwriter.cc \
	     $(SRC_DIR)/google/protobuf/util/internal/error_listener.cc \
	     $(SRC_DIR)/google/protobuf/util/internal/field_mask_utility.cc \
	     $(SRC_DIR)/google/protobuf/util/internal/json_escaping.cc \
	     $(SRC_DIR)/google/protobuf/util/internal/json_objectwriter.cc \
	     $(SRC_DIR)/google/protobuf/util/internal/json_stream_parser.cc \
	     $(SRC_DIR)/google/protobuf/util/internal/object_writer.cc \
	     $(SRC_DIR)/google/protobuf/util/internal/proto_writer.cc \
	     $(SRC_DIR)/google/protobuf/util/internal/protostream_objectsource.cc \
	     $(SRC_DIR)/google/protobuf/util/internal/protostream_objectwriter.cc \
	     $(SRC_DIR)/google/protobuf/util/internal/type_info.cc \
	     $(SRC_DIR)/google/protobuf/util/internal/type_info_test_helper.cc \
	     $(SRC_DIR)/google/protobuf/util/internal/utility.cc \
	     $(SRC_DIR)/google/protobuf/util/json_util.cc \
	     $(SRC_DIR)/google/protobuf/util/message_differencer.cc \
	     $(SRC_DIR)/google/protobuf/util/time_util.cc \
	     $(SRC_DIR)/google/protobuf/util/type_resolver_util.cc \
	     $(SRC_DIR)/google/protobuf/wire_format.cc \
	     $(SRC_DIR)/google/protobuf/wire_format_lite.cc \
	     $(SRC_DIR)/google/protobuf/wrappers.pb.cc
SRC_PROTOC=\
	      $(SRC_DIR)/google/protobuf/compiler/code_generator.cc \
	      $(SRC_DIR)/google/protobuf/compiler/command_line_interface.cc \
	      $(SRC_DIR)/google/protobuf/compiler/cpp/cpp_enum.cc \
	      $(SRC_DIR)/google/protobuf/compiler/cpp/cpp_enum_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/cpp/cpp_extension.cc \
	      $(SRC_DIR)/google/protobuf/compiler/cpp/cpp_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/cpp/cpp_file.cc \
	      $(SRC_DIR)/google/protobuf/compiler/cpp/cpp_generator.cc \
	      $(SRC_DIR)/google/protobuf/compiler/cpp/cpp_helpers.cc \
	      $(SRC_DIR)/google/protobuf/compiler/cpp/cpp_map_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/cpp/cpp_message.cc \
	      $(SRC_DIR)/google/protobuf/compiler/cpp/cpp_message_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/cpp/cpp_padding_optimizer.cc \
	      $(SRC_DIR)/google/protobuf/compiler/cpp/cpp_primitive_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/cpp/cpp_service.cc \
	      $(SRC_DIR)/google/protobuf/compiler/cpp/cpp_string_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/csharp/csharp_doc_comment.cc \
	      $(SRC_DIR)/google/protobuf/compiler/csharp/csharp_enum.cc \
	      $(SRC_DIR)/google/protobuf/compiler/csharp/csharp_enum_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/csharp/csharp_field_base.cc \
	      $(SRC_DIR)/google/protobuf/compiler/csharp/csharp_generator.cc \
	      $(SRC_DIR)/google/protobuf/compiler/csharp/csharp_helpers.cc \
	      $(SRC_DIR)/google/protobuf/compiler/csharp/csharp_map_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/csharp/csharp_message.cc \
	      $(SRC_DIR)/google/protobuf/compiler/csharp/csharp_message_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/csharp/csharp_primitive_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/csharp/csharp_reflection_class.cc \
	      $(SRC_DIR)/google/protobuf/compiler/csharp/csharp_repeated_enum_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/csharp/csharp_repeated_message_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/csharp/csharp_repeated_primitive_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/csharp/csharp_source_generator_base.cc \
	      $(SRC_DIR)/google/protobuf/compiler/csharp/csharp_wrapper_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_context.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_doc_comment.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_enum.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_enum_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_enum_field_lite.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_enum_lite.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_extension.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_extension_lite.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_file.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_generator.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_generator_factory.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_helpers.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_map_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_map_field_lite.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_message.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_message_builder.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_message_builder_lite.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_message_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_message_field_lite.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_message_lite.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_name_resolver.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_primitive_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_primitive_field_lite.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_service.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_shared_code_generator.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_string_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/java/java_string_field_lite.cc \
	      $(SRC_DIR)/google/protobuf/compiler/js/js_generator.cc \
	      $(SRC_DIR)/google/protobuf/compiler/js/well_known_types_embed.cc \
	      $(SRC_DIR)/google/protobuf/compiler/objectivec/objectivec_enum.cc \
	      $(SRC_DIR)/google/protobuf/compiler/objectivec/objectivec_enum_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/objectivec/objectivec_extension.cc \
	      $(SRC_DIR)/google/protobuf/compiler/objectivec/objectivec_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/objectivec/objectivec_file.cc \
	      $(SRC_DIR)/google/protobuf/compiler/objectivec/objectivec_generator.cc \
	      $(SRC_DIR)/google/protobuf/compiler/objectivec/objectivec_helpers.cc \
	      $(SRC_DIR)/google/protobuf/compiler/objectivec/objectivec_map_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/objectivec/objectivec_message.cc \
	      $(SRC_DIR)/google/protobuf/compiler/objectivec/objectivec_message_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/objectivec/objectivec_oneof.cc \
	      $(SRC_DIR)/google/protobuf/compiler/objectivec/objectivec_primitive_field.cc \
	      $(SRC_DIR)/google/protobuf/compiler/php/php_generator.cc \
	      $(SRC_DIR)/google/protobuf/compiler/plugin.cc \
	      $(SRC_DIR)/google/protobuf/compiler/plugin.pb.cc \
	      $(SRC_DIR)/google/protobuf/compiler/python/python_generator.cc \
	      $(SRC_DIR)/google/protobuf/compiler/ruby/ruby_generator.cc \
	      $(SRC_DIR)/google/protobuf/compiler/subprocess.cc \
	      $(SRC_DIR)/google/protobuf/compiler/zip_writer.cc

OBJ_PROTOC=$(notdir $(patsubst %.cc,%.o,$(SRC_PROTOC)))
OBJ_PROTOBUF=$(notdir $(patsubst %.cc,%.o,$(SRC_PROTOBUF)))

DEPS=\
     -I$(ROOT_DIR)/cmake \
     -I$(ROOT_DIR)/src \
     -I$(ZLIB_PATH)/ \
     -I$(ROOT_DIR)/third_party/googletest/googlemock \
     -I$(ROOT_DIR)/third_party/googletest/googletest \
     -I$(ROOT_DIR)/third_party/googletest/googletest/include \
     -I$(ROOT_DIR)/third_party/googletest/googlemock/include

-include $(patsubst %.o,%.d,$(OBJ_PROTOC))
-include $(patsubst %.o,%.d,$(OBJ_PROTOBUF))
-include $(patsubst %.o,%.d,$(OBJ_LIBZ))

vpath %.h $(ZLIB_PATH)/ $(SRC_DIR)/cmake $(ROOT_DIR)/src $(ROOT_DIR)/third_party/googletest/googlemock $(ROOT_DIR)/third_party/googletest/googletest $(ROOT_DIR)/third_party/googletest/googletest/include $(ROOT_DIR)/third_party/googletest/googlemock/include 
vpath %.cc $(SRC_DIR)/ $(SRC_DIR)/google/protobuf/ $(SRC_DIR)/google/protobuf/io/ $(SRC_DIR)/google/protobuf/stubs/ $(SRC_DIR)/google/protobuf/util/ $(SRC_DIR)/google/protobuf/util/internal/ $(SRC_DIR)/google/protobuf/compiler/ $(SRC_DIR)/google/protobuf/compiler/python/ $(SRC_DIR)/google/protobuf/compiler/ruby/ $(SRC_DIR)/google/protobuf/compiler/cpp/ $(SRC_DIR)/google/protobuf/compiler/csharp/ $(SRC_DIR)/google/protobuf/compiler/java/ $(SRC_DIR)/google/protobuf/compiler/js/ $(SRC_DIR)/google/protobuf/compiler/objectivec/  $(SRC_DIR)/google/protobuf/compiler/php/  $(SRC_DIR)/google/protobuf/compiler/
vpath %.cpp $(SRC_DIR)/ 
vpath %.c $(ZLIB_PATH)/  

protoc:  libz.a main.o libprotoc.a libprotobuf.a 
	 $(CPP) $(CPPFLAGS_ALL) $(CPPFLAGS_SPEC) -qmakedep -o $@ $^ 

%.o: %.cc $(ME)
	$(CPP) $(CPPFLAGS_ALL) $(CPPFLAGS_SPEC) $(CPPFLAGS_COMPILE) $< -o $@ $(DEPS) 

libprotobuf.a: $(OBJ_PROTOBUF)
	$(AR) $(ARFLAGS) $@ $(OBJ_PROTOBUF)	

libprotoc.a: $(OBJ_PROTOC)
	$(AR) $(ARFLAGS) $@ $(OBJ_PROTOC)


$(ZLIB_PATH)/%.o: %.c
	$(CC) $(CFLAGS) $< -o $@	

$(SRC_LIBZ):
	git clone https://github.com/madler/zlib
	-@echo "zlib cloned successfully"

libz.a: $(SRC_LIBZ) $(OBJ_LIBZ)
	$(AR) $(ARFLAGS) $@ $(OBJ_LIBZ)
	-@ ($(RANLIB) $@ || true) >/dev/null 2>&1

install: protoc
	cp $< $(BIN_DIR)
	export PATH=$(PATH):$(BIN_DIR)
clean:
	-@rm *.a *.d *.o protoc 2>/dev/null
