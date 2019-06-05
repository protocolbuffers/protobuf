/*
** Shared definitions for upb Lua modules.
*/

#ifndef UPB_LUA_UPB_H_
#define UPB_LUA_UPB_H_

#include "lauxlib.h"
#include "upb/def.h"
#include "upb/handlers.h"
#include "upb/msg.h"
#include "upb/msgfactory.h"

/* Lua 5.1/5.2 compatibility code. */
#if LUA_VERSION_NUM == 501

#define lua_rawlen lua_objlen

/* Lua >= 5.2's getuservalue/setuservalue functions do not exist in prior
 * versions but the older function lua_getfenv() can provide 100% of its
 * capabilities (the reverse is not true). */
#define lua_getuservalue(L, index) lua_getfenv(L, index)
#define lua_setuservalue(L, index) lua_setfenv(L, index)

void *luaL_testudata(lua_State *L, int ud, const char *tname);

#define lupb_setfuncs(L, l) luaL_register(L, NULL, l)

#elif LUA_VERSION_NUM == 502

int luaL_typerror(lua_State *L, int narg, const char *tname);

#define lupb_setfuncs(L, l) luaL_setfuncs(L, l, 0)

#else
#error Only Lua 5.1 and 5.2 are supported
#endif

#define lupb_assert(L, predicate) \
  if (!(predicate))               \
    luaL_error(L, "internal error: %s, %s:%d ", #predicate, __FILE__, __LINE__);

/* Function for initializing the core library.  This function is idempotent,
 * and should be called at least once before calling any of the functions that
 * construct core upb types. */
int luaopen_upb(lua_State *L);

/* Gets or creates a package table for a C module that is uniquely identified by
 * "ptr".  The easiest way to supply a unique "ptr" is to pass the address of a
 * static variable private in the module's .c file.
 *
 * If this module has already been registered in this lua_State, pushes it and
 * returns true.
 *
 * Otherwise, creates a new module table for this module with the given name,
 * pushes it, and registers the given top-level functions in it.  It also sets
 * it as a global variable, but only if the current version of Lua expects that
 * (ie Lua 5.1/LuaJIT).
 *
 * If "false" is returned, the caller is guaranteed that this lib has not been
 * registered in this Lua state before (regardless of any funny business the
 * user might have done to the global state), so the caller can safely perform
 * one-time initialization. */
bool lupb_openlib(lua_State *L, void *ptr, const char *name,
                  const luaL_Reg *funcs);

/* Custom check/push functions.  Unlike the Lua equivalents, they are pinned to
 * specific types (instead of lua_Number, etc), and do not allow any implicit
 * conversion or data loss. */
int64_t lupb_checkint64(lua_State *L, int narg);
int32_t lupb_checkint32(lua_State *L, int narg);
uint64_t lupb_checkuint64(lua_State *L, int narg);
uint32_t lupb_checkuint32(lua_State *L, int narg);
double lupb_checkdouble(lua_State *L, int narg);
float lupb_checkfloat(lua_State *L, int narg);
bool lupb_checkbool(lua_State *L, int narg);
const char *lupb_checkstring(lua_State *L, int narg, size_t *len);
const char *lupb_checkname(lua_State *L, int narg);

void lupb_pushint64(lua_State *L, int64_t val);
void lupb_pushint32(lua_State *L, int32_t val);
void lupb_pushuint64(lua_State *L, uint64_t val);
void lupb_pushuint32(lua_State *L, uint32_t val);
void lupb_pushdouble(lua_State *L, double val);
void lupb_pushfloat(lua_State *L, float val);

/* Registers a type with the given name, methods, and metamethods. */
void lupb_register_type(lua_State *L, const char *name, const luaL_Reg *m,
                        const luaL_Reg *mm);

/* Checks the given upb_status and throws a Lua error if it is not ok. */
void lupb_checkstatus(lua_State *L, upb_status *s);


/** From def.c. ***************************************************************/

upb_fieldtype_t lupb_checkfieldtype(lua_State *L, int narg);

const upb_msgdef *lupb_msgdef_check(lua_State *L, int narg);
const upb_enumdef *lupb_enumdef_check(lua_State *L, int narg);
const upb_fielddef *lupb_fielddef_check(lua_State *L, int narg);
upb_symtab *lupb_symtab_check(lua_State *L, int narg);

void lupb_def_registertypes(lua_State *L);


/** From msg.c. ***************************************************************/

struct lupb_msgclass;
typedef struct lupb_msgclass lupb_msgclass;

upb_arena *lupb_arena_check(lua_State *L, int narg);
int lupb_arena_new(lua_State *L);
upb_arena *lupb_arena_get(lua_State *L);
int lupb_msg_pushref(lua_State *L, int msgclass, void *msg);
const upb_msg *lupb_msg_checkmsg(lua_State *L, int narg,
                                 const lupb_msgclass *lmsgclass);
upb_msg *lupb_msg_checkmsg2(lua_State *L, int narg,
                            const upb_msglayout **layout);

const lupb_msgclass *lupb_msgclass_check(lua_State *L, int narg);
const upb_msglayout *lupb_msgclass_getlayout(lua_State *L, int narg);
const upb_msgdef *lupb_msgclass_getmsgdef(const lupb_msgclass *lmsgclass);
upb_msgfactory *lupb_msgclass_getfactory(const lupb_msgclass *lmsgclass);
void lupb_msg_registertypes(lua_State *L);

#endif  /* UPB_LUA_UPB_H_ */
