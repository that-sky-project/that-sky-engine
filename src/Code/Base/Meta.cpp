#include "Assert.hpp"
#include "Meta.hpp"

// Register a type.
#define META_REGISTER_TYPE(T)\
  template<>\
  const MetaType *GetMetaTypeByType<T>() {\
    return g_metaType_##T.m_self;\
  }

// Register a number type.
#define META_REGISTER_TYPE_NUMBER(T)\
  static MetaTypeNumber<T> g_metaType_##T{#T};\
  META_REGISTER_TYPE(T)

// Register a number type.
#define META_REGISTER_TYPE_STRING(T)\
  static MetaTypeString<T> g_metaType_##T{#T};\
  META_REGISTER_TYPE(T)

// ----------------------------------------------------------------------------
// [SECTION] MetaType
// ----------------------------------------------------------------------------

static MetaTypeBool g_metaType_bool{"bool"};
template<>
const MetaType *GetMetaTypeByType<bool>() {
  return g_metaType_bool.m_self;
}

META_REGISTER_TYPE_NUMBER(uint8_t);
META_REGISTER_TYPE_NUMBER(int8_t);
META_REGISTER_TYPE_NUMBER(uint16_t);
META_REGISTER_TYPE_NUMBER(int16_t);
META_REGISTER_TYPE_NUMBER(uint32_t);
META_REGISTER_TYPE_NUMBER(int32_t);
META_REGISTER_TYPE_NUMBER(uint64_t);
META_REGISTER_TYPE_NUMBER(int64_t);
META_REGISTER_TYPE_NUMBER(float);
META_REGISTER_TYPE_NUMBER(double);

META_REGISTER_TYPE_STRING(cstring);
META_REGISTER_TYPE_STRING(TgcString);

static MetaClassVoid g_metaType_void{"void"};

const MetaType *GetMetaType() {
  return (const MetaClassVoid *)g_metaType_void.m_self;
}

const MetaClass *GetMetaClass() {
  return GetMetaType()->AsClass();
}

// ----------------------------------------------------------------------------
// [SECTION] MetaClass
// ----------------------------------------------------------------------------

void MetaClass::DynamicCast(
  void *targetObject,
  void *sourceObject,
  const MetaType &sourceType
) const {

}

double MetaClass::ToNumber(void *) const {
  return 0;
}

const char *MetaClass::ToString(void *) const {
  return "";
}

const MetaClass *MetaClass::AsClass() const {
  return this;
}

void MetaClass::WriteType(
  lua_State *L,
  void *object
) const {
  Payload *ppObject = (Payload *)object
    , pObject = *ppObject
    , result;

 if (!pObject)
    return lua_pushnil(L);

  Object *pBase = (Object *)Upcast(pObject);
  if (pBase)
    result = GetMetaClassById(pBase->m_metaClassId)->Downcast(pBase);
  else
    result = pObject;

  *(Payload *)lua_newuserdata(L, 8) = result;
  lua_getglobal(L, m_name);
  lua_setmetatable(L, -2);
}

