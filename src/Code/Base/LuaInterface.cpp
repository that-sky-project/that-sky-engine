#include "LuaInterface.hpp"

bool IsDerivedFrom(MetaClass const*, MetaClass const*) {
  return false;
}

int MetaLuaEq(
  lua_State *L
) {
  void **obj1 = (void **)lua_touserdata(L, -1)
    , **obj2 = (void **)lua_touserdata(L, -2);
  int result = 0;

  if (obj1 && obj2) {
    if (lua_type(L, -1) != LUA_TLIGHTUSERDATA && lua_type(L, -2) != LUA_TLIGHTUSERDATA)
      result = *obj1 == *obj2;
  }

  lua_pushboolean(L, result);

  return 1;
}

int MetaLuaIndex(
  lua_State *L
) {
  lua_getmetatable(L, -2);
  lua_pushvalue(L, -2);
  lua_gettable(L, -2);
  return 1;
}

int MetaLuaToString(
  lua_State *L
) {
  // Access the userdata upvalue bound by MetaLuaBindClass.
  MetaClass *upval = (MetaClass *)lua_touserdata(L, lua_upvalueindex(1));
  skyAssert(upval);

  lua_pushstring(L, upval->m_name);

  return 1;
}

int MetaLuaCast(
  lua_State *L
) {
  Object **ppSourceObj, *t, *pBase;
  const MetaClass *upval, *mc, *sourceClass;
  const MetaType *sourceType;
  void *targetObj;

  // Upvalue points to current MetaClass, i.e. the type to be casted to.
  upval = (const MetaClass *)lua_touserdata(L, lua_upvalueindex(1));
  skyAssert(upval);

  // Read an object from the stack as an Object.
  mc = MetaClassImpl<Object>::Must_call_META_REGISTER_CLASS();
  ppSourceObj = (Object **)mc->ConstructByType(&t);
  mc->ReadType(L, 1, ppSourceObj);

  // I don't know why TGC wrote this, let's just copy.
  mc = mc->AsClass();

  pBase = (Object *)mc->Upcast(t);
  if (pBase)
    sourceType = GetMetaClassById(pBase->m_metaClassId);
  else
    sourceType = GetMetaType();

  sourceClass = sourceType->AsClass();
  if (sourceClass) {
    if (!ppSourceObj)
      goto PushNil;
  } else {
    sourceClass = GetMetaClass();
    if (!ppSourceObj) {
      goto PushNil;
    }
  }

  if (!IsDerivedFrom(sourceClass, upval)) {
PushNil:
    lua_pushnil(L);
    goto Ret;
  }

  targetObj = sourceClass->Downcast(pBase);
  sourceClass->WriteType(L, &targetObj);

Ret:
  mc->DestructByType(ppSourceObj);
  return 1;
}

class Heap;
META_REGISTER_CLASS(Heap);
class Heap: public Object {
public:
  Heap() {
    m_metaClassId = MetaClassImpl<Heap>::Must_call_META_REGISTER_CLASS()->m_globalId;
  }

  void *Allocate(
    size_t size,
    int tag,
    size_t align
  ) {
    return malloc(size);
  }

  void Free(void *p) {
    free(p);
  }
};

static constexpr int tag_LuaInterface = 0;

int MetaLuaNewImpl(
  lua_State *L
) {
  const MetaClass *upval, *metaClassHeap;
  Object *pObject;
  Heap **ppHeapObject, *pHeapObject;

  upval = (const MetaClass *)lua_touserdata(L, lua_upvalueindex(1));
  skyAssert(upval);

  metaClassHeap = MetaClassImpl<Heap>::Must_call_META_REGISTER_CLASS();
  ppHeapObject = (Heap **)metaClassHeap->ConstructByType(&pHeapObject);

  if (lua_type(L, 1) != LUA_TNONE)
    metaClassHeap->ReadType(L, 1, ppHeapObject);

  metaClassHeap->DynamicCast(
    &pHeapObject,
    ppHeapObject,
    metaClassHeap);

  if (pHeapObject) {
    void *payload = pHeapObject->Allocate(
      upval->SizeOfObject(),
      tag_LuaInterface,
      upval->AlignOfObject());
    pObject = (Object *)upval->ConstructObject(payload);
  } else {
    pObject = (Object *)upval->NewObject();
  }

  upval->WriteType(L, &pObject);
  metaClassHeap->DestructByType(ppHeapObject);

  return 1;
}

