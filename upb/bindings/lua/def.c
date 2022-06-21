/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "upb/def.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "lauxlib.h"
#include "upb/bindings/lua/upb.h"
#include "upb/reflection.h"

#define LUPB_ENUMDEF "lupb.enumdef"
#define LUPB_ENUMVALDEF "lupb.enumvaldef"
#define LUPB_FIELDDEF "lupb.fielddef"
#define LUPB_FILEDEF "lupb.filedef"
#define LUPB_MSGDEF "lupb.msgdef"
#define LUPB_ONEOFDEF "lupb.oneof"
#define LUPB_SYMTAB "lupb.defpool"
#define LUPB_OBJCACHE "lupb.objcache"

static void lupb_DefPool_pushwrapper(lua_State* L, int narg, const void* def,
                                     const char* type);

/* lupb_wrapper ***************************************************************/

/* Wrappers around upb def objects.  The userval contains a reference to the
 * defpool. */

#define LUPB_SYMTAB_INDEX 1

typedef struct {
  const void* def; /* upb_MessageDef, upb_EnumDef, upb_OneofDef, etc. */
} lupb_wrapper;

static const void* lupb_wrapper_check(lua_State* L, int narg,
                                      const char* type) {
  lupb_wrapper* w = luaL_checkudata(L, narg, type);
  return w->def;
}

static void lupb_wrapper_pushdefpool(lua_State* L, int narg) {
  lua_getiuservalue(L, narg, LUPB_SYMTAB_INDEX);
}

/* lupb_wrapper_pushwrapper()
 *
 * For a given def wrapper at index |narg|, pushes a wrapper for the given |def|
 * and the given |type|.  The new wrapper will be part of the same defpool. */
static void lupb_wrapper_pushwrapper(lua_State* L, int narg, const void* def,
                                     const char* type) {
  lupb_wrapper_pushdefpool(L, narg);
  lupb_DefPool_pushwrapper(L, -1, def, type);
  lua_replace(L, -2); /* Remove defpool from stack. */
}

/* lupb_MessageDef_pushsubmsgdef()
 *
 * Pops the msgdef wrapper at the top of the stack and replaces it with a msgdef
 * wrapper for field |f| of this msgdef (submsg may not be direct, for example
 * it may be the submessage of the map value).
 */
void lupb_MessageDef_pushsubmsgdef(lua_State* L, const upb_FieldDef* f) {
  const upb_MessageDef* m = upb_FieldDef_MessageSubDef(f);
  assert(m);
  lupb_wrapper_pushwrapper(L, -1, m, LUPB_MSGDEF);
  lua_replace(L, -2); /* Replace msgdef with submsgdef. */
}

/* lupb_FieldDef **************************************************************/

const upb_FieldDef* lupb_FieldDef_check(lua_State* L, int narg) {
  return lupb_wrapper_check(L, narg, LUPB_FIELDDEF);
}

static int lupb_FieldDef_ContainingOneof(lua_State* L) {
  const upb_FieldDef* f = lupb_FieldDef_check(L, 1);
  const upb_OneofDef* o = upb_FieldDef_ContainingOneof(f);
  lupb_wrapper_pushwrapper(L, 1, o, LUPB_ONEOFDEF);
  return 1;
}

static int lupb_FieldDef_ContainingType(lua_State* L) {
  const upb_FieldDef* f = lupb_FieldDef_check(L, 1);
  const upb_MessageDef* m = upb_FieldDef_ContainingType(f);
  lupb_wrapper_pushwrapper(L, 1, m, LUPB_MSGDEF);
  return 1;
}

static int lupb_FieldDef_Default(lua_State* L) {
  const upb_FieldDef* f = lupb_FieldDef_check(L, 1);
  upb_CType type = upb_FieldDef_CType(f);
  if (type == kUpb_CType_Message) {
    return luaL_error(L, "Message fields do not have explicit defaults.");
  }
  lupb_pushmsgval(L, 0, type, upb_FieldDef_Default(f));
  return 1;
}

static int lupb_FieldDef_Type(lua_State* L) {
  const upb_FieldDef* f = lupb_FieldDef_check(L, 1);
  lua_pushnumber(L, upb_FieldDef_Type(f));
  return 1;
}

static int lupb_FieldDef_HasSubDef(lua_State* L) {
  const upb_FieldDef* f = lupb_FieldDef_check(L, 1);
  lua_pushboolean(L, upb_FieldDef_HasSubDef(f));
  return 1;
}

static int lupb_FieldDef_Index(lua_State* L) {
  const upb_FieldDef* f = lupb_FieldDef_check(L, 1);
  lua_pushinteger(L, upb_FieldDef_Index(f));
  return 1;
}

