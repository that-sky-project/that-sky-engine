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
// [SECTION] Macros
// ----------------------------------------------------------------------------

// Declare a type.
#define META_DECLARE_TYPE(T)\
template<> const MetaType *GetMetaTypeByType<T>();

// Declare a class.
#define META_DECLARE_CLASS(T) \
template<> MetaClass *MetaClassImpl<T>::Must_call_META_REGISTER_CLASS();\
template<> const MetaClass *GetMetaClassByType<T *>();

// Register a class.
#define META_REGISTER_CLASS(T, ...) \
static MetaClassImpl<T> g_metaClass_##T{#T, ## __VA_ARGS__};\
template<> MetaClass *MetaClassImpl<T>::Must_call_META_REGISTER_CLASS() {\
  return static_cast<MetaClassImpl<T> *>(g_metaClass_##T.m_self);\
}\
template<> const MetaClass *GetMetaClassByType<T *>() {\
  return MetaClassImpl<T>::Must_call_META_REGISTER_CLASS();\
}

#define MetaClassId(T) MetaClassImpl<T>::Must_call_META_REGISTER_CLASS()->m_globalId

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

struct MetaStrLt {
  bool operator()(
    const char* a,
    const char* b
  ) const {
    return strcmp(a, b) == 0;
  }
};

template<typename Tv>
using MetaStrHashMap = std::unordered_map<const char *, Tv, MetaStrHash, MetaStrLt>;

using PFN_RegisterType = MetaType *(*)();
using PFN_RegisterClass = MetaClass *(*)();
using PFN_GetType = const MetaType *(*)();
using PFN_GetClass = const MetaClass *(*)();

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
    const char *name
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
  const char *m_name = nullptr;
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
  const MetaClass *(*getType)();
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
    const char *name,
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
    const char *name,
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
    const char *name,
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

  inline const MetaType *GetType() { return m_type(); }
  inline const MetaType *GetCountType() { return m_countType(); }
  inline const MetaClass *GetClass() { return m_class(); }
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
    const char *name
  )
    : MetaObject<MetaType>(name)
  {
    m_prev = MetaObject<MetaType>::m_List();
    MetaObject<MetaType>::m_List() = this;
  }

  MetaType(
    const char *name,
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
    const MetaType *sourceType
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
  virtual const char *ToString(
    void *object
  ) const = 0;

  // Return "this" when the type is not a primitive type.
  virtual const MetaClass *AsClass() const = 0;

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
  virtual MetaType *Copy() const = 0;

  virtual void SimpleCopy(
    void *target
  ) const {
    memcpy(target, this, sizeof(MetaType));
  }

  MetaType &operator=(const MetaType &) = default;

  // Unknown member, maybe for padding.
  void *unk_1 = nullptr;
  // Point to currently activated copy of the type/class.
  MetaType *m_self = this;

protected:
  // Helper functions for extracting C strings.
  static inline const char *ExtractCString(
    const char *const *ptr
  ) {
    return *ptr;
  }

  static inline const char *ExtractCString(
    std::string *ptr
  ) {
    return ptr->c_str();
  }
};

// ----------------------------------------------------------------------------
// [SECTION] MetaClass
// ----------------------------------------------------------------------------

struct MetaDataContainer {
  // Member variables of the object.
  MetaStrHashMap<MetaMemberVariable *> m_variables = {};
  // Member functions of the object.
  MetaStrHashMap<MetaMemberFunction *> m_functions = {};
  std::unordered_map<const char *, void *> unk_3 = {};
  std::unordered_map<const char *, void *> unk_4 = {};
  std::unordered_map<const char *, void *> unk_5 = {};
};

// MetaClass object implementation.
// NOTE: We can consider MetaClass as MetaTypePointer. All operations of
// MetaClass is performed on the pointer to the objects.
class MetaClass: public MetaType {
protected:
  using Payload = void *;

public:
  // The original code of TGC as below.
  static constexpr int kMaxClasses = 0xA00;

  MetaClass(
    const char *name,
    PFN_RegisterClass parent = nullptr
  )
    : MetaType(name)
    , m_parent(parent)
  { }

  MetaClass(
    const char *name,
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
    const MetaType *sourceType
  ) const override;

  virtual double ToNumber(void *) const override;

  virtual const char *ToString(void *) const override;

