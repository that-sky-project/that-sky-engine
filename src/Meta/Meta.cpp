#include "Meta.hpp"

MetaTypeBool g_metaType_bool{"bool"};

MetaTypeNumber<uint8_t> g_metaType_uint8_t{"uint8_t"};
MetaTypeNumber<int8_t> g_metaType_int8_t{"int8_t"};
MetaTypeNumber<uint16_t> g_metaType_uint16_t{"uint16_t"};
MetaTypeNumber<int16_t> g_metaType_int16_t{"int16_t"};
MetaTypeNumber<uint32_t> g_metaType_uint32_t{"uint32_t"};
MetaTypeNumber<int32_t> g_metaType_int32_t{"int32_t"};
MetaTypeNumber<uint64_t> g_metaType_uint64_t{"uint64_t"};
MetaTypeNumber<int64_t> g_metaType_int64_t{"int64_t"};
MetaTypeNumber<float> g_metaType_float{"float"};
MetaTypeNumber<double> g_metaType_double{"double"};

MetaTypeString<const char *> g_metaType_cstring{"cstring"};
MetaTypeString<std::string> g_metaType_TgcString{"TgcString"};

MetaClassVoid g_metaType_void{"void"};

META_REGISTER_CLASS(Object, nullptr);
META_REGISTER_CLASS(MetaClass, nullptr);

#include <stdio.h>

class Test {

};
META_REGISTER_CLASS(Test, nullptr);

class TestChild: public Test {

};
META_REGISTER_CLASS(TestChild, MetaClassImpl<Test>::Must_call_META_REGISTER_CLASS);

int main() {
  for (auto p = MetaObject<MetaType>::m_List(); p; p = p->m_prev) {
    printf("%p MetaType<%s>\n", p, p->m_name);
  }

  printf("%p\n", MetaClassImpl<Object>::Must_call_META_REGISTER_CLASS());

  return 0;
}