void MetaClass::ReadType(
  lua_State *L,
  int index,
  void *object
) const {
  Payload *ppObject = (Payload *)object
    , pObject;
  const char *err = nullptr;
  const MetaClass *metaClass;
  bool isDerived = false;
  char buffer[1088];

  if (!lua_type(L, index)) {
    // LUA_TNIL, directly return nullptr.
    *ppObject = nullptr;
    return;
  }

  pObject = lua_touserdata(L, index);
  if (!pObject || !lua_getmetatable(L, index)) {
    // Not a userdata (objects created by MetaSystem) or no metatable.
    err = lua_typename(L, lua_type(L, index));
    goto Err;
  }
  lua_getfield(L, -1, "__metaclass");
  metaClass = (const MetaClass *)lua_touserdata(L, -1);
  lua_settop(L, -3);
  if (!metaClass) {
    // No MetaClass.
    err = "<unknown userdata>";
    goto Err;
  }

  if (metaClass != this) {
    // Not the expected class.
    // Set the error message to the type name of recieved class name.
    err = metaClass->m_name;
    if (m_topoOrder == -1)
      // Not a valid topological order.
      goto Err;

    for (auto &it: m_baseTopoIdList) {
      // Find parent class. The expected type must have a valid topological
      // order, and the base class list of the actual type contains this order.
      if (it == m_topoOrder) {
        isDerived = true;
        break;
      }
    }

    if (!isDerived) {
Err:
      // Error handler.
      const char *msg = "!!!NULL!!!";
      if (err)
        msg = err;
      snprintf(buffer, 1024, "Expected %s, but got %s.", m_name, msg);
      lua_pushstring(L, buffer);
      lua_error(L);

      // lua_error never returns.
      return;
    }
  }

  DynamicCast(ppObject, pObject, *metaClass);
}

void MetaClass::SimpleCopy(
  void *target
) const {
  *(MetaType *)target = *(MetaType *)this;
  ((MetaClass *)target)->m_parent = m_parent;
}

// ----------------------------------------------------------------------------
// [SECTION] Object
// ----------------------------------------------------------------------------

META_REGISTER_CLASS(Object, nullptr);
META_REGISTER_CLASS(MetaClass, nullptr);

// ----------------------------------------------------------------------------
// [SECTION] MetaSystem
// ----------------------------------------------------------------------------

META_REGISTER_CLASS(MetaSystem, nullptr);

static MetaSystem *g_metaSystem = nullptr;

void MetaSystem::m_RecursiveInit(
  MetaClass *mc,
  int *globalId,
  int *topoId
) {
  if (mc->m_globalId != -1)
    return;

  if (mc->m_parent)
    MetaSystem::m_RecursiveInit(mc->m_parent(), globalId, topoId);

  skyAssert(*globalId < kMaxClasses);

  mc->m_globalId = (*globalId)++;
  mc->m_baseTopoIdList.clear();

  if (mc->m_parent) {
    MetaClass *superClass = mc->m_parent();
  
    if (superClass->m_topoOrder == -1) {
      int id = *topoId;
      superClass->m_topoOrder = id;
      superClass->m_baseTopoIdList.push_back(id);
      *topoId = id + 1;
    }

    if (superClass != mc) {
      mc->m_baseTopoIdList.insert(
        mc->m_baseTopoIdList.end(),
        superClass->m_baseTopoIdList.begin(),
        superClass->m_baseTopoIdList.end());
    }
  }

  mc->m_metaDataContainer = new MetaDataContainer();
}

void MetaSystem::Initialize() {
  skyAssertMsg(!g_metaSystem, "MetaSystem is a singleton and must be initialized only once.");
  g_metaSystem = this;

  m_data = new MetaSystemDataContainer();

  for (auto it = MetaObject<MetaType>::m_List(); it; it = it->m_prev) {
    char *name = new char[strlen(it->m_name) + 1];
    strcpy(name, it->m_name);

    MetaType *mt = it->Copy();
    mt->m_name = name;
    it->m_self = mt;

    m_data->m_metaTypes[name] = mt;

    if (!it->AsClass())
      continue;

    m_data->m_metaClasses[name] = (MetaClass *)mt;
  }

  int topoOrder = 0
    , globalId = 0;
  
  for (auto &it: m_data->m_metaClasses) {
    m_RecursiveInit(it.second, &globalId, &topoOrder);
  }

  m_metaClassId = MetaClassImpl<MetaSystem>::Must_call_META_REGISTER_CLASS()->m_globalId;
}

// ----------------------------------------------------------------------------
// [SECTION] Functions
// ----------------------------------------------------------------------------

const MetaClass *GetMetaClassById(
  int globalId
) {
  return MetaClassImpl<Object>::Must_call_META_REGISTER_CLASS();
}
