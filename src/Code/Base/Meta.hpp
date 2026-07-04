#ifndef __META_HPP__
#define __META_HPP__

#include <stdint.h>
#include <array>
#include <tuple>
#include <type_traits>
#include <vector>
#include <string>
#include <unordered_map>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include "Utils/StlAllocator.hpp"
#include "Utils/Types.h"
#include "Utils/Assert.hpp"

// ----------------------------------------------------------------------------
// [SECTION] HeaderMisc
// ----------------------------------------------------------------------------

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

#if defined(_MSC_VER)
// Force /vmg member function pointers.
// NOTE: The sizeof() expression of Intellisense is usually incorrect.
#pragma pointers_to_members(full_generality, virtual_inheritance)
#endif

// ----------------------------------------------------------------------------
// [SECTION] Macros
// ----------------------------------------------------------------------------

// Register a metadata key-value pair for a type.
// Example:
//   META_DATA_TYPE(CameraLensType, Enum_Type, "CameraLensType")
#define META_DATA_TYPE(_Type, _Key, _Value) \
  static MetaData g_metaDataType_ ## _Type ## _ ## _Key = {\
    g_metaType_ ## _Type,\
    #_Key,\
    _Value\
  };

// Register a metadata key-value pair for a class.
// Example:
//   META_DATA_CLASS(EventBarn, ClearMemory, "ZERO")
#define META_DATA_CLASS(_Class, _Key, _Value) \
  static MetaData g_metaDataClass_ ## _Class ## _ ## _Key = {\
    g_metaClass_ ## _Class,\
    #_Key,\
    _Value\
  };

// Register a metadata for a constant.
// Example:
//   META_DATA_CONSTANT(kCameraGuideZoom_None, Enum_Type, "CameraGuideZoom")
#define META_DATA_CONSTANT(_Name, _Key, _Value) \
  static MetaData g_metaDataConstant_ ## _Name ## _ ## _Key = {\
    g_metaConstant_ ## _Name,\
    #_Key,\
    _Value\
  };

// Register a metadata kv pair for a member variable.
// Example:
//   META_DATA_MEMBER_VARIABLE(Event, autoStart, Tool_Description, "ZERO")
#define META_DATA_MEMBER_VARIABLE(_Class, _Name, _Key, _Value) \
  static MetaData g_metaDataMemberVariable_ ## _Class ## _ ## _Name ## _ ## _Key = {\
    g_metaMemberVariable_ ## _Class ## _ ## _Name,\
    #_Key,\
    _Value\
  };

// Register a metadata kv pair for a member function.
#define META_DATA_MEMBER_FUNCTION(_Class, _Name, _Key, _Value) \
  static MetaData g_metaDataMemberFunction_ ## _Class ## _ ## _Name ## _ ## _Key = {\
    g_metaMemberFunction_ ## _Class ## _ ## _Name,\
    #_Key,\
    _Value\
  };

// Declare a type. Place the macro in header files.
// Example:
//   META_DECLARE_TYPE(uint8_t)
#define META_DECLARE_TYPE(T)\
  template<> LPCMetaType GetMetaTypeByType<T>();\
  namespace Adl {\
    LPCMetaType GetMetaTypeImpl(const T &, Force);\
  }

// Declare a class. Place the macro in header files, and must before the declaration
// of the class:
//
// Because MSVC calculates the size of a member function pointer by recording whether
// the type is complete when it first encounters the class declaration, if this macro
// is placed after the class's complete declaration, the calculated size of
// MetaMemberFunction may not match expectations.
//
// Example:
//   META_DECLARE_CLASS(Heap)
#define META_DECLARE_CLASS(T) \
  template<> LPMetaClass MetaClassImpl<T>::Must_call_META_REGISTER_CLASS();\
  template<> LPCMetaType GetMetaTypeByType<T *>();\
  template<> LPCMetaClass GetMetaClassByType<T *>();

// Register a type. Place the macro in cpp files.
// Example:
//   META_REGISTER_TYPE(Heap)
#define META_REGISTER_TYPE(T)\
  template<>\
  LPCMetaType GetMetaTypeByType<T>() {\
    return g_metaType_##T.GetActive();\
  }\
  namespace Adl {\
    LPCMetaType GetMetaTypeImpl(const T &, Force) {\
      return g_metaType_##T.GetActive();\
    }\
  }

