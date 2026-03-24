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

class MetaTypeBool: public MetaType {
public:
  MetaTypeBool(const char *name): MetaType(name) { }

  virtual size_t SizeOfType() const override {
    return sizeof(bool);
  }

  virtual size_t AlignOfType() const override {
    return alignof(bool);
  }

  virtual void *CreateByType() const override {
    return new bool;
  }

  virtual void DeleteByType(void *p) const override {
    delete (bool *)p;
  }

  virtual void *ConstructByType(void *p) const override {
    return new (p) bool;
  }

  virtual void DestructByType(void *) const override { }

  virtual void DynamicCast(
    void *targetObject,
    void *sourceObject,
    const MetaType *sourceType
  ) const override {
    double num = sourceType->ToNumber(sourceObject);
    *(bool *)targetObject = !!num;
  }

  virtual bool IsNumber() const override {
    return false;
  }

  virtual bool IsString() const override {
    return false;
  }

  virtual lua_Number ToNumber(void *object) const override {
    return *(bool *)object ? 1.0 : 0.0;
  }

  virtual const char *ToString(void *object) const override {
    return *(bool *)object ? "true" : "false";
  }

  virtual const MetaClass *AsClass() const override {
    return nullptr;
  }

  virtual void WriteType(
    lua_State *L,
    void *object
  ) const override {
    lua_pushboolean(L, *(bool *)object);
  }

  virtual void ReadType(
    lua_State *L,
    int index,
    void *object
  ) const override {
    *(bool *)object = !!lua_toboolean(L, index);
  }

  virtual MetaType *Copy() const override {
    return new MetaTypeBool{*this};
  }
};

template<typename T>
class MetaTypeNumber: public MetaType {
public:
  MetaTypeNumber(const char *name): MetaType(name) { }

  virtual size_t SizeOfType() const override {
    return sizeof(T);
  }

  virtual size_t AlignOfType() const override {
    return alignof(T);
  }

  virtual void *CreateByType() const override {
    return new T;
  }

  virtual void DeleteByType(void *p) const override {
    delete (T *)p;
  }

  // Construct type T in-place. In most cases this function does nothing.
  virtual void *ConstructByType(void *p) const override {
    return new (p) T;
  }

  // Call the destructor function of type T.
  virtual void DestructByType(void *p) const override {
    ((T *)p)->~T();
  }

  // Perform number type dynamic cast. Convert to lua_Number (double), then
  // convert to type T.
  virtual void DynamicCast(
    void *targetObject,
    void *sourceObject,
    const MetaType *sourceType
  ) const override {
    *(T *)targetObject = (T)sourceType->ToNumber(sourceObject);
  }

  virtual bool IsNumber() const override {
    return true;
  }

  virtual bool IsString() const override {
    return false;
  }

  // Convert from type T to lua_Number.
  virtual lua_Number ToNumber(
    void *object
  ) const override {
    return (lua_Number)*(T *)object;
  }

  // Convert from type T to string.
  virtual const char *ToString(
    void *object
  ) const override {
    static char buf[80];
    snprintf(buf, 65, "%g", ToNumber(object));
    return buf;
  }

  virtual const MetaClass *AsClass() const override {
    return nullptr;
  }

  virtual void WriteType(
    lua_State *L,
    void *object
  ) const override {
    lua_pushnumber(L, (lua_Number)*(T *)object);
  }

  virtual void ReadType(
    lua_State *L,
    int index,
    void *object
  ) const override {
    *(T *)object = luaL_checknumber(L, index);
  }

  virtual MetaType *Copy() const override {
    return new MetaTypeNumber<T>{*this};
  }
};

template<typename T>
class MetaTypeString: public MetaType {
public:
  MetaTypeString(const char *name): MetaType(name) { }

  virtual size_t SizeOfType() const override {
    return sizeof(T);
  }

  virtual size_t AlignOfType() const override {
    return alignof(T);
  }

  virtual void *CreateByType() const override {
    return new T;
  }

  virtual void DeleteByType(void *p) const override {
    delete (T *)p;
  }

  virtual void *ConstructByType(void *p) const override {
    return new (p) T;
  }

  virtual void DestructByType(void *p) const override {
    ((T *)p)->~T();
  }

  virtual void DynamicCast(
    void *targetObject,
    void *sourceObject,
    const MetaType *sourceType
  ) const override {
    const char *s = sourceType->ToString(sourceObject);
    *(T *)targetObject = s;
  }

  virtual bool IsNumber() const override {
    return false;
  }

  virtual bool IsString() const override {
    return true;
  }

  virtual lua_Number ToNumber(
    void *object
  ) const override {
    return atof(MetaType::ExtractCString((T *)object));
  }

  virtual const char *ToString(
    void *object
  ) const override {
    return MetaType::ExtractCString((T *)object);
  }

  virtual const MetaClass *AsClass() const override {
    return nullptr;
  }

  virtual void WriteType(
    lua_State *L,
    void *object
  ) const override {
    lua_pushstring(L, MetaType::ExtractCString((T *)object));
  }

  virtual void ReadType(
    lua_State *L,
    int index,
    void *object
  ) const override {
    if (!lua_isstring(L, index))
      return;
    *(T *)object = luaL_checklstring(L, index, nullptr);
  }

  virtual MetaType *Copy() const override {
    return new MetaTypeString<T>{
      static_cast<const MetaTypeString<T> &>(*this)};
  }
};

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

