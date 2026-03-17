#ifndef __META_HPP__
#define __META_HPP__

#include <stdint.h>
#include <type_traits>
#include <vector>
#include <string>

// ----------------------------------------------------------------------------
// [SECTION] Declarations
// ----------------------------------------------------------------------------

class MetaFunction;
class MetaVariable;
class MetaMemberFunction;
class MetaMemberVariable;
class MetaType;

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
  virtual size_t GetSizeOf() = 0;

  // Get the alignment of the type.
  virtual size_t GetAlignOf() = 0;

  // Create an object of the type.
  virtual void *CreateObject() = 0;

  // Delete an created object of the type.
  virtual void DeleteObject(void *p) = 0;

  //virtual void f1() = 0;
  //virtual void f2() = 0;

  void *unk_1;
  MetaType *m_self;
};

class MetaTypeBool: public MetaType {
public:
  MetaTypeBool(const char *name): MetaType(name) { }

  virtual size_t GetSizeOf() override {
    return 1;
  }

  virtual size_t GetAlignOf() override {
    return 1;
  }

  virtual void *CreateObject() override {
    return new bool;
  }

  virtual void DeleteObject(void *p) override {
    delete (bool *)p;
  }
};

template<typename T>
class MetaTypeNumber: public MetaType {
public:
  MetaTypeNumber(const char *name): MetaType(name) { }

  virtual size_t GetSizeOf() override {
    return sizeof(T);
  }

  virtual size_t GetAlignOf() override {
    return alignof(T);
  }

  virtual void *CreateObject() override {
    return new T;
  }

  virtual void DeleteObject(void *p) override {
    delete (T *)p;
  }
};

template<typename T>
class MetaTypeString: public MetaType {
public:
  MetaTypeString(const char *name): MetaType(name) { }

  virtual size_t GetSizeOf() override {
    return sizeof(T);
  }

  virtual size_t GetAlignOf() override {
    return alignof(T);
  }

  virtual void *CreateObject() override {
    return new T;
  }

  virtual void DeleteObject(void *p) override {
    delete (T *)p;
  }
};

// ----------------------------------------------------------------------------
// [SECTION] MetaClass
// ----------------------------------------------------------------------------

class MetaClassVoid: public MetaType {
public:
  MetaClassVoid(const char *name): MetaType(name) { }

  virtual size_t GetSizeOf() override {
    return 0;
  }

  virtual size_t GetAlignOf() override {
    return 0;
  }

  virtual void *CreateObject() override {
    return nullptr;
  }

  virtual void DeleteObject(void *p) override { }
};

using PFN_RegisterClass = MetaClassVoid *(*)();

template<typename T>
class MetaClassImpl: public MetaClassVoid {
public:
  static MetaClassVoid *Must_call_META_REGISTER_CLASS() {
    return nullptr;
  }

  MetaClassImpl(
    const char *name,
    PFN_RegisterClass parent = nullptr
  )
    : MetaClassVoid(name)
    , m_parent(parent)
    , m_globalId(0xFFFFFFFF)
    , m_topoOrder(-1)
    , m_baseTopoIdList()
    , m_metaDataContainer(nullptr)
    , m_vtableCache(nullptr)
  { }

  PFN_RegisterClass m_parent;
  unsigned int m_globalId;
  int m_topoOrder;
  std::vector<int> m_baseTopoIdList;
  void *m_metaDataContainer;
  void *m_vtableCache;
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
MetaClassVoid *GetMetaClassByType() {
  //using T = typename std::remove_pointer<Tp>::type;
  //return MetaClassImpl<T>::Must_call_META_REGISTER_CLASS();
  return nullptr;
}

// ----------------------------------------------------------------------------
// [SECTION] Object
// ----------------------------------------------------------------------------

class Object { };
class MetaClass { };

// ----------------------------------------------------------------------------
// [SECTION] MetaSystem
// ----------------------------------------------------------------------------

class MetaSystem: public MetaClass {
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
template<> MetaClassVoid *MetaClassImpl<T>::Must_call_META_REGISTER_CLASS();\
template<> MetaClassVoid *GetMetaClassByType<T *>();

// Register a class.
#define META_REGISTER_CLASS(T, P) \
static MetaClassImpl<T> g_metaClass_##T{#T, P};\
template<> MetaClassVoid *MetaClassImpl<T>::Must_call_META_REGISTER_CLASS() {\
  return static_cast<MetaClassVoid *>(g_metaClass_##T.m_self);\
}\
template<> MetaClassVoid *GetMetaClassByType<T *>() {\
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
