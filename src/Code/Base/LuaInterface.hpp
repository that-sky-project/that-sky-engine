#ifndef __LUAINTERFACE_HPP__
#define __LUAINTERFACE_HPP__

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lstate.h>
}

#include "Base/Meta.hpp"

META_DECLARE_CLASS(lua_State)

int MetaLuaEq(
  lua_State *L);

int MetaLuaIndex(
  lua_State *L);

int MetaLuaNewImpl(
  lua_State *L);

int MetaLuaCast(
  lua_State *L);

void MetaLuaBindClass(
  const MetaClass *T,
  lua_State *L);

#endif
