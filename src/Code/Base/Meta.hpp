#ifndef __META_HPP__
#define __META_HPP__

#include <stdint.h>
#include <type_traits>
#include <vector>
#include <string>
#include <unordered_map>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#include "StlAllocator.hpp"
#include "Types.h"
#include "Assert.hpp"

// ----------------------------------------------------------------------------
// [SECTION] HeaderMisc
// ----------------------------------------------------------------------------

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

// ----------------------------------------------------------------------------
// [SECTION] Macros
// ----------------------------------------------------------------------------

// Declare a type.
#define META_DECLARE_TYPE(T)\
  template<> LPCMetaType GetMetaTypeByType<T>();

// Declare a class.
#define META_DECLARE_CLASS(T) \
  template<> LPMetaClass MetaClassImpl<T>::Must_call_META_REGISTER_CLASS();\
  template<> LPCMetaClass GetMetaClassByType<T *>();

// Register a type.
#define META_REGISTER_TYPE(T)\
  template<>\
  LPCMetaType GetMetaTypeByType<T>() {\
    return g_metaType_##T.GetActive();\
  }

// Register a number type.
#define META_REGISTER_TYPE_NUMBER(T)\
  static MetaTypeNumber<T> g_metaType_##T{#T};\
  META_REGISTER_TYPE(T)

// Register a number type.
#define META_REGISTER_TYPE_STRING(T)\
  static MetaTypeString<T> g_metaType_##T{#T};\
  META_REGISTER_TYPE(T)

// Register a class.
#define META_REGISTER_CLASS(T, ...) \
  static MetaClassImpl<T> g_metaClass_##T{#T, ## __VA_ARGS__};\
  template<> LPMetaClass MetaClassImpl<T>::Must_call_META_REGISTER_CLASS() {\
    return static_cast<MetaClassImpl<T> *>(g_metaClass_##T.m_self);\
  }\
  template<> LPCMetaClass GetMetaClassByType<T *>() {\
    return MetaClassImpl<T>::Must_call_META_REGISTER_CLASS();\
  }

#define MetaClassId(T) MetaClassImpl<T>::Must_call_META_REGISTER_CLASS()->GetId()

// Register a member variable.
#define META_REGISTER_SIMPLE_MEMBER(_Type, _Class, _Name) \
static MetaMemberVariable g_metaMemberVariable_ ## _Class ## _ ## _Name = {\
  #_Name,\
  (PFN_GetClass)MetaClassImpl<_Class>::Must_call_META_REGISTER_CLASS,\
  (int32_t)offsetof(_Class, _Name),\
  (PFN_GetType)GetMetaTypeByType<_Type>\
};

// Register a member array.
#define META_REGISTER_STATIC_MEMBER(_Type, _Class, _Name, _Length, _CountType, _CountName) \
static MetaMemberVariable g_metaMemberVariable_ ## _Class ## _ ## _Name = {\
  #_Name,\
  (PFN_GetClass)MetaClassImpl<_Class>::Must_call_META_REGISTER_CLASS,\
  (int32_t)offsetof(_Class, _Name),\
  (PFN_GetType)GetMetaTypeByType<_Type>,\
  (int32_t)offsetof(_Class, _CountName),\
  (PFN_GetType)GetMetaTypeByType<_CountType>,\
  _Length\
};

#define META_REGISTER_CONSTANT(_Type, _Name, _Value) \
static MetaConstantImpl<_Type> g_metaConstant_ ## _Name = {\
  #_Name,\
  GetMetaTypeByType<_Type>,\
  _Value\
};

// ----------------------------------------------------------------------------
// [SECTION] Declarations
// ----------------------------------------------------------------------------

class MetaFunction;
class MetaVariable;
class MetaMemberFunction;
class MetaMemberVariable;
class MetaType;
class MetaClass;
class MetaSystem;
class Object;

struct MetaStrHash {
  std::size_t operator()(
    const char* s
  ) const {
    return std::hash<std::string>{}(s);
  }
};

struct MetaStrEq {
  bool operator()(
    const char* a,
    const char* b
  ) const {
    return strcmp(a, b) == 0;
  }
};