int MetaLuaDeleteImpl(
  lua_State *L
) {
  const MetaClass *upval, *metaClassHeap;
  Object **ppObject, *t;
  Heap **ppHeapObject, *pHeapObject;

  upval = (const MetaClass *)lua_touserdata(L, lua_upvalueindex(1));
  skyAssert(upval);

  ppObject = (Object **)upval->ConstructByType(&t);
  upval->ReadType(L, 1, ppObject);

  metaClassHeap = MetaClassImpl<Heap>::Must_call_META_REGISTER_CLASS();
  ppHeapObject = (Heap **)metaClassHeap->ConstructByType(&pHeapObject);

  if (lua_type(L, 2) != LUA_TNONE)
    metaClassHeap->ReadType(L, 2, ppHeapObject);

  metaClassHeap->DynamicCast(&pHeapObject, ppHeapObject, metaClassHeap);
  if (pHeapObject) {
    upval->DestructObject(t);
    pHeapObject->Free(*ppObject);
  } else {
    upval->DeleteObject(t);
  }
  metaClassHeap->DestructByType(ppHeapObject);
  upval->DestructByType(ppObject);

  return 0;
}

void MetaLuaBindClass(
  const MetaClass *T,
  lua_State *L
) {
  lua_createtable(L, 0, 0);
  lua_createtable(L, 0, 0);
  //MetaLuaBindClassMetaData(a1, L, 0xFFFFFFFF);
  lua_setfield(L, -2, "__metadata");
  lua_createtable(L, 0, 0);
  lua_setfield(L, -2, "__funs");
  //MetaLuaBindMemberFunctions((MetaClassImpl *)a1, L, -1);
  lua_createtable(L, 0, 0);
  lua_setfield(L, -2, "__vars");
  //MetaLuaBindMemberVariables((MetaClassImpl *)a1, L, -1);
  lua_pushlightuserdata(L, (void *)T);
  lua_pushcclosure(L, MetaLuaEq, 1);
  lua_setfield(L, -2, "__eq");
  lua_pushlightuserdata(L, (void *)T);
  lua_pushcclosure(L, MetaLuaIndex, 1);
  lua_setfield(L, -2, "__index");
  lua_pushlightuserdata(L, (void *)T);
  lua_pushcclosure(L, MetaLuaToString, 1);
  lua_setfield(L, -2, "__tostring");
  lua_pushlightuserdata(L, (void *)T);
  lua_pushcclosure(L, MetaLuaCast, 1);
  lua_setfield(L, -2, "cast");
  if (!T->IsAbstract()) {
    lua_pushlightuserdata(L, (void *)T);
    lua_pushcclosure(L, MetaLuaNewImpl, 1);
    lua_setfield(L, -2, "new");
  }
  lua_pushlightuserdata(L, (void *)T);
  lua_pushcclosure(L, MetaLuaDeleteImpl, 1);
  lua_setfield(L, -2, "delete");
  lua_pushstring(L, T->m_name);
  lua_setfield(L, -2, "__type");
  lua_pushinteger(L, T->SizeOfObject());
  lua_setfield(L, -2, "__sizeof");
  lua_pushinteger(L, T->AlignOfObject());
  lua_setfield(L, -2, "__alignof");
  lua_pushlightuserdata(L, (void *)T);
  lua_setfield(L, -2, "__metaclass");
  lua_setglobal(L, T->m_name);
}
