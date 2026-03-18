#ifndef __META_HPP__
#define __META_HPP__

#include <stdint.h>
#include <type_traits>
#include <vector>
#include <string>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#include "Types.h"

// ----------------------------------------------------------------------------
// [SECTION] Declarations
// ----------------------------------------------------------------------------

class MetaFunction;
class MetaVariable;
class MetaMemberFunction;
class MetaMemberVariable;
class MetaType;
class MetaClass;

// ----------------------------------------------------------------------------
// [SECTION] Object
// ----------------------------------------------------------------------------

class Object {
public:
  Object(unsigned int metaClassId): m_metaClassId(metaClassId) { }

  ~Object() = default;

  unsigned int m_metaClassId;
};

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

};

// ----------------------------------------------------------------------------
// [SECTION] MetaMemberVariable
// ----------------------------------------------------------------------------

class MetaMemberVariable: public MetaObject<MetaMemberVariable> {

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

  virtual ~MetaType() = default;

  // Get the size of the type.
  virtual size_t SizeOfType() const = 0;

  // Get the alignment of the type.
  virtual size_t AlignOfType() const = 0;

  // Create an object of the type.
  virtual void *CreateByType() const = 0;

  // Delete an created object of the type.
  virtual void DeleteByType(void *p) const = 0;

  virtual void *ConstructByType(void *p) const = 0;

  virtual void DestructByType(void *p) const = 0;

  // Dynamic cast an object "sourceObject" of type specified by "sourceType" to
  // an object "targetObject" of type specified by this.
  virtual void DynamicCast(
    void *targetObject,
    void *sourceObject,
    const MetaType &sourceType
  ) const = 0;

  virtual bool IsNumber() const = 0;

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

