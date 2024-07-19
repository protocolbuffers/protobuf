ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

NAME=protobuf

DIST_BASE=$(PRODUCT_ROOT)/../

#$(INSTALL_ROOT_$(OS)) is pointing to $QNX_TARGET
#by default, unless it was manually re-routed to
#a staging area by setting both INSTALL_ROOT_nto
#and USE_INSTALL_ROOT
INSTALL_ROOT ?= $(INSTALL_ROOT_$(OS))

#A prefix path to use **on the target**. This is
#different from INSTALL_ROOT, which refers to a
#installation destination **on the host machine**.
#This prefix path may be exposed to the source code,
#the linker, or package discovery config files (.pc,
#CMake config modules, etc.). Default is /usr/local
PREFIX ?= /usr/local

#choose Release or Debug
CMAKE_BUILD_TYPE ?= Release

#override 'all' target to bypass the default QNX build system
ALL_DEPENDENCIES = protobuf_all
.PHONY: protobuf_all install check clean

CFLAGS += $(FLAGS) -D_XOPEN_SOURCE=700 -D_QNX_SOURCE

#Search paths for all of CMake's find_* functions --
#headers, libraries, etc.
#
#$(QNX_TARGET): for architecture-agnostic files shipped with SDP (e.g. headers)
#$(QNX_TARGET)/$(CPUVARDIR): for architecture-specific files in SDP
#$(INSTALL_ROOT)/$(CPUVARDIR): any packages that may have been installed in the staging area
CMAKE_FIND_ROOT_PATH := $(QNX_TARGET);$(QNX_TARGET)/$(CPUVARDIR);$(INSTALL_ROOT)/$(CPUVARDIR)

#Path to CMake modules; These are CMake files installed by other packages
#for downstreams to discover them automatically. We support discovering
#CMake-based packages from inside SDP or in the staging area.
#Note that CMake modules can automatically detect the prefix they are
#installed in.
CMAKE_MODULE_PATH := $(QNX_TARGET)/$(CPUVARDIR)/$(PREFIX)/lib/cmake;$(INSTALL_ROOT)/$(CPUVARDIR)/$(PREFIX)/lib/cmake

#Headers from INSTALL_ROOT need to be made available by default
#because CMake and pkg-config do not necessary add it automatically
#if the include path is "default"
CFLAGS += -I$(INSTALL_ROOT)/$(CPUVARDIR)/$(PREFIX)/include

CMAKE_COMMON_ARGS = -DCMAKE_TOOLCHAIN_FILE=$(PROJECT_ROOT)/qnx.nto.toolchain.cmake \
                    -DCMAKE_SYSTEM_PROCESSOR=$(CPUVARDIR) \
                    -DCMAKE_CXX_COMPILER_TARGET=gcc_nto$(CPUVARDIR) \
                    -DCMAKE_C_COMPILER_TARGET=gcc_nto$(CPUVARDIR) \
                    -DCMAKE_INSTALL_PREFIX="$(PREFIX)" \
                    -DCMAKE_STAGING_PREFIX="$(INSTALL_ROOT)/$(CPUVARDIR)/$(PREFIX)" \
                    -DCMAKE_MODULE_PATH="$(CMAKE_MODULE_PATH)" \
                    -DCMAKE_FIND_ROOT_PATH="$(CMAKE_FIND_ROOT_PATH)" \
                    -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
                    -DEXTRA_CMAKE_C_FLAGS="$(CFLAGS)" \
                    -DEXTRA_CMAKE_CXX_FLAGS="$(CFLAGS)" \
                    -DEXTRA_CMAKE_ASM_FLAGS="$(FLAGS)" \
                    -DEXTRA_CMAKE_LINKER_FLAGS="$(LDFLAGS)" \
                    -DBUILD_SHARED_LIBS=1 \
                    -Dprotobuf_USE_EXTERNAL_GTEST=OFF \
                    -Dprotobuf_INSTALL=ON \
                    -Dprotobuf_ABSOLUTE_TEST_PLUGIN_PATH=OFF

HOST_CMAKE_ARGS =   -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
                    -Dprotobuf_BUILD_TESTS=OFF \
                    -Dprotobuf_BUILD_LIBUPB=ON

HOST_PROTOC_PATH = $(PROJECT_ROOT)/host_protoc

include $(MKFILES_ROOT)/qtargets.mk

GENERATOR_ARGS ?= -j $(firstword $(JLEVEL) 1) --verbose

ifndef NO_TARGET_OVERRIDE

protobuf_all: protobuf protoc_target

protobuf: protoc_host
	mkdir -p protobuf
	cd protobuf && cmake $(CMAKE_COMMON_ARGS) -DWITH_PROTOC=$(HOST_PROTOC_PATH)/protoc -Dprotobuf_BUILD_LIBUPB=OFF $(DIST_BASE)
	cd protobuf && cmake --build . $(GENERATOR_ARGS)

protoc_target:
	mkdir -p protoc
	cd protoc && cmake $(CMAKE_COMMON_ARGS) -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_LIBUPB=ON $(DIST_BASE)
	cd protoc && cmake --build . $(GENERATOR_ARGS)

protoc_host:
	mkdir -p $(HOST_PROTOC_PATH)
	cd $(HOST_PROTOC_PATH) && \
	cmake $(HOST_CMAKE_ARGS) $(DIST_BASE) && \
    cmake --build . $(GENERATOR_ARGS)

install: protobuf_all
	cd protobuf && cmake --build . --target install $(GENERATOR_ARGS)
	cd protoc && cmake --build . --target install $(GENERATOR_ARGS)

clean: protobuf_host_tools_clean
	rm -rf protobuf
	rm -rf protoc

protobuf_host_tools_clean:
	rm -rf $(HOST_PROTOC_PATH)

uninstall:
endif