// Register a number type.
// Example:
//   META_REGISTER_TYPE_NUMBER(CameraGuideZoom)
#define META_REGISTER_TYPE_NUMBER(T)\
  static MetaTypeNumber<T> g_metaType_##T{#T};\
  META_REGISTER_TYPE(T)

// Register a string type. The ExtraceCString function for the type must be
// declared.
// Example:
//   META_REGISTER_TYPE_STRING(std::string)
#define META_REGISTER_TYPE_STRING(T)\
  static MetaTypeString<T> g_metaType_##T{#T};\
  META_REGISTER_TYPE(T)

// Register a class.
// Example:
//   META_REGISTER_CLASS(lua_State)
#define META_REGISTER_CLASS(T, ...) \
  static MetaClassImpl<T> g_metaClass_##T{#T, ## __VA_ARGS__};\
  template<> LPMetaClass MetaClassImpl<T>::Must_call_META_REGISTER_CLASS() {\
    return static_cast<MetaClassImpl<T> *>(g_metaClass_##T.m_self);\
  }\
  template<> LPCMetaClass GetMetaClassByType<T *>() {\
    return MetaClassImpl<T>::Must_call_META_REGISTER_CLASS();\
  }\
  template<> LPCMetaType GetMetaTypeByType<T *>() {\
    return g_metaClass_##T.GetActive();\
  }

// Get the class id of a class.
#define MetaClassId(T) MetaClassImpl<T>::Must_call_META_REGISTER_CLASS()->GetId()

// Register a member variable.
// Example:
//   META_REGISTER_STATIC_MEMBER(Event, autoStart)
// Reference to:
//   bool Event::autoStart;
#define META_REGISTER_SIMPLE_MEMBER(_Class, _Name) \
  static MetaMemberVariable g_metaMemberVariable_ ## _Class ## _ ## _Name = {\
    #_Name,\
    &_Class::_Name\
  };

// Register a member array.
// Example:
//   META_REGISTER_STATIC_MEMBER(Timeline, timelineNodes, timelineNodesCount)
// Reference to:
//   TimelineNode Timeline::timelineNodes[128];
//   uint16_t Timeline::timelineNodesCount;
#define META_REGISTER_ARRAY_MEMBER(_Class, _Name, _CountName) \
  static MetaMemberVariable g_metaMemberVariable_ ## _Class ## _ ## _Name = {\
    #_Name,\
    &_Class::_Name,\
    &_Class::_CountName\
  };

// Register a member function.
// Example:
//   META_REGISTER_FUNCTION_MEMBER(Event, Start)
#define META_REGISTER_FUNCTION_MEMBER(_Class, _Name) \
  static MetaMemberFunction g_metaMemberFunction_ ## _Class ## _ ## _Name = {\
    #_Name,\
    &_Class::_Name\
  };

// Register a constant value.
// Example:
//   META_REGISTER_CONSTANT(CameraGuideZoom, kCameraGuideZoom_None, kCameraGuideZoom_None)
#define META_REGISTER_CONSTANT(_Type, _Name, _Value) \
  static MetaConstantImpl<_Type> g_metaConstant_ ## _Name = {\
    #_Name,\
    GetMetaTypeByType<_Type>,\
    _Value\
  };

