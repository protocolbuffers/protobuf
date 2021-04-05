# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Google LLC nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "liblua_headers",
    hdrs = [
        "src/lauxlib.h",
        "src/lua.h",
        "src/lua.hpp",
        "src/luaconf.h",
        "src/lualib.h",
    ],
    defines = ["LUA_USE_LINUX"],
    includes = ["src"],
)

cc_library(
    name = "liblua",
    srcs = [
        "src/lapi.c",
        "src/lapi.h",
        "src/lauxlib.c",
        "src/lauxlib.h",
        "src/lbaselib.c",
        "src/lbitlib.c",
        "src/lcode.c",
        "src/lcode.h",
        "src/lcorolib.c",
        "src/lctype.c",
        "src/lctype.h",
        "src/ldblib.c",
        "src/ldebug.c",
        "src/ldebug.h",
        "src/ldo.c",
        "src/ldo.h",
        "src/ldump.c",
        "src/lfunc.c",
        "src/lfunc.h",
        "src/lgc.c",
        "src/lgc.h",
        "src/linit.c",
        "src/liolib.c",
        "src/llex.c",
        "src/llex.h",
        "src/llimits.h",
        "src/lmathlib.c",
        "src/lmem.c",
        "src/lmem.h",
        "src/loadlib.c",
        "src/lobject.c",
        "src/lobject.h",
        "src/lopcodes.c",
        "src/lopcodes.h",
        "src/loslib.c",
        "src/lparser.c",
        "src/lparser.h",
        "src/lstate.c",
        "src/lstate.h",
        "src/lstring.c",
        "src/lstring.h",
        "src/lstrlib.c",
        "src/ltable.c",
        "src/ltable.h",
        "src/ltablib.c",
        "src/ltm.c",
        "src/ltm.h",
        "src/lundump.c",
        "src/lundump.h",
        "src/lvm.c",
        "src/lvm.h",
        "src/lzio.c",
        "src/lzio.h",
    ],
    hdrs = [
        "src/lauxlib.h",
        "src/lua.h",
        "src/lua.hpp",
        "src/luaconf.h",
        "src/lualib.h",
    ],
    defines = ["LUA_USE_LINUX"],
    includes = ["src"],
    linkopts = [
        "-lm",
        "-ldl",
    ],
)

cc_binary(
    name = "lua",
    srcs = [
        "src/lua.c",
    ],
    linkopts = [
        "-lreadline",
        "-rdynamic",
    ],
    deps = [
        ":liblua",
    ],
)
