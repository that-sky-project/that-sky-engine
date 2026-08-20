#include "Utils/StlAllocator.hpp"
#include "Utils/Assert.hpp"
#include "Base/Meta.hpp"

// ----------------------------------------------------------------------------
// [SECTION] MetaMemberVariable
// ----------------------------------------------------------------------------

// Place the complete definition in .cpp because we need a complete MetaClass
// interface.
size_t MetaMemberVariable::Count(
  void *pObj
) const {
  // NOTE: The logic here looks strange, but this is the libBootloader.so
  // and Sky.exe disassembly shows. The commented is my implementation, for
  // a better compatibility.
  void *p = nullptr;
  if (m_countAddress) {
    LPCMetaClass mc = GetClass()->AsClass();
    p = mc->ResolveMember(pObj, mc, m_countAddress);
  }
  return (size_t)GetCountType()->ToNumber(p);
  /*
  if (!m_countAddress)
    return 0;
  LPCMetaClass mc = GetClass()->AsClass();
  void *p = mc->ResolveMember(pObj, mc, m_countAddress);
  return (size_t)GetCountType()->ToNumber(p);
  */
}

// ----------------------------------------------------------------------------
// [SECTION] MetaType
// ----------------------------------------------------------------------------

class MetaTypeBool: public MetaType {
public:
  MetaTypeBool(cstring name): MetaType(name) { }

  virtual size_t SizeOfType() const override                { return sizeof(bool); }
  virtual size_t AlignOfType() const override               { return alignof(bool); }

  virtual void *CreateByType() const override               { return new bool; }
  virtual void  DeleteByType(void *p) const override        { delete (bool *)p; }

  virtual void *ConstructByType(void *p) const override     { return new (p) bool; }
  virtual void  DestructByType(void *) const override       { }

  virtual void DynamicCast(
    void *targetObject,
    void *sourceObject,
    LPCMetaType sourceType
  ) const override {
    double num = sourceType->ToNumber(sourceObject);
    *(bool *)targetObject = !!num;
  }

  virtual bool IsNumber() const override                    { return false; }
  virtual bool IsString() const override                    { return false; }

  virtual lua_Number  ToNumber(void *object) const override { return *(bool *)object ? 1.0 : 0.0; }
  virtual cstring     ToString(void *object) const override { return *(bool *)object ? "true" : "false"; }

  virtual LPCMetaClass AsClass() const override             { return nullptr; }

  virtual void WriteType(lua_State *L, void *object) const override { lua_pushboolean(L, *(bool *)object); }
  virtual void ReadType(lua_State *L, int index, void *object) const override { *(bool *)object = !!lua_toboolean(L, index); }

  virtual LPMetaType Copy() const override                  { return new MetaTypeBool{*this}; }
};

// Define ExtractCString helper functions.

// For const char *
template<>
cstring MetaTypeString<cstring>::ExtractCString(
  const cstring *ptr
) {
  return *ptr;
}

// For std::string.
template<>
cstring MetaTypeString<TgcString>::ExtractCString(
  const TgcString *ptr
) {
  return ptr->c_str();
}

static MetaTypeBool g_metaType_bool{"bool"};
META_REGISTER_TYPE(bool)

META_REGISTER_TYPE_NUMBER(uint8_t)
META_REGISTER_TYPE_NUMBER(int8_t)
META_REGISTER_TYPE_NUMBER(uint16_t)
META_REGISTER_TYPE_NUMBER(int16_t)
META_REGISTER_TYPE_NUMBER(uint32_t)
META_REGISTER_TYPE_NUMBER(int32_t)
META_REGISTER_TYPE_NUMBER(uint64_t)
META_REGISTER_TYPE_NUMBER(int64_t)
META_REGISTER_TYPE_NUMBER(float)
META_REGISTER_TYPE_NUMBER(double)

META_REGISTER_TYPE_STRING(cstring)
META_REGISTER_TYPE_STRING(TgcString)

// ----------------------------------------------------------------------------
// [SECTION] MetaClassVoid
// ----------------------------------------------------------------------------

// A MetaClass representing "void" types.
class MetaClassVoid: public MetaClass {
public:
  MetaClassVoid(cstring name): MetaClass(name) { }

  virtual size_t  SizeOfType() const override               { return 0; }
  virtual size_t  AlignOfType() const override              { return 0; }

