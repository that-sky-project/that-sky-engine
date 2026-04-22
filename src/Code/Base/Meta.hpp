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

// ----------------------------------------------------------------------------
// [SECTION] MetaObject
// ----------------------------------------------------------------------------

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
    , m_fields(nullptr)
    , m_prev(nullptr)
  { }

  MetaObject(
    const MetaObject &src
  )
    : m_name(src.m_name)
    , m_fields(src.m_fields)
    , m_prev(src.m_prev)
  { }

  const char *m_name;
  void *m_fields;
  T *m_prev;
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

class MetaMemberVariable: public MetaObject<MetaMemberVariable> {
public:
  uint64_t unk_1;
  uint64_t m_offsetOf;
  int unk_2;
  const MetaType *(*m_getType)();
  const MetaClass *(*m_getClass)();
  uint64_t m_countAddress;
  int unk_4;
  uint64_t unk_5;
  uint64_t m_staticArraySize;
};

// ----------------------------------------------------------------------------
// [SECTION] MetaType
// ----------------------------------------------------------------------------

class MetaType: public MetaObject<MetaType> {
public:
  MetaType(
    const char *name
  )
    : MetaObject<MetaType>(name)
    , unk_1(nullptr)
    , m_self(this)
  {
    m_prev = MetaObject<MetaType>::m_List();
    MetaObject<MetaType>::m_List() = this;
  }

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

  void *unk_1;
  MetaType *m_self;

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
  MetaStrHashMap<MetaMemberVariable *> m_variables;
  MetaStrHashMap<MetaMemberFunction *> m_functions;
  std::unordered_map<const char *, void *> unk_3;
  std::unordered_map<const char *, void *> unk_4;
  std::unordered_map<const char *, void *> unk_5;
};

using PFN_RegisterClass = MetaClass *(*)();

// MetaClass object implementation.
// NOTE: We can consider MetaClass as MetaTypePointer. All operations of
// MetaClass is performed on the pointer to the objects.
class MetaClass: public MetaType {
protected:
  using Payload = void *;

public:
  static constexpr int kMaxClasses = 0xA00;

  MetaClass(
    const char *name,
    PFN_RegisterClass parent = nullptr
  )
    : MetaType(name)
    , m_parent(parent)
    , m_globalId(-1)
    , m_topoOrder(-1)
    , m_baseTopoIdList()
    , m_metaDataContainer(nullptr)
    , m_vtableCache(nullptr)
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

  PFN_RegisterClass m_parent;
  int m_globalId;
  int m_topoOrder;
  std::vector<int> m_baseTopoIdList;
  MetaDataContainer *m_metaDataContainer;
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
      skyAssertMsg(false, "Tried to call new on abstract or non-default-constructible type.");
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
      skyAssertMsg(false, "Tried to call placement new on abstract or non-default-constructible type.");
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

META_DECLARE_CLASS(Object);

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

META_DECLARE_CLASS(MetaSystem);

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
  { }

  void Initialize();

  MetaSystemDataContainer *m_data;
  const MetaClass *m_classes[kMaxClasses];
};

using cstring = const char *;
using TgcString = std::string;

META_DECLARE_TYPE(bool);

META_DECLARE_TYPE(uint8_t);
META_DECLARE_TYPE(int8_t);
META_DECLARE_TYPE(uint16_t);
META_DECLARE_TYPE(int16_t);
META_DECLARE_TYPE(uint32_t);
META_DECLARE_TYPE(int32_t);
META_DECLARE_TYPE(uint64_t);
META_DECLARE_TYPE(int64_t);
META_DECLARE_TYPE(float);
META_DECLARE_TYPE(double);

META_DECLARE_TYPE(cstring);
META_DECLARE_TYPE(TgcString);

META_DECLARE_CLASS(MetaClass);

// #ifndef __META_HPP__
#endif