// ----------------------------------------------------------------------------
// [SECTION] MetaClassVoid
// ----------------------------------------------------------------------------

// A MetaClass representing "void" types.
class MetaClassVoid: public MetaClass {
public:
  MetaClassVoid(const char *name): MetaClass(name) { }

  virtual size_t SizeOfType() const override { return 0; }

  virtual size_t AlignOfType() const override { return 0; }

  virtual void *CreateByType() const override { return nullptr; }

  virtual void DeleteByType(void *p) const override { }

  virtual void *ConstructByType(void *p) const override { return p; }

  virtual void DestructByType(void *p) const override { }

  virtual bool IsNumber() const override { return false; }

  virtual bool IsString() const override { return false; }

  virtual MetaType *Copy() const override {
    return new MetaClassVoid{*this};
  }

  virtual bool IsAbstract() const override { return true; }

  virtual bool IsPolymorphic() const override { return false; }

  virtual size_t SizeOfObject() const override { return 0; }

  virtual size_t AlignOfObject() const override { return 0; }

  virtual void *NewObject() const override { return nullptr; }

  virtual void DeleteObject(void *) const override { }

  virtual void *ConstructObject(void *) const override { return nullptr; }

  virtual void DestructObject(void *) const override { }

  virtual void *Upcast(void *const &) const override { return nullptr; }

  virtual void *Downcast(Object *const &) const override { return nullptr; }
};

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
  const MetaType *sourceType
) const {
  Payload *ppObject = (Payload *)sourceObject
    , *ppResult = (Payload *)targetObject;
  const MetaClass *pSrcClass;

  if (this == sourceType) {
    *ppResult = *ppObject;
    return;
  }
  
  pSrcClass = sourceType->AsClass();
  if (!pSrcClass) {
    // The source object is not a class.
    *ppResult = nullptr;
    return;
  }

  Object *pObject = (Object *)pSrcClass->Upcast(*ppObject);
  if (!pObject) {
    // The source object is void.
    *ppResult = nullptr;
    return;
  }

  // Get the actual type of the source object.
  //
  // This function allows you to cast anything in the same inheritance chain like
  // to their base class, even if there's virtual function added in the derived
  // class. But the function don't changed the metaclass id carried by the original
  // object.
  //
  // So we can, and we need to extract the metaclass id to get the correct address
  // of the object.
  const MetaClass *pObjectClass = GetMetaClassById(pObject->m_metaClassId);
  if (pObjectClass->AsClass() == this) {
    // The actual type of the source object is the current type.
    // Downcast to adjust pointer.
    *ppResult = Downcast(pObject);
    return;
  }

  if (m_topoOrder == -1) {
    // Not a base class of any class.
    *ppResult = nullptr;
    return;
  }

  for (auto topoId: pObjectClass->m_baseTopoIdList) {
    if (m_topoOrder == topoId) {
      // The source object is a subclass of target object.
      *ppResult = Downcast(pObject);
      return;
    }
  }

  *ppResult = nullptr;
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

  *(Payload *)lua_newuserdata(L, SizeOfType()) = result;
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

  DynamicCast(ppObject, pObject, metaClass);
}

void MetaClass::SimpleCopy(
  void *target
) const {
  MetaType::SimpleCopy(target);
  ((MetaClass *)target)->m_parent = m_parent;
}

// ----------------------------------------------------------------------------
// [SECTION] Object
// ----------------------------------------------------------------------------

Object::Object() {
  m_metaClassId = MetaClassImpl<Object>::Must_call_META_REGISTER_CLASS()->m_globalId;
}

META_REGISTER_CLASS(Object, nullptr);
META_REGISTER_CLASS(MetaClass, nullptr);

// ----------------------------------------------------------------------------
// [SECTION] MetaSystem
// ----------------------------------------------------------------------------

META_REGISTER_CLASS(MetaSystem, nullptr);

static MetaSystem *g_metaSystem = nullptr;

MetaSystem::MetaSystem() {
  m_metaClassId = MetaClassImpl<MetaSystem>::Must_call_META_REGISTER_CLASS()->m_globalId;
}

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
    mt->m_self = it->m_self = mt;

    m_data->m_metaTypes[name] = mt;

    if (!it->AsClass())
      continue;

    m_data->m_metaClasses[name] = (MetaClass *)mt;
  }

  for (int i = 0; i < kMaxClasses; i++) {
    m_classes[i] = GetMetaType()->AsClass();
  }

  int topoOrder = 0
    , globalId = 0;
  
  for (auto &it: m_data->m_metaClasses) {
    m_RecursiveInit(it.second, &globalId, &topoOrder);
    m_classes[it.second->m_globalId] = it.second;
  }

  m_metaClassId = MetaClassImpl<MetaSystem>::Must_call_META_REGISTER_CLASS()->m_globalId;
}

// ----------------------------------------------------------------------------
// [SECTION] Functions
// ----------------------------------------------------------------------------

const MetaClass *GetMetaClassById(
  int globalId
) {
  return g_metaSystem->m_classes[globalId];
}

//const MetaClass *

const MetaClass *GetMetaClassByName(
  const char *name,
  bool constString
) {
  if (constString)
    ;

  auto &classes = g_metaSystem->m_data->m_metaClasses;
  auto it = classes.find(name);
  if (it == classes.end())
    return nullptr;

  return it->second;
}
