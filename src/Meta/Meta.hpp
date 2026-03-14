#include <stdint.h>
#include <type_traits>
#include <vector>
#include <string>

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
// [SECTION] MetaType
// ----------------------------------------------------------------------------

class MetaType;

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

  void *unk_1;
  MetaType *m_self;
};

class MetaTypeBool: public MetaType {
public:
  MetaTypeBool(const char *name): MetaType(name) { }
};

template<typename T>
class MetaTypeNumber: public MetaType {
public:
  MetaTypeNumber(const char *name): MetaType(name) { }
};

template<typename T>
class MetaTypeString: public MetaType {
public:
  MetaTypeString(const char *name): MetaType(name) { }
};

// ----------------------------------------------------------------------------
// [SECTION] MetaClass
// ----------------------------------------------------------------------------

class MetaClassVoid: public MetaType {
public:
  MetaClassVoid(const char *name): MetaType(name) { }
};

using PFN_RegisterClass = MetaClassVoid *(__fastcall *)();

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

// Register a class.
#define META_REGISTER_CLASS(T, P) \
MetaClassImpl<T> g_metaClass_##T{#T, P};\
template<>\
MetaClassVoid *MetaClassImpl<T>::Must_call_META_REGISTER_CLASS() {\
  return (MetaClassVoid *)g_metaClass_##T.m_self;\
}

class Object { };
class MetaClass { };

// ----------------------------------------------------------------------------
// [SECTION] Functions
// ----------------------------------------------------------------------------

// Get metaclass from type pointer.
template<typename Tp>
MetaClassVoid *GetMetaClassByType() {
  using T = typename std::remove_pointer<Tp>::type;
  return MetaClassImpl<T>::Must_call_META_REGISTER_CLASS();
}