template<typename Tv>
using MetaStrHashMap = std::unordered_map<cstring , Tv, MetaStrHash, MetaStrEq>;

using LPMetaType = MetaType *;
using LPMetaClass = MetaClass *;
using LPCMetaType = const MetaType *;
using LPCMetaClass = const MetaClass *;

using PFN_RegisterType = LPMetaType (*)();
using PFN_RegisterClass = LPMetaClass (*)();
using PFN_GetType = LPCMetaType (*)();
using PFN_GetClass = LPCMetaClass (*)();

// ----------------------------------------------------------------------------
// [SECTION] MetaObject
// ----------------------------------------------------------------------------

// Represents an object.
template<typename T>
class MetaObject {
public:
  static inline T *&m_List() {
    static T *p = nullptr;
    return p;
  }

  MetaObject(
    cstring name
  )
    : m_name(name)
  { }

  MetaObject(
    const MetaObject &src
  )
    : m_name(src.m_name)
    , m_fields(src.m_fields)
    , m_prev(src.m_prev)
  { }

  inline cstring &GetName() { return m_name; }
  inline const cstring &GetName() const { return m_name; }
  inline void *GetFields() const { return m_fields; }
  inline T *GetPrev() const { return m_prev; }

protected:
  // Name of the object.
  cstring m_name = nullptr;
  // External descriptors.
  void *m_fields = nullptr;
  // Previous object, build a chain list for initialization.
  T *m_prev = nullptr;
};

// ----------------------------------------------------------------------------
// [SECTION] MetaFunction
// ----------------------------------------------------------------------------

class MetaFunction: public MetaObject<MetaFunction> {

};

// ----------------------------------------------------------------------------
// [SECTION] MetaVariable
// ----------------------------------------------------------------------------

class MetaVariable: public MetaObject<MetaVariable> {

};

// ----------------------------------------------------------------------------
// [SECTION] MetaConstant
// ----------------------------------------------------------------------------

class MetaConstant: public MetaObject<MetaConstant> {
public:
  MetaConstant(
    cstring name,
    PFN_RegisterType type
  )
    : MetaObject<MetaConstant>(name)
    , m_type(type)
  {
    m_prev = MetaObject<MetaConstant>::m_List(); 
    MetaObject<MetaConstant>::m_List() = this; 
  }

  inline LPCMetaType GetType() {
    return m_type();
  }

protected:
  void *unk_1;
  void *m_valuePtr;
  PFN_RegisterType m_type;
};

template<typename T>
class MetaConstantImpl: public MetaConstant {
public:
  MetaConstantImpl(
    cstring name,
    PFN_RegisterType type,
    T value
  )
    : MetaConstant(name, type)
  {
    m_valuePtr = new T;
    *m_valuePtr = value;
  }

  inline T GetValue() {
    return *m_valuePtr;
  }
};

// ----------------------------------------------------------------------------
// [SECTION] MetaMemberFunction
// ----------------------------------------------------------------------------

// Represents a member function.
class MetaMemberFunction: public MetaObject<MetaMemberFunction> {
public:
  void *unk_1;
  void *signature[3];
  void *function;
  void *unk_2;
  int unk_3;
  void (*applyWrapper)();
  void (*initSignature)(void *);
  LPCMetaClass (*getType)();
};

// ----------------------------------------------------------------------------
// [SECTION] MetaMemberVariable
// ----------------------------------------------------------------------------

// Represents a member variable.
class MetaMemberVariable: public MetaObject<MetaMemberVariable> {
public:
  struct Context {
    Context(
      int32_t offset,
      int32_t vbtableOffset,
      int32_t vbtableSlot
    )
      : offset(offset)
      , vbtableOffset(vbtableOffset)
      , vbtableSlot(vbtableSlot)
    { }

    // Offset of a member variable within the object (either the passed object
    // itself or a base class subobject).
    int32_t offset = 0;
    // Offset of the vbtable within the object itself, always be 0 or 8.
    int32_t vbtableOffset = 0;
    // The vbtable slot index corresponding to the base class subobject that
    // contains the target member variable.
    int32_t vbtableSlot = 0;
  };