  virtual const MetaType *GetSelf() const = 0;

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
    *(MetaType *)target = *(MetaType *)this;
  }

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
    const MetaType &sourceType
  ) const override {
    double num = sourceType.ToNumber(sourceObject);
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

  virtual const MetaType *GetSelf() const override {
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

  virtual void *ConstructByType(void *p) const override {
    return new (p) T;
  }

  virtual void DestructByType(void *p) const override {
    ((T *)p)->~T();
  }

  virtual void DynamicCast(
    void *targetObject,
    void *sourceObject,
    const MetaType &sourceType
  ) const override {
    *(T *)targetObject = (T)sourceType.ToNumber(sourceObject);
  }

  virtual bool IsNumber() const override {
    return true;
  }

  virtual bool IsString() const override {
    return false;
  }

  virtual lua_Number ToNumber(
    void *object
  ) const override {
    return (lua_Number)*(T *)object;
  }

  virtual const char *ToString(
    void *object
  ) const override {
    static char buf[80];
    snprintf(buf, 65, "%d", *(T *)object);
    return buf;
  }

  virtual const MetaType *GetSelf() const override {
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
    const MetaType &sourceType
  ) const override {
    const char *s = sourceType.ToString(sourceObject);
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

  virtual const MetaType *GetSelf() const override {
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

// ----------------------------------------------------------------------------
// [SECTION] MetaClass
// ----------------------------------------------------------------------------

using PFN_RegisterClass = MetaClass *(*)();

class MetaClass: public MetaType {
public:
  MetaClass(
    const char *name,
    PFN_RegisterClass parent = nullptr
  )
    : MetaType(name)
    , m_parent(parent)
    , m_globalId(0xFFFFFFFF)
    , m_topoOrder(-1)
    , m_baseTopoIdList()
    , m_metaDataContainer(nullptr)
    , m_vtableCache(nullptr)
  { }

  virtual ~MetaClass() = default;

  virtual void DynamicCast(
    void *targetObject,
    void *sourceObject,
    const MetaType &sourceType
  ) const override {

  }

  virtual double ToNumber(void *) const override { return 0; }

  virtual const char *ToString(void *) const override { return ""; }

  virtual const MetaType *GetSelf() const override { return this; }

  virtual void WriteType(
    lua_State *L,
    void *object
  ) const override {

  }

  virtual void ReadType(
    lua_State *L,
    int index,
    void *object
  ) const override {

  }

  virtual void SimpleCopy(
    void *target
  ) const override {

  }

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
  virtual void *Upcast(Object **object) const = 0;

  // Get the pointer to a subclass from an Object sub object, i.e. static_cast
  // from `Object *` to `T *`.
  virtual void *Downcast(Object **object) const = 0;

  PFN_RegisterClass m_parent;
  unsigned int m_globalId;
  int m_topoOrder;
  std::vector<int> m_baseTopoIdList;
  void *m_metaDataContainer;
  void *m_vtableCache;
};

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

  virtual void *Upcast(Object **) const override { return nullptr; }

  virtual void *Downcast(Object **) const override { return nullptr; }
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
    return sizeof(void *);
  }

  virtual size_t AlignOfType() const override {
    return alignof(void *);
  }

  virtual void *CreateByType() const override {
    return new void *{nullptr};
  }

  virtual void DeleteByType(void *p) const override {
    delete (void **)p;
  }

  virtual void *ConstructByType(void *p) const override { return p; }

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
      return new T{Must_call_META_REGISTER_CLASS()->m_globalId};
    else
      // Tried to call new on abstract or non-default-constructible type.
      return nullptr;
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
      new (object) T{Must_call_META_REGISTER_CLASS()->m_globalId};
    else
      // Tried to call placement new on abstract or non-default-constructible type.
      return nullptr;

    return object;
  }

  virtual void DestructObject(
    void *object
  ) const override {
    ((T *)object)->~T();
  }

  virtual void *Upcast(
    Object **object
  ) const override {
    if (!*object)
      return nullptr;
    return (Object *)(*(T **)object);
  }

  virtual void *Downcast(
    Object **object
  ) const override {
    if (!*object)
      return nullptr;
    return (T *)*object;
  }
};

// ----------------------------------------------------------------------------
// [SECTION] Functions
// ----------------------------------------------------------------------------

MetaType *GetMetaType();

// Get MetaType from type name.
template<typename T>
MetaType *GetMetaTypeByType() {
  return nullptr;
}

// Get MetaClassImpl from class.
template<typename Tp>
MetaClass *GetMetaClassByType() {
  return nullptr;
}

// ----------------------------------------------------------------------------
// [SECTION] MetaLua
// ----------------------------------------------------------------------------

void MetaLuaBindClass(
  MetaType *T,
  lua_State *L);

// ----------------------------------------------------------------------------
// [SECTION] MetaSystem
// ----------------------------------------------------------------------------

class MetaSystem: public Object {
  void Initialize() {

  }
};

// ----------------------------------------------------------------------------
// [SECTION] Macros
// ----------------------------------------------------------------------------

// Declare a type.
#define META_DECLARE_TYPE(T)\
template<> MetaType *GetMetaTypeByType<T>();

// Declare a class.
#define META_DECLARE_CLASS(T) \
template<> MetaClass *MetaClassImpl<T>::Must_call_META_REGISTER_CLASS();\
template<> MetaClass *GetMetaClassByType<T *>();

// Register a class.
#define META_REGISTER_CLASS(T, ...) \
static MetaClassImpl<T> g_metaClass_##T{#T, ## __VA_ARGS__};\
template<> MetaClass *MetaClassImpl<T>::Must_call_META_REGISTER_CLASS() {\
  return static_cast<MetaClassVoid *>(g_metaClass_##T.m_self);\
}\
template<> MetaClass *GetMetaClassByType<T *>() {\
  return MetaClassImpl<T>::Must_call_META_REGISTER_CLASS();\
}

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

META_DECLARE_CLASS(Object);
META_DECLARE_CLASS(MetaClass);

// #ifndef __META_HPP__
#endif