  virtual void *  CreateByType() const override             { return nullptr; }
  virtual void    DeleteByType(void *p) const override      { }

  virtual void *  ConstructByType(void *p) const override   { return p; }
  virtual void    DestructByType(void *p) const override    { }

  virtual bool    IsNumber() const override                 { return false; }
  virtual bool    IsString() const override                 { return false; }

  virtual LPMetaType Copy() const override                  { return new MetaClassVoid{*this}; }

  virtual bool    IsAbstract() const override               { return true; }
  virtual bool    IsPolymorphic() const override            { return false; }

  virtual size_t  SizeOfObject() const override             { return 0; }
  virtual size_t  AlignOfObject() const override            { return 0; }

  virtual void *  NewObject() const override                { return nullptr; }
  virtual void    DeleteObject(void *) const override       { }

  virtual void *  ConstructObject(void *) const override    { return nullptr; }
  virtual void    DestructObject(void *) const override     { }

  virtual void *  Upcast(void *const &) const override      { return nullptr; }
  virtual void *  Downcast(Object *const &) const override  { return nullptr; }

  virtual void *ResolveMember(
    void *,
    LPCMetaClass,
    MetaMemberVariable::PMember
  ) const override {
    return nullptr;
  }
};

static MetaClassVoid g_metaType_void{"void"};

LPCMetaType GetMetaType() {
  return (LPCMetaType)g_metaType_void.GetActive();
}

LPCMetaClass GetMetaClass() {
  return GetMetaType()->AsClass();
}

// ----------------------------------------------------------------------------
// [SECTION] MetaClass
// ----------------------------------------------------------------------------