  // Register a static array member variable.
  MetaMemberVariable(
    cstring name,
    PFN_GetClass clazz,
    int32_t offset,
    PFN_GetType type,
    int32_t countOffset,
    PFN_GetType countType,
    uint64_t maxSize
  )
    : MetaObject<MetaMemberVariable>(name)
    , m_address(offset)
    , m_type(type)
    , m_class(clazz)
    , m_countAddress(countOffset)
    , m_countType(countType)
    , m_staticArraySize(maxSize)
  {
    m_prev = MetaObject<MetaMemberVariable>::m_List();
    MetaObject<MetaMemberVariable>::m_List() = this;
  }

  // Register a dynamic array member variable.
  MetaMemberVariable(
    cstring name,
    PFN_GetClass clazz,
    int32_t offset,
    PFN_GetType type,
    int32_t countOffset,
    PFN_GetType countType
  )
    : MetaMemberVariable(name, clazz, offset, type, countOffset, countType, 0)
  { }

  // Register a simple member variable.
  MetaMemberVariable(
    cstring name,
    PFN_GetClass clazz,
    int32_t offset,
    PFN_GetType type
  )
    : MetaMemberVariable(name, clazz, offset, type, 0, nullptr, 0)
  { }

  inline Context GetContext() {
    return { m_address, m_vbAddress, m_vbSlot };
  }

  inline Context GetCountContext() {
    return { m_countAddress, m_countVbAddress, m_countVbSlot };
  }

  inline LPCMetaType GetType() { return m_type(); }
  inline LPCMetaType GetCountType() { return m_countType(); }
  inline LPCMetaClass GetClass() { return m_class(); }
  inline uint64_t GetStaticSize() { return m_staticArraySize; }
  inline bool HasCount() { return m_countAddress || m_countVbAddress != -1; }
  inline bool IsDynamic() { return HasCount() && !m_staticArraySize; }

protected:
  uint64_t unk_1 = 0;
  // Offset of the member variable in the object.
  int32_t m_address = 0;
  int32_t m_vbAddress = 0;
  int32_t m_vbSlot = 0;
  // Get the type of this member variable.
  PFN_GetType m_type = nullptr;
  // Get the class of this member variable belongs to.
  PFN_GetClass m_class = nullptr;
  // Descriptor of array length.
  int32_t m_countAddress = 0;
  int32_t m_countVbAddress = 0;
  int32_t m_countVbSlot = -1;
  // Type of the array length.
  PFN_GetType m_countType = nullptr;
  // Max length of the array.
  uint64_t m_staticArraySize = 0;
};

// ----------------------------------------------------------------------------
// [SECTION] MetaType
// ----------------------------------------------------------------------------

// Repersents a type.
class MetaType: public MetaObject<MetaType> {
protected:
  using NoChainList_t = void *;

public:
  static constexpr NoChainList_t noChainList = nullptr;

  MetaType(
    cstring name
  )
    : MetaObject<MetaType>(name)
  {
    m_prev = MetaObject<MetaType>::m_List();
    MetaObject<MetaType>::m_List() = this;
  }

  MetaType(
    cstring name,
    const NoChainList_t &
  )
    : MetaObject<MetaType>(name)
  { }

  MetaType(
    const MetaType &src
  )
    : MetaObject<MetaType>(src)
    , unk_1(src.unk_1)
    , m_self(src.m_self)
  { }

  virtual ~MetaType() = default;

  // Get the size of the type.
  virtual size_t SizeOfType() const = 0;

  // Get the alignment of the type.
  virtual size_t AlignOfType() const = 0;

  // Create an object of the type.
  virtual void *CreateByType() const = 0;

  // Delete an created object of the type.
  virtual void DeleteByType(void *p) const = 0;

  // Calls the constructor of the type. Only valid when the type is std::string.
  virtual void *ConstructByType(void *p) const = 0;

  // Calls the destructor of the type. Only valid when the type is std::string.
  virtual void DestructByType(void *p) const = 0;

  // Dynamic cast an object "sourceObject" of type specified by "sourceType" to
  // an object "targetObject" of type specified by this.
  virtual void DynamicCast(
    void *targetObject,
    void *sourceObject,
    LPCMetaType sourceType
  ) const = 0;