static int lupb_FieldDef_IsExtension(lua_State* L) {
  const upb_FieldDef* f = lupb_FieldDef_check(L, 1);
  lua_pushboolean(L, upb_FieldDef_IsExtension(f));
  return 1;
}

static int lupb_FieldDef_Label(lua_State* L) {
  const upb_FieldDef* f = lupb_FieldDef_check(L, 1);
  lua_pushinteger(L, upb_FieldDef_Label(f));
  return 1;
}

static int lupb_FieldDef_Name(lua_State* L) {
  const upb_FieldDef* f = lupb_FieldDef_check(L, 1);
  lua_pushstring(L, upb_FieldDef_Name(f));
  return 1;
}

static int lupb_FieldDef_Number(lua_State* L) {
  const upb_FieldDef* f = lupb_FieldDef_check(L, 1);
  int32_t num = upb_FieldDef_Number(f);
  if (num) {
    lua_pushinteger(L, num);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int lupb_FieldDef_IsPacked(lua_State* L) {
  const upb_FieldDef* f = lupb_FieldDef_check(L, 1);
  lua_pushboolean(L, upb_FieldDef_IsPacked(f));
  return 1;
}

static int lupb_FieldDef_MessageSubDef(lua_State* L) {
  const upb_FieldDef* f = lupb_FieldDef_check(L, 1);
  const upb_MessageDef* m = upb_FieldDef_MessageSubDef(f);
  lupb_wrapper_pushwrapper(L, 1, m, LUPB_MSGDEF);
  return 1;
}

static int lupb_FieldDef_EnumSubDef(lua_State* L) {
  const upb_FieldDef* f = lupb_FieldDef_check(L, 1);
  const upb_EnumDef* e = upb_FieldDef_EnumSubDef(f);
  lupb_wrapper_pushwrapper(L, 1, e, LUPB_ENUMDEF);
  return 1;
}

static int lupb_FieldDef_CType(lua_State* L) {
  const upb_FieldDef* f = lupb_FieldDef_check(L, 1);
  lua_pushinteger(L, upb_FieldDef_CType(f));
  return 1;
}

static const struct luaL_Reg lupb_FieldDef_m[] = {
    {"containing_oneof", lupb_FieldDef_ContainingOneof},
    {"containing_type", lupb_FieldDef_ContainingType},
    {"default", lupb_FieldDef_Default},
    {"descriptor_type", lupb_FieldDef_Type},
    {"has_subdef", lupb_FieldDef_HasSubDef},
    {"index", lupb_FieldDef_Index},
    {"is_extension", lupb_FieldDef_IsExtension},
    {"label", lupb_FieldDef_Label},
    {"name", lupb_FieldDef_Name},
    {"number", lupb_FieldDef_Number},
    {"packed", lupb_FieldDef_IsPacked},
    {"msgsubdef", lupb_FieldDef_MessageSubDef},
    {"enumsubdef", lupb_FieldDef_EnumSubDef},
    {"type", lupb_FieldDef_CType},
    {NULL, NULL}};

/* lupb_OneofDef **************************************************************/

const upb_OneofDef* lupb_OneofDef_check(lua_State* L, int narg) {
  return lupb_wrapper_check(L, narg, LUPB_ONEOFDEF);
}

static int lupb_OneofDef_ContainingType(lua_State* L) {
  const upb_OneofDef* o = lupb_OneofDef_check(L, 1);
  const upb_MessageDef* m = upb_OneofDef_ContainingType(o);
  lupb_wrapper_pushwrapper(L, 1, m, LUPB_MSGDEF);
  return 1;
}

static int lupb_OneofDef_Field(lua_State* L) {
  const upb_OneofDef* o = lupb_OneofDef_check(L, 1);
  int32_t idx = lupb_checkint32(L, 2);
  int count = upb_OneofDef_FieldCount(o);

  if (idx < 0 || idx >= count) {
    const char* msg =
        lua_pushfstring(L, "index %d exceeds field count %d", idx, count);
    return luaL_argerror(L, 2, msg);
  }

  lupb_wrapper_pushwrapper(L, 1, upb_OneofDef_Field(o, idx), LUPB_FIELDDEF);
  return 1;
}

static int lupb_oneofiter_next(lua_State* L) {
  const upb_OneofDef* o = lupb_OneofDef_check(L, lua_upvalueindex(1));
  int* index = lua_touserdata(L, lua_upvalueindex(2));
  const upb_FieldDef* f;
  if (*index == upb_OneofDef_FieldCount(o)) return 0;
  f = upb_OneofDef_Field(o, (*index)++);
  lupb_wrapper_pushwrapper(L, lua_upvalueindex(1), f, LUPB_FIELDDEF);
  return 1;
}

static int lupb_OneofDef_Fields(lua_State* L) {
  int* index = lua_newuserdata(L, sizeof(int));
  lupb_OneofDef_check(L, 1);
  *index = 0;

  /* Closure upvalues are: oneofdef, index. */
  lua_pushcclosure(L, &lupb_oneofiter_next, 2);
  return 1;
}

static int lupb_OneofDef_len(lua_State* L) {
  const upb_OneofDef* o = lupb_OneofDef_check(L, 1);
  lua_pushinteger(L, upb_OneofDef_FieldCount(o));
  return 1;
}

/* lupb_OneofDef_lookupfield()
 *
 * Handles:
 *   oneof.lookup_field(field_number)
 *   oneof.lookup_field(field_name)
 */
static int lupb_OneofDef_lookupfield(lua_State* L) {
  const upb_OneofDef* o = lupb_OneofDef_check(L, 1);
  const upb_FieldDef* f;

  switch (lua_type(L, 2)) {
    case LUA_TNUMBER:
      f = upb_OneofDef_LookupNumber(o, lua_tointeger(L, 2));
      break;
    case LUA_TSTRING:
      f = upb_OneofDef_LookupName(o, lua_tostring(L, 2));
      break;
    default: {
      const char* msg = lua_pushfstring(L, "number or string expected, got %s",
                                        luaL_typename(L, 2));
      return luaL_argerror(L, 2, msg);
    }
  }

  lupb_wrapper_pushwrapper(L, 1, f, LUPB_FIELDDEF);
  return 1;
}

static int lupb_OneofDef_Name(lua_State* L) {
  const upb_OneofDef* o = lupb_OneofDef_check(L, 1);
  lua_pushstring(L, upb_OneofDef_Name(o));
  return 1;
}

static const struct luaL_Reg lupb_OneofDef_m[] = {
    {"containing_type", lupb_OneofDef_ContainingType},
    {"field", lupb_OneofDef_Field},
    {"fields", lupb_OneofDef_Fields},
    {"lookup_field", lupb_OneofDef_lookupfield},
    {"name", lupb_OneofDef_Name},
    {NULL, NULL}};

static const struct luaL_Reg lupb_OneofDef_mm[] = {{"__len", lupb_OneofDef_len},
                                                   {NULL, NULL}};

/* lupb_MessageDef
 * ****************************************************************/

typedef struct {
  const upb_MessageDef* md;
} lupb_MessageDef;

const upb_MessageDef* lupb_MessageDef_check(lua_State* L, int narg) {
  return lupb_wrapper_check(L, narg, LUPB_MSGDEF);
}

static int lupb_MessageDef_FieldCount(lua_State* L) {
  const upb_MessageDef* m = lupb_MessageDef_check(L, 1);
  lua_pushinteger(L, upb_MessageDef_FieldCount(m));
  return 1;
}

static int lupb_MessageDef_OneofCount(lua_State* L) {
  const upb_MessageDef* m = lupb_MessageDef_check(L, 1);
  lua_pushinteger(L, upb_MessageDef_OneofCount(m));
  return 1;
}

static bool lupb_MessageDef_pushnested(lua_State* L, int msgdef, int name) {
  const upb_MessageDef* m = lupb_MessageDef_check(L, msgdef);
  lupb_wrapper_pushdefpool(L, msgdef);
  upb_DefPool* defpool = lupb_DefPool_check(L, -1);
  lua_pop(L, 1);

  /* Construct full package.Message.SubMessage name. */
  lua_pushstring(L, upb_MessageDef_FullName(m));
  lua_pushstring(L, ".");
  lua_pushvalue(L, name);
  lua_concat(L, 3);
  const char* nested_name = lua_tostring(L, -1);

  /* Try lookup. */
  const upb_MessageDef* nested =
      upb_DefPool_FindMessageByName(defpool, nested_name);
  if (!nested) return false;
  lupb_wrapper_pushwrapper(L, msgdef, nested, LUPB_MSGDEF);
  return true;
}

/* lupb_MessageDef_Field()
 *
 * Handles:
 *   msg.field(field_number) -> fielddef
 *   msg.field(field_name) -> fielddef
 */
static int lupb_MessageDef_Field(lua_State* L) {
  const upb_MessageDef* m = lupb_MessageDef_check(L, 1);
  const upb_FieldDef* f;

  switch (lua_type(L, 2)) {
    case LUA_TNUMBER:
      f = upb_MessageDef_FindFieldByNumber(m, lua_tointeger(L, 2));
      break;
    case LUA_TSTRING:
      f = upb_MessageDef_FindFieldByName(m, lua_tostring(L, 2));
      break;
    default: {
      const char* msg = lua_pushfstring(L, "number or string expected, got %s",
                                        luaL_typename(L, 2));
      return luaL_argerror(L, 2, msg);
    }
  }

  lupb_wrapper_pushwrapper(L, 1, f, LUPB_FIELDDEF);
  return 1;
}

/* lupb_MessageDef_FindByNameWithSize()
 *
 * Handles:
 *   msg.lookup_name(name) -> fielddef or oneofdef
 */
static int lupb_MessageDef_FindByNameWithSize(lua_State* L) {
  const upb_MessageDef* m = lupb_MessageDef_check(L, 1);
  const upb_FieldDef* f;
  const upb_OneofDef* o;

  if (!upb_MessageDef_FindByName(m, lua_tostring(L, 2), &f, &o)) {
    lua_pushnil(L);
  } else if (o) {
    lupb_wrapper_pushwrapper(L, 1, o, LUPB_ONEOFDEF);
  } else {
    lupb_wrapper_pushwrapper(L, 1, f, LUPB_FIELDDEF);
  }

  return 1;
}

/* lupb_MessageDef_Name()
 *
 * Handles:
 *   msg.name() -> string
 */
static int lupb_MessageDef_Name(lua_State* L) {
  const upb_MessageDef* m = lupb_MessageDef_check(L, 1);
  lua_pushstring(L, upb_MessageDef_Name(m));
  return 1;
}

static int lupb_msgfielditer_next(lua_State* L) {
  const upb_MessageDef* m = lupb_MessageDef_check(L, lua_upvalueindex(1));
  int* index = lua_touserdata(L, lua_upvalueindex(2));
  const upb_FieldDef* f;
  if (*index == upb_MessageDef_FieldCount(m)) return 0;
  f = upb_MessageDef_Field(m, (*index)++);
  lupb_wrapper_pushwrapper(L, lua_upvalueindex(1), f, LUPB_FIELDDEF);
  return 1;
}

static int lupb_MessageDef_Fields(lua_State* L) {
  int* index = lua_newuserdata(L, sizeof(int));
  lupb_MessageDef_check(L, 1);
  *index = 0;

  /* Closure upvalues are: msgdef, index. */
  lua_pushcclosure(L, &lupb_msgfielditer_next, 2);
  return 1;
}

static int lupb_MessageDef_File(lua_State* L) {
  const upb_MessageDef* m = lupb_MessageDef_check(L, 1);
  const upb_FileDef* file = upb_MessageDef_File(m);
  lupb_wrapper_pushwrapper(L, 1, file, LUPB_FILEDEF);
  return 1;
}

static int lupb_MessageDef_FullName(lua_State* L) {
  const upb_MessageDef* m = lupb_MessageDef_check(L, 1);
  lua_pushstring(L, upb_MessageDef_FullName(m));
  return 1;
}

static int lupb_MessageDef_index(lua_State* L) {
  if (!lupb_MessageDef_pushnested(L, 1, 2)) {
    luaL_error(L, "No such nested message");
  }
  return 1;
}

static int lupb_msgoneofiter_next(lua_State* L) {
  const upb_MessageDef* m = lupb_MessageDef_check(L, lua_upvalueindex(1));
  int* index = lua_touserdata(L, lua_upvalueindex(2));
  const upb_OneofDef* o;
  if (*index == upb_MessageDef_OneofCount(m)) return 0;
  o = upb_MessageDef_Oneof(m, (*index)++);
  lupb_wrapper_pushwrapper(L, lua_upvalueindex(1), o, LUPB_ONEOFDEF);
  return 1;
}

static int lupb_MessageDef_Oneofs(lua_State* L) {
  int* index = lua_newuserdata(L, sizeof(int));
  lupb_MessageDef_check(L, 1);
  *index = 0;

  /* Closure upvalues are: msgdef, index. */
  lua_pushcclosure(L, &lupb_msgoneofiter_next, 2);
  return 1;
}

static int lupb_MessageDef_IsMapEntry(lua_State* L) {
  const upb_MessageDef* m = lupb_MessageDef_check(L, 1);
  lua_pushboolean(L, upb_MessageDef_IsMapEntry(m));
  return 1;
}

static int lupb_MessageDef_Syntax(lua_State* L) {
  const upb_MessageDef* m = lupb_MessageDef_check(L, 1);
  lua_pushinteger(L, upb_MessageDef_Syntax(m));
  return 1;
}

static int lupb_MessageDef_tostring(lua_State* L) {
  const upb_MessageDef* m = lupb_MessageDef_check(L, 1);
  lua_pushfstring(L, "<upb.MessageDef name=%s, field_count=%d>",
                  upb_MessageDef_FullName(m),
                  (int)upb_MessageDef_FieldCount(m));
  return 1;
}

static const struct luaL_Reg lupb_MessageDef_mm[] = {
    {"__call", lupb_MessageDef_call},
    {"__index", lupb_MessageDef_index},
    {"__len", lupb_MessageDef_FieldCount},
    {"__tostring", lupb_MessageDef_tostring},
    {NULL, NULL}};

static const struct luaL_Reg lupb_MessageDef_m[] = {
    {"field", lupb_MessageDef_Field},
    {"fields", lupb_MessageDef_Fields},
    {"field_count", lupb_MessageDef_FieldCount},
    {"file", lupb_MessageDef_File},
    {"full_name", lupb_MessageDef_FullName},
    {"lookup_name", lupb_MessageDef_FindByNameWithSize},
    {"name", lupb_MessageDef_Name},
    {"oneof_count", lupb_MessageDef_OneofCount},
    {"oneofs", lupb_MessageDef_Oneofs},
    {"syntax", lupb_MessageDef_Syntax},
    {"_map_entry", lupb_MessageDef_IsMapEntry},
    {NULL, NULL}};

/* lupb_EnumDef ***************************************************************/

const upb_EnumDef* lupb_EnumDef_check(lua_State* L, int narg) {
  return lupb_wrapper_check(L, narg, LUPB_ENUMDEF);
}

static int lupb_EnumDef_len(lua_State* L) {
  const upb_EnumDef* e = lupb_EnumDef_check(L, 1);
  lua_pushinteger(L, upb_EnumDef_ValueCount(e));
  return 1;
}

static int lupb_EnumDef_File(lua_State* L) {
  const upb_EnumDef* e = lupb_EnumDef_check(L, 1);
  const upb_FileDef* file = upb_EnumDef_File(e);
  lupb_wrapper_pushwrapper(L, 1, file, LUPB_FILEDEF);
  return 1;
}

/* lupb_EnumDef_Value()
 *
 * Handles:
 *   enum.value(number) -> enumval
 *   enum.value(name) -> enumval
 */
static int lupb_EnumDef_Value(lua_State* L) {
  const upb_EnumDef* e = lupb_EnumDef_check(L, 1);
  const upb_EnumValueDef* ev;

  switch (lua_type(L, 2)) {
    case LUA_TNUMBER:
      ev = upb_EnumDef_FindValueByNumber(e, lupb_checkint32(L, 2));
      break;
    case LUA_TSTRING:
      ev = upb_EnumDef_FindValueByName(e, lua_tostring(L, 2));
      break;
    default: {
      const char* msg = lua_pushfstring(L, "number or string expected, got %s",
                                        luaL_typename(L, 2));
      return luaL_argerror(L, 2, msg);
    }
  }

  lupb_wrapper_pushwrapper(L, 1, ev, LUPB_ENUMVALDEF);
  return 1;
}

static const struct luaL_Reg lupb_EnumDef_mm[] = {{"__len", lupb_EnumDef_len},
                                                  {NULL, NULL}};

static const struct luaL_Reg lupb_EnumDef_m[] = {
    {"file", lupb_EnumDef_File}, {"value", lupb_EnumDef_Value}, {NULL, NULL}};

/* lupb_EnumValueDef
 * ************************************************************/

const upb_EnumValueDef* lupb_enumvaldef_check(lua_State* L, int narg) {
  return lupb_wrapper_check(L, narg, LUPB_ENUMVALDEF);
}

static int lupb_EnumValueDef_Enum(lua_State* L) {
  const upb_EnumValueDef* ev = lupb_enumvaldef_check(L, 1);
  const upb_EnumDef* e = upb_EnumValueDef_Enum(ev);
  lupb_wrapper_pushwrapper(L, 1, e, LUPB_ENUMDEF);
  return 1;
}

static int lupb_EnumValueDef_FullName(lua_State* L) {
  const upb_EnumValueDef* ev = lupb_enumvaldef_check(L, 1);
  lua_pushstring(L, upb_EnumValueDef_FullName(ev));
  return 1;
}

static int lupb_EnumValueDef_Name(lua_State* L) {
  const upb_EnumValueDef* ev = lupb_enumvaldef_check(L, 1);
  lua_pushstring(L, upb_EnumValueDef_Name(ev));
  return 1;
}

static int lupb_EnumValueDef_Number(lua_State* L) {
  const upb_EnumValueDef* ev = lupb_enumvaldef_check(L, 1);
  lupb_pushint32(L, upb_EnumValueDef_Number(ev));
  return 1;
}

static const struct luaL_Reg lupb_enumvaldef_m[] = {
    {"enum", lupb_EnumValueDef_Enum},
    {"full_name", lupb_EnumValueDef_FullName},
    {"name", lupb_EnumValueDef_Name},
    {"number", lupb_EnumValueDef_Number},
    {NULL, NULL}};

/* lupb_FileDef ***************************************************************/

const upb_FileDef* lupb_FileDef_check(lua_State* L, int narg) {
  return lupb_wrapper_check(L, narg, LUPB_FILEDEF);
}

static int lupb_FileDef_Dependency(lua_State* L) {
  const upb_FileDef* f = lupb_FileDef_check(L, 1);
  int index = luaL_checkint(L, 2);
  const upb_FileDef* dep = upb_FileDef_Dependency(f, index);
  lupb_wrapper_pushwrapper(L, 1, dep, LUPB_FILEDEF);
  return 1;
}

static int lupb_FileDef_DependencyCount(lua_State* L) {
  const upb_FileDef* f = lupb_FileDef_check(L, 1);
  lua_pushnumber(L, upb_FileDef_DependencyCount(f));
  return 1;
}

static int lupb_FileDef_enum(lua_State* L) {
  const upb_FileDef* f = lupb_FileDef_check(L, 1);
  int index = luaL_checkint(L, 2);
  const upb_EnumDef* e = upb_FileDef_TopLevelEnum(f, index);
  lupb_wrapper_pushwrapper(L, 1, e, LUPB_ENUMDEF);
  return 1;
}

static int lupb_FileDef_enumcount(lua_State* L) {
  const upb_FileDef* f = lupb_FileDef_check(L, 1);
  lua_pushnumber(L, upb_FileDef_TopLevelEnumCount(f));
  return 1;
}

static int lupb_FileDef_msg(lua_State* L) {
  const upb_FileDef* f = lupb_FileDef_check(L, 1);
  int index = luaL_checkint(L, 2);
  const upb_MessageDef* m = upb_FileDef_TopLevelMessage(f, index);
  lupb_wrapper_pushwrapper(L, 1, m, LUPB_MSGDEF);
  return 1;
}

static int lupb_FileDef_msgcount(lua_State* L) {
  const upb_FileDef* f = lupb_FileDef_check(L, 1);
  lua_pushnumber(L, upb_FileDef_TopLevelMessageCount(f));
  return 1;
}

static int lupb_FileDef_Name(lua_State* L) {
  const upb_FileDef* f = lupb_FileDef_check(L, 1);
  lua_pushstring(L, upb_FileDef_Name(f));
  return 1;
}

static int lupb_FileDef_Package(lua_State* L) {
  const upb_FileDef* f = lupb_FileDef_check(L, 1);
  lua_pushstring(L, upb_FileDef_Package(f));
  return 1;
}

static int lupb_FileDef_Pool(lua_State* L) {
  const upb_FileDef* f = lupb_FileDef_check(L, 1);
  const upb_DefPool* defpool = upb_FileDef_Pool(f);
  lupb_wrapper_pushwrapper(L, 1, defpool, LUPB_SYMTAB);
  return 1;
}

static int lupb_FileDef_Syntax(lua_State* L) {
  const upb_FileDef* f = lupb_FileDef_check(L, 1);
  lua_pushnumber(L, upb_FileDef_Syntax(f));
  return 1;
}

static const struct luaL_Reg lupb_FileDef_m[] = {
    {"dep", lupb_FileDef_Dependency},
    {"depcount", lupb_FileDef_DependencyCount},
    {"enum", lupb_FileDef_enum},
    {"enumcount", lupb_FileDef_enumcount},
    {"msg", lupb_FileDef_msg},
    {"msgcount", lupb_FileDef_msgcount},
    {"name", lupb_FileDef_Name},
    {"package", lupb_FileDef_Package},
    {"defpool", lupb_FileDef_Pool},
    {"syntax", lupb_FileDef_Syntax},
    {NULL, NULL}};

/* lupb_DefPool
 * ****************************************************************/

/* The defpool owns all defs.  Thus GC-rooting the defpool ensures that all
 * underlying defs stay alive.
 *
 * The defpool's userval is a cache of def* -> object. */

#define LUPB_CACHE_INDEX 1

typedef struct {
  upb_DefPool* defpool;
} lupb_DefPool;

upb_DefPool* lupb_DefPool_check(lua_State* L, int narg) {
  lupb_DefPool* ldefpool = luaL_checkudata(L, narg, LUPB_SYMTAB);
  if (!ldefpool->defpool) {
    luaL_error(L, "called into dead object");
  }
  return ldefpool->defpool;
}

void lupb_DefPool_pushwrapper(lua_State* L, int narg, const void* def,
                              const char* type) {
  narg = lua_absindex(L, narg);
  assert(luaL_testudata(L, narg, LUPB_SYMTAB));

  if (def == NULL) {
    lua_pushnil(L);
    return;
  }

  lua_getiuservalue(L, narg, LUPB_CACHE_INDEX); /* Get cache. */

  /* Index by "def" pointer. */
  lua_rawgetp(L, -1, def);

  /* Stack is now: cache, cached value. */
  if (lua_isnil(L, -1)) {
    /* Create new wrapper. */
    lupb_wrapper* w = lupb_newuserdata(L, sizeof(*w), 1, type);
    w->def = def;
    lua_replace(L, -2); /* Replace nil */

    /* Set defpool as userval. */
    lua_pushvalue(L, narg);
    lua_setiuservalue(L, -2, LUPB_SYMTAB_INDEX);

    /* Add wrapper to the the cache. */
    lua_pushvalue(L, -1);
    lua_rawsetp(L, -3, def);
  }

  lua_replace(L, -2); /* Remove cache, leaving only the wrapper. */
}

/* upb_DefPool_New()
 *
 * Handles:
 *   upb.DefPool() -> <new instance>
 */
static int lupb_DefPool_New(lua_State* L) {
  lupb_DefPool* ldefpool =
      lupb_newuserdata(L, sizeof(*ldefpool), 1, LUPB_SYMTAB);
  ldefpool->defpool = upb_DefPool_New();

  /* Create our object cache. */
  lua_newtable(L);

  /* Cache metatable: specifies that values are weak. */
  lua_createtable(L, 0, 1);
  lua_pushstring(L, "v");
  lua_setfield(L, -2, "__mode");
  lua_setmetatable(L, -2);

  /* Put the defpool itself in the cache metatable. */
  lua_pushvalue(L, -2);
  lua_rawsetp(L, -2, ldefpool->defpool);

  /* Set the cache as our userval. */
  lua_setiuservalue(L, -2, LUPB_CACHE_INDEX);

  return 1;
}

static int lupb_DefPool_gc(lua_State* L) {
  lupb_DefPool* ldefpool = luaL_checkudata(L, 1, LUPB_SYMTAB);
  upb_DefPool_Free(ldefpool->defpool);
  ldefpool->defpool = NULL;
  return 0;
}

static int lupb_DefPool_AddFile(lua_State* L) {
  size_t len;
  upb_DefPool* s = lupb_DefPool_check(L, 1);
  const char* str = luaL_checklstring(L, 2, &len);
  upb_Arena* arena = lupb_Arena_pushnew(L);
  const google_protobuf_FileDescriptorProto* file;
  const upb_FileDef* file_def;
  upb_Status status;

  upb_Status_Clear(&status);
  file = google_protobuf_FileDescriptorProto_parse(str, len, arena);

  if (!file) {
    luaL_argerror(L, 2, "failed to parse descriptor");
  }

  file_def = upb_DefPool_AddFile(s, file, &status);
  lupb_checkstatus(L, &status);

  lupb_DefPool_pushwrapper(L, 1, file_def, LUPB_FILEDEF);

  return 1;
}

static int lupb_DefPool_addset(lua_State* L) {
  size_t i, n, len;
  const google_protobuf_FileDescriptorProto* const* files;
  google_protobuf_FileDescriptorSet* set;
  upb_DefPool* s = lupb_DefPool_check(L, 1);
  const char* str = luaL_checklstring(L, 2, &len);
  upb_Arena* arena = lupb_Arena_pushnew(L);
  upb_Status status;

  upb_Status_Clear(&status);
  set = google_protobuf_FileDescriptorSet_parse(str, len, arena);

  if (!set) {
    luaL_argerror(L, 2, "failed to parse descriptor");
  }

  files = google_protobuf_FileDescriptorSet_file(set, &n);
  for (i = 0; i < n; i++) {
    upb_DefPool_AddFile(s, files[i], &status);
    lupb_checkstatus(L, &status);
  }

  return 0;
}

static int lupb_DefPool_FindMessageByName(lua_State* L) {
  const upb_DefPool* s = lupb_DefPool_check(L, 1);
  const upb_MessageDef* m =
      upb_DefPool_FindMessageByName(s, luaL_checkstring(L, 2));
  lupb_DefPool_pushwrapper(L, 1, m, LUPB_MSGDEF);
  return 1;
}

static int lupb_DefPool_FindEnumByName(lua_State* L) {
  const upb_DefPool* s = lupb_DefPool_check(L, 1);
  const upb_EnumDef* e = upb_DefPool_FindEnumByName(s, luaL_checkstring(L, 2));
  lupb_DefPool_pushwrapper(L, 1, e, LUPB_ENUMDEF);
  return 1;
}

static int lupb_DefPool_FindEnumByNameval(lua_State* L) {
  const upb_DefPool* s = lupb_DefPool_check(L, 1);
  const upb_EnumValueDef* e =
      upb_DefPool_FindEnumByNameval(s, luaL_checkstring(L, 2));
  lupb_DefPool_pushwrapper(L, 1, e, LUPB_ENUMVALDEF);
  return 1;
}

static int lupb_DefPool_tostring(lua_State* L) {
  lua_pushfstring(L, "<upb.DefPool>");
  return 1;
}

static const struct luaL_Reg lupb_DefPool_m[] = {
    {"add_file", lupb_DefPool_AddFile},
    {"add_set", lupb_DefPool_addset},
    {"lookup_msg", lupb_DefPool_FindMessageByName},
    {"lookup_enum", lupb_DefPool_FindEnumByName},
    {"lookup_enumval", lupb_DefPool_FindEnumByNameval},
    {NULL, NULL}};

static const struct luaL_Reg lupb_DefPool_mm[] = {
    {"__gc", lupb_DefPool_gc},
    {"__tostring", lupb_DefPool_tostring},
    {NULL, NULL}};

/* lupb toplevel **************************************************************/

static void lupb_setfieldi(lua_State* L, const char* field, int i) {
  lua_pushinteger(L, i);
  lua_setfield(L, -2, field);
}

static const struct luaL_Reg lupbdef_toplevel_m[] = {
    {"DefPool", lupb_DefPool_New}, {NULL, NULL}};

void lupb_def_registertypes(lua_State* L) {
  lupb_setfuncs(L, lupbdef_toplevel_m);

  /* Register types. */
  lupb_register_type(L, LUPB_ENUMDEF, lupb_EnumDef_m, lupb_EnumDef_mm);
  lupb_register_type(L, LUPB_ENUMVALDEF, lupb_enumvaldef_m, NULL);
  lupb_register_type(L, LUPB_FIELDDEF, lupb_FieldDef_m, NULL);
  lupb_register_type(L, LUPB_FILEDEF, lupb_FileDef_m, NULL);
  lupb_register_type(L, LUPB_MSGDEF, lupb_MessageDef_m, lupb_MessageDef_mm);
  lupb_register_type(L, LUPB_ONEOFDEF, lupb_OneofDef_m, lupb_OneofDef_mm);
  lupb_register_type(L, LUPB_SYMTAB, lupb_DefPool_m, lupb_DefPool_mm);

  /* Register constants. */
  lupb_setfieldi(L, "LABEL_OPTIONAL", kUpb_Label_Optional);
  lupb_setfieldi(L, "LABEL_REQUIRED", kUpb_Label_Required);
  lupb_setfieldi(L, "LABEL_REPEATED", kUpb_Label_Repeated);

  lupb_setfieldi(L, "TYPE_DOUBLE", kUpb_CType_Double);
  lupb_setfieldi(L, "TYPE_FLOAT", kUpb_CType_Float);
  lupb_setfieldi(L, "TYPE_INT64", kUpb_CType_Int64);
  lupb_setfieldi(L, "TYPE_UINT64", kUpb_CType_UInt64);
  lupb_setfieldi(L, "TYPE_INT32", kUpb_CType_Int32);
  lupb_setfieldi(L, "TYPE_BOOL", kUpb_CType_Bool);
  lupb_setfieldi(L, "TYPE_STRING", kUpb_CType_String);
  lupb_setfieldi(L, "TYPE_MESSAGE", kUpb_CType_Message);
  lupb_setfieldi(L, "TYPE_BYTES", kUpb_CType_Bytes);
  lupb_setfieldi(L, "TYPE_UINT32", kUpb_CType_UInt32);
  lupb_setfieldi(L, "TYPE_ENUM", kUpb_CType_Enum);

  lupb_setfieldi(L, "DESCRIPTOR_TYPE_DOUBLE", kUpb_FieldType_Double);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_FLOAT", kUpb_FieldType_Float);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_INT64", kUpb_FieldType_Int64);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_UINT64", kUpb_FieldType_UInt64);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_INT32", kUpb_FieldType_Int32);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_FIXED64", kUpb_FieldType_Fixed64);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_FIXED32", kUpb_FieldType_Fixed32);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_BOOL", kUpb_FieldType_Bool);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_STRING", kUpb_FieldType_String);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_GROUP", kUpb_FieldType_Group);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_MESSAGE", kUpb_FieldType_Message);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_BYTES", kUpb_FieldType_Bytes);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_UINT32", kUpb_FieldType_UInt32);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_ENUM", kUpb_FieldType_Enum);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_SFIXED32", kUpb_FieldType_SFixed32);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_SFIXED64", kUpb_FieldType_SFixed64);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_SINT32", kUpb_FieldType_SInt32);
  lupb_setfieldi(L, "DESCRIPTOR_TYPE_SINT64", kUpb_FieldType_SInt64);

  lupb_setfieldi(L, "SYNTAX_PROTO2", kUpb_Syntax_Proto2);
  lupb_setfieldi(L, "SYNTAX_PROTO3", kUpb_Syntax_Proto3);
}