// Register an enum value (constant).
// Example:
//   META_REGISTER_ENUM(CameraGuideZoom, kCameraGuideZoom_None)
#define META_REGISTER_ENUM(_Type, _Name) \
  static MetaConstantImpl<_Type> g_metaConstant_ ## _Name = {\
    #_Name,\
    GetMetaTypeByType<_Type>,\
    _Name\
  };\
  META_DATA_CONSTANT(_Name, "Enum_Type", #_Type)

// ----------------------------------------------------------------------------
// [SECTION] Declarations
// ----------------------------------------------------------------------------

class MetaData;
class MetaFunction;
class MetaVariable;
class MetaMemberFunction;
class MetaMemberVariable;
class MetaConstant;
class MetaType;
class MetaClass;
class MetaSystem;
class Object;

// FNV-1a hash for cstring.
struct MetaStrHash {
  std::size_t operator()(
    cstring s
  ) const {
    return std::hash<std::string>{}(s);
  }
};

struct MetaStrEq {
  bool operator()(
    cstring a,
    cstring b
  ) const {
    return strcmp(a, b) == 0;
  }
};

template<typename Tv>
using MetaStrHashMap = std::unordered_map<cstring , Tv, MetaStrHash, MetaStrEq>;

using LPMetaData = MetaData *;
using LPMetaType = MetaType *;
using LPMetaClass = MetaClass *;
using LPCMetaData = const MetaData *;
using LPCMetaType = const MetaType *;
using LPCMetaClass = const MetaClass *;

using PFN_RegisterType = LPMetaType (*)();
using PFN_RegisterClass = LPMetaClass (*)();
using PFN_GetType = LPCMetaType (*)();
using PFN_GetClass = LPCMetaClass (*)();

// Argument-dependent lookup.
namespace Adl {
struct Force {};
}

// Type-erased class.
class VoidClass { };

// Type-erased variable storage.
struct Variable {
  // Raw value.
  void *value;
  // MetaType.
  LPCMetaType type;
};

template<typename Class, typename Fn>
void ApplyWrapper(
  void (VoidClass::*fn)(void),
  void *pObject,
  Variable *retval,
  Variable *args,
  u32 argCount);

template<typename T> LPCMetaType GetMetaTypeByType();
template<typename Tp> LPCMetaClass GetMetaClassByType();

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

// ----------------------------------------------------------------------------
// [SECTION] MetaData
// ----------------------------------------------------------------------------

// Store metadata in KV pairs.
class MetaData {
public:
  // Basic constructor.
  MetaData(
    cstring name,
    void *aux = nullptr
  )
    : m_name(name)
    , m_fields(aux)
  { }

  // Copy constructor.
  MetaData(
    const MetaData &src
  )
    : m_name(src.m_name)
    , m_fields(src.m_fields)
    , m_prev(src.m_prev)
  { }

  // Attach a MetaData key-value pair to a MetaObject.
  MetaData(
    MetaData &target,
    cstring key,
    cstring value
  )
    : m_name(key)
    , m_fields((void *)value)
  {
    m_prev = (LPMetaData)target.m_fields;
    target.m_fields = (void *)this;
  }

  inline cstring GetName() const { return m_name; }
  inline void SetName(cstring name) { m_name = name; }
  inline LPMetaData GetPrev() const { return m_prev; }
  inline void *GetFields() const { return m_fields; }

protected:
  // Name of the object.
  cstring m_name = nullptr;
  // External descriptors.
  void *m_fields = nullptr;
  // Previous object, build a chain list for initialization.
  MetaData *m_prev = nullptr;
};

// ----------------------------------------------------------------------------
// [SECTION] MetaObject
// ----------------------------------------------------------------------------

// Represents an object.
template<typename T>
class MetaObject: public MetaData {
public:
  static inline T *&m_List() {
    static T *p = nullptr;
    return p;
  }

  MetaObject(
    cstring name,
    void *aux = nullptr
  )
    : MetaData(name, aux)
  { }

  MetaObject(
    const MetaObject &src
  )
    : MetaData(src)
    , unk_1(src.unk_1)
  { }

  // Get the next(prev) object.
  inline T *GetPrev() const { return (T *)m_prev; }

  // Get a MetaData value.
  inline cstring GetMetaData(
    cstring key
  ) const {
    for (LPCMetaData p = (LPCMetaData)m_fields; p; p = p->GetPrev())
      if (!strcmp(p->GetName(), key))
        return (cstring)p->GetFields();
    return nullptr;
  }

protected:
  // Unknown member, maybe for padding or alignment.
  void *unk_1 = nullptr;
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
public:

protected:
  cstring m_sourceFile = nullptr;
  void *m_address = nullptr;
  PFN_RegisterType m_type = nullptr;
  void *unk[3];
};

// ----------------------------------------------------------------------------
// [SECTION] MetaConstant
// ----------------------------------------------------------------------------

// Global constant.
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

  inline LPCMetaType GetType() const { return m_type(); }

protected:
  void *m_valuePtr = nullptr;
  PFN_RegisterType m_type = nullptr;
};