  // Return whether the type is a number type.
  virtual bool IsNumber() const = 0;

  // Return whether the type is a string type.
  virtual bool IsString() const = 0;

  // Convert the type represented by the metaclass to a lua number (double).
  virtual lua_Number ToNumber(
    void *object
  ) const = 0;

  // Convert the type represented by the metaclass to a string.
  // NOTE: This function is considered as single-threaded.
  virtual cstring ToString(
    void *object
  ) const = 0;

  // Return "this" when the type is not a primitive type.
  virtual LPCMetaClass AsClass() const = 0;

  // Write the type represented by the metaclass to lua_State.
  virtual void WriteType(
    lua_State *L,
    void *object
  ) const = 0;

  // Read the type represented by the metaclass from lua_State.
  virtual void ReadType(
    lua_State *L,
    int index,
    void *object
  ) const = 0;

  // Copy the MetaType object to a new MetaType object.
  virtual LPMetaType Copy() const = 0;

  virtual void SimpleCopy(
    void *target
  ) const {
    memcpy(target, this, sizeof(MetaType));
  }

  MetaType &operator=(const MetaType &) = default;

  inline LPMetaType &GetActive() { return m_self; }
  inline LPMetaType const &GetActive() const  { return m_self; }

protected:
  // Unknown member, maybe for padding.
  void *unk_1 = nullptr;
  // Point to currently activated copy of the type/class.
  LPMetaType m_self = this;
};

template<typename T>
class MetaTypeNumber: public MetaType {
public:
  using value_type      = T;
  using pointer         = value_type *;
  using const_pointer   = const value_type *;
  using reference       = value_type &;
  using const_reference = const value_type &;

  MetaTypeNumber(cstring name): MetaType(name) { }

  virtual size_t SizeOfType() const override {
    return sizeof(value_type);
  }

  virtual size_t AlignOfType() const override {
    return alignof(value_type);
  }

  virtual void *CreateByType() const override {
    return new value_type;
  }

  virtual void DeleteByType(void *p) const override {
    delete (pointer)p;
  }

  // Construct type T in-place. In most cases this function does nothing.
  virtual void *ConstructByType(void *p) const override {
    return new (p) value_type;
  }

  // Call the destructor function of type T.
  virtual void DestructByType(void *p) const override {
    ((value_type *)p)->~value_type();
  }

  // Perform number type dynamic cast. Convert to lua_Number (double), then
  // convert to type T.
  virtual void DynamicCast(
    void *targetObject,
    void *sourceObject,
    LPCMetaType sourceType
  ) const override {
    *(pointer)targetObject = (value_type)sourceType->ToNumber(sourceObject);
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
    return (lua_Number)*(pointer)object;
  }

  // Convert from type T to string.
  virtual cstring ToString(
    void *object
  ) const override {
    static char buf[80];
    snprintf(buf, 65, "%g", ToNumber(object));
    return buf;
  }

  virtual LPCMetaClass AsClass() const override {
    return nullptr;
  }

  virtual void WriteType(
    lua_State *L,
    void *object
  ) const override {
    lua_pushnumber(L, (lua_Number)*(pointer)object);
  }

  virtual void ReadType(
    lua_State *L,
    int index,
    void *object
  ) const override {
    *(pointer)object = (value_type)luaL_checknumber(L, index);
  }

  virtual LPMetaType Copy() const override {
    return new MetaTypeNumber<value_type>{*this};
  }
};

// Represents a string.
template<typename T>
class MetaTypeString: public MetaType {
public:
  using value_type      = T;
  using pointer         = value_type *;
  using const_pointer   = const value_type *;
  using reference       = value_type &;
  using const_reference = const value_type &;

  // Helper function for extracting C strings.
  static cstring ExtractCString(const_pointer s);

  MetaTypeString(cstring name): MetaType(name) { }

  virtual size_t SizeOfType() const override {
    return sizeof(value_type);
  }

  virtual size_t AlignOfType() const override {
    return alignof(value_type);
  }

  virtual void *CreateByType() const override {
    return new value_type;
  }

  virtual void DeleteByType(void *p) const override {
    delete (pointer)p;
  }