void MetaClass::DynamicCast(
  void *targetObject,
  void *sourceObject,
  LPCMetaType sourceType
) const {
  pointer ppObject = (pointer)sourceObject
    , ppResult = (pointer)targetObject;
  LPCMetaClass pSrcClass;

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
  LPCMetaClass pObjectClass = GetMetaClassById(pObject->GetMetaClassId());
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

lua_Number MetaClass::ToNumber(void *) const {
  return 0;
}

cstring MetaClass::ToString(void *) const {
  return "";
}

LPCMetaClass MetaClass::AsClass() const {
  return this;
}

void MetaClass::WriteType(
  lua_State *L,
  void *object
) const {
  pointer ppObject = (pointer)object;
  value_type pObject = *ppObject
    , result;

  if (!pObject)
    return lua_pushnil(L);

  Object *pBase = (Object *)Upcast(pObject);
  if (pBase)
    result = GetMetaClassById(pBase->GetMetaClassId())->Downcast(pBase);
  else
    result = pObject;

  *(pointer)lua_newuserdata(L, SizeOfType()) = result;
  lua_getglobal(L, m_name);
  lua_setmetatable(L, -2);
}

void MetaClass::ReadType(
  lua_State *L,
  int index,
  void *object
) const {
  pointer ppObject = (pointer)object;
  cstring err = nullptr;
  LPCMetaClass metaClass;
  bool isDerived = false;
  char buffer[1088];

  if (!lua_type(L, index)) {
    // LUA_TNIL, directly return nullptr.
    *ppObject = nullptr;
    return;
  }

  pointer pObject = (pointer)lua_touserdata(L, index);
  if (!pObject || !lua_getmetatable(L, index)) {
    // Not a userdata (objects created by MetaSystem) or no metatable.
    err = lua_typename(L, lua_type(L, index));
    goto Err;
  }
  lua_getfield(L, -1, "__metaclass");
  metaClass = (LPCMetaClass)lua_touserdata(L, -1);
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

    for (const auto &it: m_baseTopoIdList) {
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
      cstring msg = "!!!NULL!!!";
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
  ((LPMetaClass)target)->m_parent = m_parent;
}

// ----------------------------------------------------------------------------
// [SECTION] Object
// ----------------------------------------------------------------------------

META_REGISTER_CLASS(Object, nullptr)
META_REGISTER_CLASS(MetaClass, nullptr)

// ----------------------------------------------------------------------------
// [SECTION] MetaSystem
// ----------------------------------------------------------------------------

META_REGISTER_CLASS(MetaSystemExample, nullptr)

static MetaSystemExample *g_metaSystem = nullptr;

static LPCMetaClass GetMetaClassById_default(
  const void *,
  int globalId
) {
  return g_metaSystem->m_classes[globalId];
}

static LPCMetaClass GetMetaClassByName_default(
  const void *,
  cstring name,
  bool constString
) {
  if (constString) { }

  auto &classes = g_metaSystem->m_data->m_metaClasses;
  auto it = classes.find(name);
  if (it == classes.end())
    return nullptr;

  return it->second;
}

void MetaSystemExample::m_RecursiveInit(
  LPMetaClass mc,
  int *globalId,
  int *topoId
) {
  if (mc->m_globalId != -1)
    return;

  if (mc->m_parent)
    m_RecursiveInit(mc->m_parent(), globalId, topoId);

  SkyAssert(*globalId < kMaxClasses);

  mc->m_globalId = (*globalId)++;
  mc->m_baseTopoIdList.clear();

  if (mc->m_parent) {
    LPMetaClass superClass = mc->m_parent();
  
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

void MetaSystemExample::Initialize() {
  SkyAssertMsg(!g_metaSystem, "MetaSystem is a singleton and must be initialized only once.");
  g_metaSystem = this;
  SetMetaSystem(this, GetMetaClassById_default, GetMetaClassByName_default);

  m_data = new MetaSystemDataContainer();

  // Copy metatypes.
  for (auto it = MetaObject<MetaType>::m_List(); it; it = it->GetPrev()) {
    char *name = new char[strlen(it->GetName()) + 1];
    strcpy(name, it->GetName());

    LPMetaType mt = it->Copy();
    mt->SetName(name);
    mt->SetActive(mt);
    it->SetActive(mt);

    m_data->m_metaTypes[name] = mt;

    if (!it->AsClass())
      continue;

    m_data->m_metaClasses[name] = (LPMetaClass )mt;
  }

  // Store metaclasses.
  for (int i = 0; i < kMaxClasses; i++) {
    m_classes[i] = GetMetaType()->AsClass();
  }

  int topoOrder = 0
    , globalId = 0;
  
  // Initialize metaclasses.
  for (auto &it: m_data->m_metaClasses) {
    m_RecursiveInit(it.second, &globalId, &topoOrder);
    m_classes[it.second->m_globalId] = it.second;
  }

  // Initialize metamemberfunctons.
  for (auto it = MetaObject<MetaMemberFunction>::m_List(); it; it = it->GetPrev()) {
    char *name = new char[strlen(it->GetName()) + 1];
    strcpy(name, it->GetName());

    MetaMemberFunction *mf = new MetaMemberFunction(*it);
    mf->SetName(name);
    mf->Initialize();
    mf->GetClass()->m_metaDataContainer->m_functions[name] = mf;
  }

  m_metaClassId = MetaClassImpl<MetaSystemExample>::Must_call_META_REGISTER_CLASS()->m_globalId;
}

// ----------------------------------------------------------------------------
// [SECTION] Functions
// ----------------------------------------------------------------------------

static const void *g_metaSystem_userData = nullptr;
static PFN_UserGetMetaClassById g_metaSystem_idGetter = nullptr;
static PFN_UserGetMetaClassByName g_metaSystem_nameGetter = nullptr;

void SetMetaSystem(
  const void *userdata,
  PFN_UserGetMetaClassById idGetter,
  PFN_UserGetMetaClassByName nameGetter
) {
  g_metaSystem_userData = userdata;
  g_metaSystem_idGetter = idGetter;
  g_metaSystem_nameGetter = nameGetter;
}

void GetMetaSystem(
  const void **pUserdata,
  PFN_UserGetMetaClassById *pIdGetter,
  PFN_UserGetMetaClassByName *pNameGetter
) {
  if (pUserdata) *pUserdata = g_metaSystem_userData;
  if (pIdGetter) *pIdGetter = g_metaSystem_idGetter;
  if (pNameGetter) *pNameGetter = g_metaSystem_nameGetter;
}

LPCMetaClass GetMetaClassById(
  int globalId
) {
  if (g_metaSystem_idGetter)
    return g_metaSystem_idGetter(g_metaSystem_userData, globalId);
  return nullptr;
}

LPCMetaClass GetMetaClassByName(
  cstring name,
  bool constString
) {
  if (g_metaSystem_nameGetter)
    return g_metaSystem_nameGetter(g_metaSystem_userData, name, constString);
  return nullptr;
}