// Global constant implement.
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

  inline T GetValue() const { return *scast<T *>(m_valuePtr); }
};

// ----------------------------------------------------------------------------
// [SECTION] MetaMemberFunction
// ----------------------------------------------------------------------------

// Function signature storage.
class FunctionSignature {
public:
  FunctionSignature() = default;

  template<typename Class, typename Ret, typename ...Args>
  void Initialize(Ret (Class::*)(Args...)) {
    static std::array<LPCMetaType, sizeof...(Args)> args = {
      GetMetaTypeByType<Args>()...
    };

    m_ret = GetMetaTypeByType<Ret>();
    m_args = args.data();
    m_argCount = sizeof...(Args);
  }

  template<typename Class, typename ...Args>
  void Initialize(void (Class::*)(Args...)) {
    static std::array<LPCMetaType, sizeof...(Args)> args = {
      GetMetaTypeByType<Args>()...
    };
    m_ret = GetMetaType();
    m_args = args.data();
    m_argCount = sizeof...(Args);
  }

protected:
  LPCMetaType m_ret = nullptr;
  const LPCMetaType *m_args = nullptr;
  int m_argCount = 0;
};

// Templated function signature.
template<typename Fn>
void InitializeFunctionSignature(
  FunctionSignature *sig
) {
  sig->Initialize(Fn());
}

// Represents a member function.
class MetaMemberFunction: public MetaObject<MetaMemberFunction> {
protected:
  using PFN_VoidMember = void (VoidClass::*)(void);
  using PFN_InitFunctionSignature = void (*)(
    FunctionSignature *);
  using PFN_ApplyWrapper = void (*)(
    void (VoidClass::*)(void), void *, Variable *, Variable *, u32);

public:
  template<typename Class, typename Ret, typename ...Args>
  MetaMemberFunction(
    cstring name,
    Ret (Class::*member)(Args...)
  )
    : MetaObject<MetaMemberFunction>(name)
    , m_function(rcast<PFN_VoidMember>(member))
    , m_initSignature(InitializeFunctionSignature<Ret (Class::*)(Args...)>)
    , m_applyWrapper(ApplyWrapper<Class, Ret (Class::*)(Args...)>)
    , m_class(GetMetaClassByType<Class *>)
  {
    m_prev = MetaObject<MetaMemberFunction>::m_List();
    MetaObject<MetaMemberFunction>::m_List() = this;
  }

  // Allow explicit copy,
  explicit MetaMemberFunction(
    const MetaMemberFunction &) = default;

  // but cannot move or copy with `=`.
  const MetaMemberFunction &operator=(
    const MetaMemberFunction &) = delete;
  MetaMemberFunction(
    const MetaMemberFunction &&) = delete;
  const MetaMemberFunction &operator=(
    const MetaMemberFunction &&) = delete;

  // Initialize function signature.
  inline void Initialize() { m_initSignature(&m_signature); }
  // Get the class where the function from.
  inline LPCMetaClass GetClass() const { return m_class(); }

protected:
  // Function signature.
  FunctionSignature m_signature = {};
  // Registered function pointer.
  PFN_VoidMember m_function = nullptr;
  // Apply wrapper.
  PFN_ApplyWrapper m_applyWrapper = nullptr;
  // Function signature initializer.
  PFN_InitFunctionSignature m_initSignature = nullptr;
  // The class which the member function from.
  PFN_GetClass m_class = nullptr;
};

// ----------------------------------------------------------------------------
// [SECTION] MetaMemberVariable
// ----------------------------------------------------------------------------