  virtual void *ConstructByType(void *p) const override {
    return new (p) value_type;
  }

  virtual void DestructByType(void *p) const override {
    ((value_type *)p)->~value_type();
  }

  virtual void DynamicCast(
    void *targetObject,
    void *sourceObject,
    LPCMetaType sourceType
  ) const override {
    cstring s = sourceType->ToString(sourceObject);
    *(pointer)targetObject = s;
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
    return atof(ExtractCString((pointer)object));
  }

  virtual cstring ToString(
    void *object
  ) const override {
    return ExtractCString((pointer)object);
  }

  virtual LPCMetaClass AsClass() const override {
    return nullptr;
  }

  virtual void WriteType(
    lua_State *L,
    void *object
  ) const override {
    lua_pushstring(L, MetaTypeString::ExtractCString((pointer)object));
  }

  virtual void ReadType(
    lua_State *L,
    int index,
    void *object
  ) const override {
    if (!lua_isstring(L, index))
      return;
    *(pointer)object = luaL_checklstring(L, index, nullptr);
  }

  virtual LPMetaType Copy() const override {
    return new MetaTypeString<T>{
      static_cast<const MetaTypeString<T> &>(*this)
    };
  }
};

// Must define your own ExtractCString when registering a new string type.
template<>
cstring MetaTypeString<cstring>::ExtractCString(
  const cstring *ptr);

template<>
cstring MetaTypeString<TgcString>::ExtractCString(
  const TgcString *ptr);

// ----------------------------------------------------------------------------
// [SECTION] MetaClass
// ----------------------------------------------------------------------------

struct MetaDataContainer {
  // Member variables of the object.
  MetaStrHashMap<MetaMemberVariable *> m_variables = {};
  // Member functions of the object.
  MetaStrHashMap<MetaMemberFunction *> m_functions = {};
  std::unordered_map<cstring , void *> unk_3 = {};
  std::unordered_map<cstring , void *> unk_4 = {};
  std::unordered_map<cstring , void *> unk_5 = {};
};

// MetaClass object implementation.
// NOTE: We can consider MetaClass as MetaTypePointer. All operations of
// MetaClass is performed on the pointer to the objects.
class MetaClass: public MetaType {
public:
  using object_type     = void;
  using value_type      = object_type *;
  using pointer         = value_type *;
  using const_pointer   = const value_type *;
  using reference       = value_type &;
  using const_reference = const value_type &;

  // The original code of TGC as below.
  static constexpr int kMaxClasses = 0xA00;

  MetaClass(
    cstring name,
    PFN_RegisterClass parent = nullptr
  )
    : MetaType(name)
    , m_parent(parent)
  { }

  MetaClass(
    cstring name,
    const NoChainList_t &,
    PFN_RegisterClass parent = nullptr
  )
    : MetaType(name, MetaType::noChainList)
    , m_parent(parent)
  { }

  virtual ~MetaClass() = default;

  virtual void DynamicCast(
    void *targetObject,
    void *sourceObject,
    LPCMetaType sourceType
  ) const override;

  virtual lua_Number ToNumber(void *) const override;
  virtual cstring ToString(void *) const override;

  virtual LPCMetaClass AsClass() const override;

  // Write an object to lua stack.
  virtual void WriteType(
    lua_State *L,
    void *object
  ) const override;

  // Read an object from lua stack.
  virtual void ReadType(
    lua_State *L,
    int index,
    void *object
  ) const override;

  // Copy the MetaType sub object to "target".
  virtual void SimpleCopy(
    void *target
  ) const override;

  // Get whether the object is abstract.
  virtual bool IsAbstract() const = 0;

  // Get whether the object is polymorphic.
  virtual bool IsPolymorphic() const = 0;

  // Get the size of the object represented by the type.
  virtual size_t SizeOfObject() const = 0;

  // Get the alignment of the object represented by the type.
  virtual size_t AlignOfObject() const = 0;

  // Create a new object.
  virtual void *NewObject() const = 0;

  // Delete an object which created by NewObject().
  virtual void DeleteObject(void *object) const = 0;

  // Construct a new object at "object".
  virtual void *ConstructObject(void *object) const = 0;