  virtual const MetaClass *AsClass() const override;

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
    const MetaClass *pClass,
    const MetaMemberVariable::Context *context
  ) const = 0;

  // Call the function to get the parent class.
  PFN_RegisterClass m_parent = nullptr;
  // Global id of the metaclass.
  int m_globalId = -1;
  // Topology id of the metaclass.
  int m_topoOrder = -1;
  // Topology id list of parent classes in the inheritance chain.
  std::vector<int> m_baseTopoIdList = {};
  // Point to external data container.
  MetaDataContainer *m_metaDataContainer = nullptr;
  // 
  void *m_vtableCache;
};

// A MetaClass representing "Object" types.
template<typename T>
class MetaClassImpl: public MetaClass {
public:
  static MetaClass *Must_call_META_REGISTER_CLASS() {
    return nullptr;
  }

  MetaClassImpl(
    const char *name,
    PFN_RegisterClass parent = nullptr
  )
    : MetaClass(name, parent)
  { }

  virtual size_t SizeOfType() const override {
    return sizeof(Payload);
  }

  virtual size_t AlignOfType() const override {
    return alignof(Payload);
  }

  virtual void *CreateByType() const override {
    return new Payload{nullptr};
  }

  virtual void DeleteByType(void *p) const override {
    delete (Payload *)p;
  }

  // Construct a nullptr on p, i.e. p = Payload * = void **.
  virtual void *ConstructByType(void *p) const override {
    return new (p) Payload{nullptr};
  }

  virtual void DestructByType(void *p) const override { }

  virtual bool IsNumber() const override { return false; }

  virtual bool IsString() const override { return false; }

  virtual MetaType *Copy() const override {
    return new MetaClassImpl<T>{*this};
  }

  virtual bool IsAbstract() const override {
    return std::is_abstract_v<T>;
  }

  virtual bool IsPolymorphic() const override {
    return std::is_abstract_v<T>;
  }

  virtual size_t SizeOfObject() const override {
    return sizeof(T);
  }

  virtual size_t AlignOfObject() const override {
    return alignof(T);
  }

  virtual void *NewObject() const override {
    if constexpr (!std::is_abstract_v<T>)
      return new T;
    else {
      SkyAssertMsg(false, "Tried to call new on abstract or non-default-constructible type.");
      return nullptr;
    }
  }

  virtual void DeleteObject(
    void *object
  ) const override {
    if (object)
      delete (T *)object;
  }

  virtual void *ConstructObject(
    void *object
  ) const override {
    if constexpr (!std::is_abstract_v<T>)
      new (object) T;
    else {
      SkyAssertMsg(false, "Tried to call placement new on abstract or non-default-constructible type.");
      return nullptr;
    }

    return object;
  }

  virtual void DestructObject(
    void *object
  ) const override {
    ((T *)object)->~T();
  }

  virtual void *Upcast(
    void *const &object
  ) const override {
    if (!object)
      return nullptr;
    
    // Convert to `T *` then convert to `Object *`, in order to adjust the
    // pointer.
    return (Object *)(T *)object;
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
    const MetaClass *pClass,
    const MetaMemberVariable::Context *pContext
  ) const override {
    i32 vbtblOffs = pContext->vbtableOffset
      , vbtblSlot = pContext->vbtableSlot;
    Payload pObject = nullptr;

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
const MetaType *GetMetaType();

// Get an implmentation of GetMetaClass.
const MetaClass *GetMetaClass();

// Get a MetaClass from global id.
const MetaClass *GetMetaClassById(
  int globalId);

// Get a MetaClass from name.
const MetaClass *GetMetaClassByName(
  const char *name,
  bool constString = false);

bool IsDerivedFrom(
  const MetaClass *mc1,
  const MetaClass *mc2);

// Get MetaType from type name.
template<typename T>
const MetaType *GetMetaTypeByType() {
  return nullptr;
}

// Get MetaClassImpl from class.
template<typename Tp>
const MetaClass *GetMetaClassByType() {
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
  MetaStrHashMap<MetaType *> m_metaTypes;
  MetaStrHashMap<void *> m_metaConstants;
  MetaStrHashMap<void *> m_metaVariables;
  MetaStrHashMap<void *> m_metaFunctions;
  MetaStrHashMap<MetaClass *> m_metaClasses;
  std::unordered_map<const char *, void *> unk_6;
  std::unordered_map<const char *, void *> unk_7;
  std::unordered_map<const char *, void *> unk_8;
};

META_DECLARE_CLASS(MetaSystem)

class MetaSystem: public Object {
private:
  static void m_RecursiveInit(
    MetaClass *pMetaClass,
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
  const MetaClass *m_classes[kMaxClasses];
};

using cstring = const char *;
using TgcString = std::string;

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