// Represents a member variable.
// WARN: The implementation of MetaMemberVariable IS NOT type-safe; passing an
// incorrect object/type may cause a crash.
class MetaMemberVariable: public MetaObject<MetaMemberVariable> {
public:
  // void ** can be dereference, is a better intermediate type.
  using PMember = void *VoidClass::*;

private:
  void ChainList() {
    m_prev = MetaObject<MetaMemberVariable>::m_List();
    MetaObject<MetaMemberVariable>::m_List() = this;
  }

public:
  // Simple member.
  template<typename Class, typename Type>
  MetaMemberVariable(
    cstring name,
    Type Class::*member
  )
    : MetaObject<MetaMemberVariable>(name)
    , m_type(GetMetaTypeByType<Type>)
    , m_class(GetMetaClassByType<Class *>)
    , m_address(rcast<PMember>(member))
  {
    ChainList();
  }

  // Dynamic array.
  template<typename Class, typename Type, typename CountType>
  MetaMemberVariable(
    cstring name,
    Type *Class::*member,
    CountType Class::*count
  )
    : MetaObject<MetaMemberVariable>(name)
    , m_class(GetMetaClassByType<Class *>)
    , m_type(GetMetaTypeByType<Type>)
    , m_address(rcast<PMember>(member))
    , m_countType(GetMetaTypeByType<CountType>)
    , m_countAddress(rcast<PMember>(count))
  {
    ChainList();
  }

  // Static array.
  template<typename Class, typename Type, typename CountType, std::size_t N>
  MetaMemberVariable(
    cstring name,
    Type (Class::*member)[N],
    CountType Class::*count
  )
    : MetaObject<MetaMemberVariable>(name)
    , m_class(GetMetaClassByType<Class *>)
    , m_type(GetMetaTypeByType<Type>)
    , m_address(rcast<PMember>(member))
    , m_countType(GetMetaTypeByType<CountType>)
    , m_countAddress(rcast<PMember>(count))
    , m_staticArraySize(N)
  {
    ChainList();
  }

  // Allow explicit copy,
  explicit MetaMemberVariable(
    const MetaMemberVariable &) = default;

  // but cannot move or copy with `=`.
  const MetaMemberVariable &operator=(
    const MetaMemberVariable &) = delete;
  MetaMemberVariable(
    const MetaMemberVariable &&) = delete;
  const MetaMemberVariable &operator=(
    const MetaMemberVariable &&) = delete;

  // Get the count of a member array, from an object.
  inline size_t Count(void *pObj) const;

  // Get the original member pointer.
  // WARN: The function below is not type-safe. Please ensure the correct object
  // is passed when using member pointers.
  inline PMember Address() const { return m_address; }
  inline PMember CountAddress() const { return m_countAddress; }

  // - Context functions.

  inline LPCMetaType GetType() const { return m_type(); }
  inline LPCMetaType GetCountType() const { return m_countType(); }
  inline LPCMetaClass GetClass() const { return m_class(); }
  inline size_t StaticArraySize() const { return m_staticArraySize; }
  inline bool IsArray() const { return !!m_countAddress; }
  inline bool IsStaticArray() const { return IsArray() && !!m_staticArraySize; }
  inline bool IsDynamicArray() const { return IsArray() && !m_staticArraySize; }

protected:
  // Offset of the member variable in the object.
  PMember m_address = nullptr;
  // Get the type of this member variable.
  PFN_GetType m_type = GetMetaType;
  // Get the class of this member variable belongs to.
  PFN_GetClass m_class = GetMetaClass;
  // Descriptor of array length.
  PMember m_countAddress = nullptr;
  // Type of the array length.
  PFN_GetType m_countType = GetMetaType;
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

  inline LPMetaType GetActive() { return m_self; }
  inline void SetActive(LPMetaType p) { m_self = p; }

protected:
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
  std::unordered_map<cstring, void *> unk_2 = {};
  // Member functions of the object.
  MetaStrHashMap<MetaMemberFunction *> m_functions = {};
  std::unordered_map<cstring, void *> unk_4 = {};
  std::unordered_map<cstring, void *> unk_5 = {};
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
    MetaMemberVariable::PMember context
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
  void *m_vtableCache = nullptr;
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

protected:
  // Helper struct for constructing objects.
  template<typename _Obj, bool _Abstract = std::is_abstract<_Obj>::value>
  struct ConstructHelper {
    static inline _Obj *Impl(void *object) {
      new (object) _Obj;
      return static_cast<_Obj *>(object);
    }