  // Destruct an object which constructed by ConstructObject().
  virtual void DestructObject(void *object) const = 0;

  // Get the pointer of Object sub object from the sub class, i.e. static_cast
  // from `T *` to `Object *`.
  virtual void *Upcast(void *const &object) const = 0;

  // Get the pointer to a subclass from an Object sub object, i.e. static_cast
  // from `Object *` to `T *`.
  virtual void *Downcast(Object *const &object) const = 0;

  // Resolve member variable base address.
  virtual void *ResolveMember(
    void *ppObject,
    LPCMetaClass pClass,
    const MetaMemberVariable::Context *context
  ) const = 0;

  inline LPMetaClass GetParent() const { if (!m_parent) return nullptr; return m_parent(); }
  inline int32_t GetId() const { return m_globalId; }
  inline int32_t GetTopoId() const { return m_topoOrder; }
  inline const MetaDataContainer *GetData() const { return m_metaDataContainer; }

  // Call the function to get the parent class.
  PFN_RegisterClass m_parent = nullptr;
  // Global id of the metaclass.
  int32_t m_globalId = -1;
  // Topology id of the metaclass.
  int32_t m_topoOrder = -1;
  // Topology id list of parent classes in the inheritance chain.
  std::vector<int32_t> m_baseTopoIdList = {};
  // Point to external data container.
  MetaDataContainer *m_metaDataContainer = nullptr;
  // 
  void *m_vtableCache;
};

// A MetaClass representing "Object" types.
template<typename T>
class MetaClassImpl: public MetaClass {
public:
  using object_type     = T;
  using value_type      = T *;
  using pointer         = value_type *;
  using const_pointer   = const value_type *;
  using reference       = value_type &;
  using const_reference = const value_type &;

  static LPMetaClass Must_call_META_REGISTER_CLASS() {
    return nullptr;
  }

  MetaClassImpl(
    cstring name,
    PFN_RegisterClass parent = nullptr
  )
    : MetaClass(name, parent)
  { }

  virtual size_t SizeOfType() const override {
    return sizeof(value_type);
  }

  virtual size_t AlignOfType() const override {
    return alignof(value_type);
  }

  virtual void *CreateByType() const override {
    return new value_type{nullptr};
  }

  virtual void DeleteByType(void *p) const override {
    delete (pointer)p;
  }

  // Construct a nullptr on p, i.e. p = Payload * = void **.
  virtual void *ConstructByType(void *p) const override {
    return new (p) value_type{nullptr};
  }

  virtual void DestructByType(void *p) const override { }

  virtual bool IsNumber() const override { return false; }

  virtual bool IsString() const override { return false; }

  virtual LPMetaType Copy() const override {
    return new MetaClassImpl<object_type>{*this};
  }

  virtual bool IsAbstract() const override {
    return std::is_abstract_v<object_type>;
  }

  virtual bool IsPolymorphic() const override {
    return std::is_polymorphic_v<object_type>;
  }

  virtual size_t SizeOfObject() const override {
    return sizeof(object_type);
  }

  virtual size_t AlignOfObject() const override {
    return alignof(object_type);
  }

  virtual void *NewObject() const override {
    if constexpr (!std::is_abstract_v<object_type>)
      return new object_type;
    else {
      SkyAssertMsg(false, "Tried to call new on abstract or non-default-constructible type.");
      return nullptr;
    }
  }

  virtual void DeleteObject(
    void *object
  ) const override {
    if (object)
      delete (value_type)object;
  }

  virtual void *ConstructObject(
    void *object
  ) const override {
    if constexpr (!std::is_abstract_v<object_type>)
      new (object) object_type;
    else {
      SkyAssertMsg(false, "Tried to call placement new on abstract or non-default-constructible type.");
      return nullptr;
    }

    return object;
  }

  virtual void DestructObject(
    void *object
  ) const override {
    ((object_type *)object)->~object_type();
  }

  virtual void *Upcast(
    void *const &object
  ) const override {
    if (!object)
      return nullptr;
    
    // Convert to `T *` then convert to `Object *`, in order to adjust the
    // pointer.
    return (Object *)(value_type)object;
  }

