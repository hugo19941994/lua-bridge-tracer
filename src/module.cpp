#include "lua_tracer.h"
#include "lua_span.h"
#include "lua_span_context.h"

#include <opentracing/dynamic_load.h>
#include <iostream>
#include <iterator>

extern "C" {
#include <lua.h>
#include <lua/lauxlib.h>
} // extern "C"

static void setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
  luaL_checkstack(L, nup+1, "too many upvalues");
  for (; l->name != NULL; l++) {  /* fill the table with given functions */
    int i;
    lua_pushstring(L, l->name);
    for (i = 0; i < nup; i++)  /* copy upvalues to the top */
      lua_pushvalue(L, -(nup+1));
    lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
    lua_settable(L, -(nup + 3));
  }
  lua_pop(L, nup);  /* remove upvalues */
}

static void make_lua_class(
    lua_State* L, const lua_bridge_tracer::LuaClassDescription& description) {
  luaL_newmetatable(L, description.metatable);

  if (description.free_function != nullptr) {
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, description.free_function);
    lua_settable(L, -3);
  }

  setfuncs(L, &*std::begin(description.methods), 0);

  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2); /* pushes the metatable */
  lua_settable(L, -3);  /* metatable.__index = metatable */

  if (description.name != nullptr) {
    lua_newtable(L);
    setfuncs(L, &*std::begin(description.functions), 0);
    lua_setglobal(L, description.name);
  }
}

extern "C" int luaopen_opentracing_bridge_tracer(lua_State* L) {
  make_lua_class(L, lua_bridge_tracer::LuaTracer::description);
  make_lua_class(L, lua_bridge_tracer::LuaSpan::description);
  make_lua_class(L, lua_bridge_tracer::LuaSpanContext::description);

  return 0;
}