    static inline _Obj *Impl() {
      static_assert(std::is_default_constructible<T>::value,
        "T must be default-constructible");
      return new _Obj;
    }
  };

  template<typename _Obj>
  struct ConstructHelper<_Obj, true> {
    static inline _Obj *Impl(void *) {
      SkyAssertMsg(false, "Tried to call placement new on abstract type.");
      return nullptr;
    }

    static inline _Obj *Impl() {
      SkyAssertMsg(false, "Tried to call new on abstract or non-default-constructible type.");
      return nullptr;
    }
  };

  // Helper for pointer upcast and downcast.
  template <typename _Obj, bool _Base = std::is_base_of<Object, _Obj>::value>
  struct CastHelper {
    static inline void *Up(void *const &object) {
      if (!object)
        return nullptr;
      // Convert to `T *` then convert to `Object *`, in order to adjust the
      // pointer.
      return static_cast<Object *>(static_cast<_Obj *>(object));
    }

    static inline void *Down(Object *const &object) {
      if (!object)
        return nullptr;
      return static_cast<_Obj *>(object);
    }
  };

  // If T is not a derived class of Object, return nullptr directly.
  template<typename _Obj>
  struct CastHelper<_Obj, false> {
    static inline _Obj *Up(void *const &object) {
      return nullptr;
    }

    static inline _Obj *Down(Object *const &object) {
      return nullptr;
    }
  };

public:
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
    return ConstructHelper<object_type>::Impl();
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
    return ConstructHelper<object_type>::Impl(object);
  }

  virtual void DestructObject(
    void *object
  ) const override {
    ((object_type *)object)->~object_type();
  }

  virtual void *Upcast(
    void *const &object
  ) const override {
    return CastHelper<object_type>::Up(object);
  }

  virtual void *Downcast(
    Object *const &object
  ) const override {
    return CastHelper<object_type>::Down(object);
  }

  virtual void *ResolveMember(
    void *ppObject,
    LPCMetaClass pClass,
    MetaMemberVariable::PMember pContext
  ) const override {
    value_type pObject = nullptr;

    DynamicCast(&pObject, ppObject, pClass);
    SkyAssert(pObject);
    u08 object_type::*pMember = rcast<u08 object_type::*>(pContext);

    // pContext is a pointer to void*, so the resolved type is void*.
    // In fact we don't care the actual type (and the alignment), the function
    // only applys the offset.
    return (void *)&(pObject->*pMember);
  }
};

// ----------------------------------------------------------------------------
// [SECTION] Functions
// ----------------------------------------------------------------------------

// Convert one type-erased Variable into the concrete parameter type Arg and
// return it by value, so the result can be dropped straight into a call's
// argument list (see ApplyImpl / ApplyVoidImpl).
template<typename Arg>
inline Arg ConvertArg(
  const Variable &v
) {
  // Uninitialised local matching the disassembly's bare stack slot; DynamicCast
  // writes the whole value before it is read.
  Arg result;
  GetMetaTypeByType<Arg>()->DynamicCast(&result, v.value, v.type);
  return result;
}

// Invoke a non-void member function through type-erased arguments, then convert
// its return value into the caller-supplied retval slot. Index-pack worker.
template<typename Class, typename Ret, typename ...Args, size_t ...I>
inline void ApplyImpl(
  Ret (Class::*fn)(Args...),
  Class *pObject,
  Variable *retval,
  Variable *args,
  std::index_sequence<I...>
) {
  // Call the member function on the object and keep its raw return value.
  // The arguments are converted inline as part of the call expression.
  Ret ret = (pObject->*fn)(ConvertArg<Args>(args[I])...);

  // Marshal the raw return value into the type the caller asked for.
  retval->type->DynamicCast(retval->value, &ret, GetMetaTypeByType<Ret>());
}