  virtual void *Downcast(
    Object *const &object
  ) const override {
    if (!object)
      return nullptr;

    return (T *)object;
  }

  virtual void *ResolveMember(
    void *ppObject,
    LPCMetaClass pClass,
    const MetaMemberVariable::Context *pContext
  ) const override {
    i32 vbtblOffs = pContext->vbtableOffset
      , vbtblSlot = pContext->vbtableSlot;
    value_type pObject = nullptr;

    DynamicCast(&pObject, ppObject, pClass);
    SkyAssert(pObject);

    uintptr_t base = (uintptr_t)pObject;
    if (vbtblSlot)
      // Simulating MSVC virtual base table addressing, resolves the address
      // of base class subobjects.
      base = base + vbtblOffs + *(int *)(*(uintptr_t *)(base + vbtblOffs) + 4 * (vbtblSlot >> 2));

    return (void *)(base + pContext->offset);
  }
};

// ----------------------------------------------------------------------------
// [SECTION] Functions
// ----------------------------------------------------------------------------

// Get an implmentation of MetaType.
LPCMetaType GetMetaType();

// Get an implmentation of GetMetaClass.
LPCMetaClass GetMetaClass();

// Get a MetaClass from global id.
LPCMetaClass GetMetaClassById(
  int globalId);

// Get a MetaClass from name.
LPCMetaClass GetMetaClassByName(
  cstring name,
  bool constString = false);

bool IsDerivedFrom(
  LPCMetaClass mc1,
  LPCMetaClass mc2);

// Get MetaType from type name.
template<typename T>
LPCMetaType GetMetaTypeByType() {
  return nullptr;
}

// Get MetaClassImpl from class.
template<typename Tp>
LPCMetaClass GetMetaClassByType() {
  return nullptr;
}

template<typename T>
static inline int GetMetaClassIdByType() {
  return MetaClassImpl<T>::Must_call_META_REGISTER_CLASS()->m_globalId;
}

// ----------------------------------------------------------------------------
// [SECTION] Object
// ----------------------------------------------------------------------------

META_DECLARE_CLASS(Object)

class Object {
public:
  Object() {
    m_metaClassId = MetaClassId(Object);
  }

  Object(int metaClassId): m_metaClassId(metaClassId) { }

  ~Object() = default;

  int m_metaClassId;
};

// ----------------------------------------------------------------------------
// [SECTION] MetaSystem
// ----------------------------------------------------------------------------

struct MetaSystemDataContainer {
  MetaStrHashMap<LPMetaType > m_metaTypes;
  MetaStrHashMap<void *> m_metaConstants;
  MetaStrHashMap<void *> m_metaVariables;
  MetaStrHashMap<void *> m_metaFunctions;
  MetaStrHashMap<LPMetaClass > m_metaClasses;
  std::unordered_map<cstring , void *> unk_6;
  std::unordered_map<cstring , void *> unk_7;
  std::unordered_map<cstring , void *> unk_8;
};

META_DECLARE_CLASS(MetaSystem)

class MetaSystem: public Object {
private:
  static void m_RecursiveInit(
    LPMetaClass pMetaClass,
    int *pIdCounter,
    int *pTopologyCounter);

public:
  static constexpr int kMaxClasses = MetaClass::kMaxClasses;

  MetaSystem()
    : Object(MetaClassId(MetaSystem))
    , m_data(nullptr)
    , m_classes()
  { }

  void Initialize();

  MetaSystemDataContainer *m_data;
  LPCMetaClass m_classes[kMaxClasses];
};

META_DECLARE_TYPE(bool)

META_DECLARE_TYPE(uint8_t)
META_DECLARE_TYPE(int8_t)
META_DECLARE_TYPE(uint16_t)
META_DECLARE_TYPE(int16_t)
META_DECLARE_TYPE(uint32_t)
META_DECLARE_TYPE(int32_t)
META_DECLARE_TYPE(uint64_t)
META_DECLARE_TYPE(int64_t)
META_DECLARE_TYPE(float)
META_DECLARE_TYPE(double)

META_DECLARE_TYPE(cstring)
META_DECLARE_TYPE(TgcString)

META_DECLARE_CLASS(MetaClass)

// #ifndef __META_HPP__
#endif
