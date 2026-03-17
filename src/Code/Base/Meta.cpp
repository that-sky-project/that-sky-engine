#include "Meta.hpp"

// Register a type.
#define META_REGISTER_TYPE(T)\
  template<>\
  MetaType *GetMetaTypeByType<T>() {\
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

MetaTypeBool g_metaType_bool{"bool"};
template<>
MetaType *GetMetaTypeByType<bool>() {
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

MetaClassVoid g_metaType_void{"void"};
MetaType *GetMetaType() {
  return g_metaType_void.m_self;
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