// Invoke a void member function through type-erased arguments; report an empty
// result in retval. Index-pack worker.
template<typename Class, typename ...Args, size_t ...I>
inline void ApplyVoidImpl(
  void (Class::*fn)(Args...),
  Class *pObject,
  Variable *retval,
  Variable *args,
  std::index_sequence<I...>
) {
  // A void function produces no value: null payload + the "void" metatype.
  retval->value = nullptr;
  retval->type = GetMetaType();

  // Call the member function; arguments are converted inline, nothing to
  // marshal back.
  (pObject->*fn)(ConvertArg<Args>(args[I])...);
}

// Public entry point for non-void member functions: check arity, then dispatch.
template<typename Class, typename Ret, typename ...Args>
void Apply(
  Ret (Class::*fn)(Args...),
  Class *pObject,
  Variable *retval,
  Variable *args,
  u32 argCount
) {
  // The supplied argument count must match the function's arity.
  SkyAssert(sizeof...(Args) == argCount);
  ApplyImpl(fn, pObject, retval, args, std::index_sequence_for<Args...>{});
}

// Public entry point for void member functions: check arity, then dispatch.
template<typename Class, typename ...Args>
void Apply(
  void (Class::*fn)(Args...),
  Class *pObject,
  Variable *retval,
  Variable *args,
  u32 argCount
) {
  // The supplied argument count must match the function's arity.
  SkyAssert(sizeof...(Args) == argCount);
  ApplyVoidImpl(fn, pObject, retval, args, std::index_sequence_for<Args...>{});
}

// Type-restoring trampoline stored in MetaMemberFunctionImpl::m_applyWrapper.
// Recovers the concrete member-function type from the type-erased pointers the
// dispatcher passes, then forwards to the matching Apply overload.
template<typename Class, typename Fn>
void ApplyWrapper(
  void (VoidClass::*fn)(void),
  void *pObject,
  Variable *retval,
  Variable *args,
  u32 argCount
) {
  // Undo the erasure performed by the dispatcher and call the real function.
  Apply(
    rcast<Fn>(fn),
    scast<Class *>(pObject),
    retval,
    args,
    argCount);
}

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

// Get MetaClass global id from class.
template<typename T>
static inline int GetMetaClassIdByType() {
  return MetaClassImpl<T>::Must_call_META_REGISTER_CLASS()->m_globalId;
}

// ----------------------------------------------------------------------------
// [SECTION] Object
// ----------------------------------------------------------------------------

META_DECLARE_CLASS(Object)

// Base class for all objects registered in MetaSystem.
class Object {
public:
  Object(): m_metaClassId(MetaClassId(Object)) { }
  Object(int metaClassId): m_metaClassId(metaClassId) { }
  ~Object() = default;

  inline int GetMetaClassId() const { return m_metaClassId; }

protected:
  int m_metaClassId;
};

// ----------------------------------------------------------------------------
// [SECTION] MetaSystem
// ----------------------------------------------------------------------------

// MetaSystem lookup table.
struct MetaSystemDataContainer {
  MetaStrHashMap<LPMetaType> m_metaTypes;
  MetaStrHashMap<MetaConstant *> m_metaConstants;
  MetaStrHashMap<MetaVariable *> m_metaVariables;
  MetaStrHashMap<MetaFunction *> m_metaFunctions;
  MetaStrHashMap<LPMetaClass> m_metaClasses;
  std::unordered_map<cstring, void *> unk_6;
  std::unordered_map<cstring, void *> unk_7;
  std::unordered_map<cstring, void *> unk_8;
};

META_DECLARE_CLASS(MetaSystem)

// Example MetaSystem declaration.
class MetaSystem: public Object {
private:
  static void m_RecursiveInit(
    LPMetaClass pMetaClass,
    int *pIdCounter,
    int *pTopologyCounter);

public:
  static constexpr int kMaxClasses = MetaClass::kMaxClasses;

  // Users can implement their own `g_metaSystem` and replace it via this
  // function to store more classes.
  static void SetMetaSystem(
    MetaSystem *ptr);

  MetaSystem()
    : Object(MetaClassId(MetaSystem))
    , m_data(nullptr)
    , m_classes()
  { }

  // This initialize function is only as an example. In most cases, you need to
  // write your own MetaSystem and MetaSystem::Initialize to apply to the game's
  // existing MetaSystem.
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
