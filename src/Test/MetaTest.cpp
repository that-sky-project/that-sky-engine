// Tests for the reflection system in Base/Meta.hpp.
//
// The reflection system is built out of file-scope static objects that chain
// themselves onto intrusive lists at static-initialization time, and a
// MetaSystem that walks those lists once to build the runtime tables. The test
// types below are registered with the same macros production code uses, so the
// registrations here participate in exactly the same way.

#include <gtest/gtest.h>

#include <algorithm>

#include "Base/Meta.hpp"
#include "Utils/Types.h"

// ----------------------------------------------------------------------------
// [SECTION] Test types
// ----------------------------------------------------------------------------

// META_DECLARE_CLASS must precede the class definition so MSVC sizes member
// function pointers with full generality.
class MetaTestBase;
META_DECLARE_CLASS(MetaTestBase)

// Polymorphic root of the test hierarchy. The vtable makes `Object` a non-zero
// offset inside the object, which is what exercises CastHelper.
class MetaTestBase: public Object {
public:
  MetaTestBase(): Object(MetaClassId(MetaTestBase)) { }
  explicit MetaTestBase(int metaClassId): Object(metaClassId) { }
  virtual ~MetaTestBase() = default;

  int32_t Add(int32_t a, int32_t b) { return a + b; }
  int32_t GetHealth() { return health; }
  void SetHealth(int32_t value) { health = value; }
  void Reset() { health = 0; ratio = 0.0f; }
  virtual cstring Kind() { return "base"; }

  int32_t health = 0;
  float ratio = 0.0f;
  TgcString name = {};

  int32_t slots[8] = {};
  uint16_t slotCount = 0;

  int32_t *buffer = nullptr;
  uint32_t bufferCount = 0;
};

class MetaTestDerived;
META_DECLARE_CLASS(MetaTestDerived)

class MetaTestDerived: public MetaTestBase {
public:
  MetaTestDerived(): MetaTestBase(MetaClassId(MetaTestDerived)) { }

  virtual cstring Kind() override { return "derived"; }

  int32_t extra = 0;
};

class MetaTestPod;
META_DECLARE_CLASS(MetaTestPod)

// Non-polymorphic, so `Object` sits at offset 0.
class MetaTestPod: public Object {
public:
  MetaTestPod(): Object(MetaClassId(MetaTestPod)) { }

  int32_t value = 0;
};

class MetaTestAbstract;
META_DECLARE_CLASS(MetaTestAbstract)

class MetaTestAbstract: public Object {
public:
  MetaTestAbstract(): Object(MetaClassId(MetaTestAbstract)) { }
  virtual ~MetaTestAbstract() = default;

  virtual int32_t Pure() = 0;
};

// A standalone MetaObject subclass, so the intrusive-list assertions run
// against a list nothing else touches.
class MetaTestNode: public MetaObject<MetaTestNode> {
public:
  using MetaObject<MetaTestNode>::MetaObject;
};

// ----------------------------------------------------------------------------
// [SECTION] Registrations
// ----------------------------------------------------------------------------

META_REGISTER_CLASS(MetaTestBase, nullptr)
META_REGISTER_CLASS(MetaTestDerived, MetaClassImpl<MetaTestBase>::Must_call_META_REGISTER_CLASS)
META_REGISTER_CLASS(MetaTestPod, nullptr)
META_REGISTER_CLASS(MetaTestAbstract, nullptr)

META_DATA_CLASS(MetaTestBase, Tool_Description, "A reflected test class")
META_DATA_CLASS(MetaTestBase, ClearMemory, "ZERO")

META_REGISTER_SIMPLE_MEMBER(MetaTestBase, health)
META_REGISTER_SIMPLE_MEMBER(MetaTestBase, ratio)
META_REGISTER_SIMPLE_MEMBER(MetaTestBase, name)
META_REGISTER_ARRAY_MEMBER(MetaTestBase, slots, slotCount)
META_REGISTER_ARRAY_MEMBER(MetaTestBase, buffer, bufferCount)
META_REGISTER_SIMPLE_MEMBER(MetaTestDerived, extra)

META_DATA_MEMBER_VARIABLE(MetaTestBase, health, Tool_Description, "Hit points")

META_REGISTER_FUNCTION_MEMBER(MetaTestBase, Add)
META_REGISTER_FUNCTION_MEMBER(MetaTestBase, GetHealth)
META_REGISTER_FUNCTION_MEMBER(MetaTestBase, SetHealth)
META_REGISTER_FUNCTION_MEMBER(MetaTestBase, Reset)

META_DATA_MEMBER_FUNCTION(MetaTestBase, Add, Tool_Description, "Adds two numbers")

// ----------------------------------------------------------------------------
// [SECTION] Fixture
// ----------------------------------------------------------------------------

namespace {

// MetaSystemExample is a singleton and asserts if initialized twice, so bring
// it up exactly once and share it across every test.
MetaSystemExample &TheMetaSystem() {
  static MetaSystemExample *system = [] {
    MetaSystemExample *p = new MetaSystemExample();
    p->Initialize();
    return p;
  }();
  return *system;
}

// Look a member function up the way callers do: by name, through the owning
// metaclass' data container. Only these copies have had Initialize() run on
// them, so only these have a populated signature.
const MetaMemberFunction *FindFunction(
  LPCMetaClass metaClass,
  cstring name
) {
  const MetaDataContainer *data = metaClass->GetData();
  if (!data)
    return nullptr;
  auto it = data->m_functions.find(name);
  if (it == data->m_functions.end())
    return nullptr;
  return it->second;
}

class MetaTest: public ::testing::Test {
protected:
  void SetUp() override { TheMetaSystem(); }
};

}

// ----------------------------------------------------------------------------
// [SECTION] MetaObject
// ----------------------------------------------------------------------------

TEST_F(MetaTest, MetaObjectStoresName) {
  MetaTestNode node{"node", MetaTestNode::nolist};
  EXPECT_STREQ("node", node.GetName());

  node.SetName("renamed");
  EXPECT_STREQ("renamed", node.GetName());
}

TEST_F(MetaTest, MetaObjectChainsOntoTheList) {
  // The list is per-T; nothing else registers a MetaTestNode on it.
  MetaObject<MetaTestNode>::m_List() = nullptr;

  MetaTestNode first{"first"};
  EXPECT_EQ(&first, MetaObject<MetaTestNode>::m_List());
  EXPECT_EQ(nullptr, first.GetPrev());

  MetaTestNode second{"second"};
  // Newest registration becomes the head; older ones hang off m_prev.
  EXPECT_EQ(&second, MetaObject<MetaTestNode>::m_List());
  EXPECT_EQ(&first, second.GetPrev());

  // Leave the list as we found it, the objects are about to go out of scope.
  MetaObject<MetaTestNode>::m_List() = nullptr;
}

TEST_F(MetaTest, MetaObjectNolistTagSkipsTheList) {
  MetaObject<MetaTestNode>::m_List() = nullptr;

  MetaTestNode detached{"detached", MetaTestNode::nolist};
  EXPECT_STREQ("detached", detached.GetName());
  EXPECT_EQ(nullptr, MetaObject<MetaTestNode>::m_List());
  EXPECT_EQ(nullptr, detached.GetPrev());
}

TEST_F(MetaTest, MetaObjectAttachesMetaDataInReverseOrder) {
  MetaTestNode node{"node", MetaTestNode::nolist};
  ASSERT_EQ(nullptr, node.GetFields());

  MetaData first{node, "First", "1"};
  MetaData second{node, "Second", "2"};

  // Each MetaData pushes itself onto the front of the field chain.
  EXPECT_EQ(&second, node.GetFields());
  EXPECT_EQ(&first, second.GetPrev());
  EXPECT_EQ(nullptr, first.GetPrev());
}

TEST_F(MetaTest, MetaDataExposesKeyAndValue) {
  MetaTestNode node{"node", MetaTestNode::nolist};
  MetaData field{node, "Key", "Value"};

  EXPECT_STREQ("Key", field.GetKey());
  EXPECT_STREQ("Value", field.GetValue());
}

TEST_F(MetaTest, MetaObjectLooksUpMetaDataByKey) {
  MetaTestNode node{"node", MetaTestNode::nolist};
  MetaData description{node, "Tool_Description", "Some text"};
  MetaData clear{node, "ClearMemory", "ZERO"};
  (void)description;
  (void)clear;

  EXPECT_STREQ("Some text", node.GetMetaData("Tool_Description"));
  EXPECT_STREQ("ZERO", node.GetMetaData("ClearMemory"));
  EXPECT_EQ(nullptr, node.GetMetaData("Missing"));
}

TEST_F(MetaTest, MetaDataRegisteredByMacroSurvivesMetaSystemCopy) {
  // Initialize() replaces every metaclass with a copy; the copy must carry the
  // field chain the macros attached to the original.
  LPCMetaClass metaClass = GetMetaClassByType<MetaTestBase *>();
  ASSERT_NE(nullptr, metaClass);

  EXPECT_STREQ("A reflected test class", metaClass->GetMetaData("Tool_Description"));
  EXPECT_STREQ("ZERO", metaClass->GetMetaData("ClearMemory"));
}

// ----------------------------------------------------------------------------
// [SECTION] MetaType
// ----------------------------------------------------------------------------

TEST_F(MetaTest, NumberTypesReportSizeAndAlignment) {
  EXPECT_EQ(sizeof(int32_t), GetMetaTypeByType<int32_t>()->SizeOfType());
  EXPECT_EQ(alignof(int32_t), GetMetaTypeByType<int32_t>()->AlignOfType());
  EXPECT_EQ(sizeof(double), GetMetaTypeByType<double>()->SizeOfType());
  EXPECT_EQ(sizeof(uint8_t), GetMetaTypeByType<uint8_t>()->SizeOfType());
}

TEST_F(MetaTest, NumberTypesClassifyThemselves) {
  EXPECT_TRUE(GetMetaTypeByType<int32_t>()->IsNumber());
  EXPECT_FALSE(GetMetaTypeByType<int32_t>()->IsString());
  EXPECT_EQ(nullptr, GetMetaTypeByType<int32_t>()->AsClass());
}

TEST_F(MetaTest, StringTypesClassifyThemselves) {
  EXPECT_FALSE(GetMetaTypeByType<TgcString>()->IsNumber());
  EXPECT_TRUE(GetMetaTypeByType<TgcString>()->IsString());
  EXPECT_TRUE(GetMetaTypeByType<cstring>()->IsString());
}

TEST_F(MetaTest, BoolTypeIsNeitherNumberNorString) {
  // MetaTypeBool deliberately reports false for both.
  EXPECT_FALSE(GetMetaTypeByType<bool>()->IsNumber());
  EXPECT_FALSE(GetMetaTypeByType<bool>()->IsString());
}

TEST_F(MetaTest, TypesCarryTheirRegisteredName) {
  EXPECT_STREQ("int32_t", GetMetaTypeByType<int32_t>()->GetName());
  EXPECT_STREQ("float", GetMetaTypeByType<float>()->GetName());
  EXPECT_STREQ("bool", GetMetaTypeByType<bool>()->GetName());
}

TEST_F(MetaTest, NumberTypeConvertsToNumberAndString) {
  int32_t value = 42;
  EXPECT_DOUBLE_EQ(42.0, GetMetaTypeByType<int32_t>()->ToNumber(&value));
  EXPECT_STREQ("42", GetMetaTypeByType<int32_t>()->ToString(&value));

  float ratio = 1.5f;
  EXPECT_DOUBLE_EQ(1.5, GetMetaTypeByType<float>()->ToNumber(&ratio));
  EXPECT_STREQ("1.5", GetMetaTypeByType<float>()->ToString(&ratio));
}

TEST_F(MetaTest, BoolTypeConvertsToNumberAndString) {
  bool yes = true, no = false;
  EXPECT_DOUBLE_EQ(1.0, GetMetaTypeByType<bool>()->ToNumber(&yes));
  EXPECT_DOUBLE_EQ(0.0, GetMetaTypeByType<bool>()->ToNumber(&no));
  EXPECT_STREQ("true", GetMetaTypeByType<bool>()->ToString(&yes));
  EXPECT_STREQ("false", GetMetaTypeByType<bool>()->ToString(&no));
}

TEST_F(MetaTest, StringTypeConvertsToNumberAndString) {
  TgcString text = "3.5";
  EXPECT_DOUBLE_EQ(3.5, GetMetaTypeByType<TgcString>()->ToNumber(&text));
  EXPECT_STREQ("3.5", GetMetaTypeByType<TgcString>()->ToString(&text));

  cstring raw = "7";
  EXPECT_DOUBLE_EQ(7.0, GetMetaTypeByType<cstring>()->ToNumber(&raw));
  EXPECT_STREQ("7", GetMetaTypeByType<cstring>()->ToString(&raw));
}

TEST_F(MetaTest, NumberToNumberDynamicCastGoesThroughDouble) {
  double source = 42.9;
  int32_t target = 0;
  GetMetaTypeByType<int32_t>()->DynamicCast(
    &target, &source, GetMetaTypeByType<double>());
  // Truncation, not rounding.
  EXPECT_EQ(42, target);
}

TEST_F(MetaTest, NumberToStringDynamicCast) {
  int32_t source = 42;
  TgcString target;
  GetMetaTypeByType<TgcString>()->DynamicCast(
    &target, &source, GetMetaTypeByType<int32_t>());
  EXPECT_EQ("42", target);
}

TEST_F(MetaTest, StringToNumberDynamicCast) {
  TgcString source = "128";
  int32_t target = 0;
  GetMetaTypeByType<int32_t>()->DynamicCast(
    &target, &source, GetMetaTypeByType<TgcString>());
  EXPECT_EQ(128, target);
}

TEST_F(MetaTest, NumberToBoolDynamicCast) {
  int32_t nonZero = 5, zero = 0;
  bool target = false;

  GetMetaTypeByType<bool>()->DynamicCast(
    &target, &nonZero, GetMetaTypeByType<int32_t>());
  EXPECT_TRUE(target);

  GetMetaTypeByType<bool>()->DynamicCast(
    &target, &zero, GetMetaTypeByType<int32_t>());
  EXPECT_FALSE(target);
}

TEST_F(MetaTest, CreateAndDeleteByType) {
  LPCMetaType type = GetMetaTypeByType<int32_t>();

  void *p = type->CreateByType();
  ASSERT_NE(nullptr, p);
  *static_cast<int32_t *>(p) = 11;
  EXPECT_DOUBLE_EQ(11.0, type->ToNumber(p));
  type->DeleteByType(p);
}

TEST_F(MetaTest, ConstructAndDestructByTypeRunStringLifetime) {
  LPCMetaType type = GetMetaTypeByType<TgcString>();
  alignas(TgcString) unsigned char storage[sizeof(TgcString)];

  void *p = type->ConstructByType(storage);
  ASSERT_EQ(static_cast<void *>(storage), p);

  // A default-constructed std::string must be empty; if ConstructByType did
  // not really run the constructor this would be reading garbage.
  *static_cast<TgcString *>(p) = "constructed";
  EXPECT_STREQ("constructed", type->ToString(p));

  type->DestructByType(p);
}

TEST_F(MetaTest, CopyProducesAnIndependentType) {
  LPCMetaType type = GetMetaTypeByType<int32_t>();

  LPMetaType copy = type->Copy();
  ASSERT_NE(nullptr, copy);
  EXPECT_NE(type, copy);
  EXPECT_STREQ(type->GetName(), copy->GetName());
  EXPECT_EQ(type->SizeOfType(), copy->SizeOfType());
  EXPECT_TRUE(copy->IsNumber());

  copy->SetName("renamed");
  EXPECT_STREQ("int32_t", type->GetName());

  delete copy;
}

TEST_F(MetaTest, VoidTypeIsTheFallback) {
  LPCMetaType voidType = GetMetaType();
  ASSERT_NE(nullptr, voidType);
  EXPECT_STREQ("void", voidType->GetName());
  EXPECT_EQ(0u, voidType->SizeOfType());
  EXPECT_FALSE(voidType->IsNumber());
  EXPECT_FALSE(voidType->IsString());
  // "void" is modelled as a MetaClass, so it answers AsClass().
  EXPECT_EQ(voidType->AsClass(), GetMetaClass());
}

// ----------------------------------------------------------------------------
// [SECTION] MetaClass
// ----------------------------------------------------------------------------

TEST_F(MetaTest, MetaClassIsRegisteredAndNamed) {
  LPCMetaClass metaClass = GetMetaClassByType<MetaTestBase *>();
  ASSERT_NE(nullptr, metaClass);
  EXPECT_STREQ("MetaTestBase", metaClass->GetName());
  EXPECT_EQ(metaClass, metaClass->AsClass());
}

TEST_F(MetaTest, MetaClassSizesTheObjectNotThePointer) {
  LPCMetaClass metaClass = GetMetaClassByType<MetaTestBase *>();

  // A MetaClass behaves as the metatype of `T *`.
  EXPECT_EQ(sizeof(MetaTestBase *), metaClass->SizeOfType());
  EXPECT_EQ(alignof(MetaTestBase *), metaClass->AlignOfType());

  EXPECT_EQ(sizeof(MetaTestBase), metaClass->SizeOfObject());
  EXPECT_EQ(alignof(MetaTestBase), metaClass->AlignOfObject());
}

TEST_F(MetaTest, MetaClassReportsAbstractAndPolymorphic) {
  EXPECT_FALSE(GetMetaClassByType<MetaTestBase *>()->IsAbstract());
  EXPECT_TRUE(GetMetaClassByType<MetaTestBase *>()->IsPolymorphic());

  EXPECT_FALSE(GetMetaClassByType<MetaTestPod *>()->IsAbstract());
  EXPECT_FALSE(GetMetaClassByType<MetaTestPod *>()->IsPolymorphic());

  EXPECT_TRUE(GetMetaClassByType<MetaTestAbstract *>()->IsAbstract());
  EXPECT_TRUE(GetMetaClassByType<MetaTestAbstract *>()->IsPolymorphic());
}

TEST_F(MetaTest, MetaClassLinksToItsParent) {
  LPCMetaClass base = GetMetaClassByType<MetaTestBase *>();
  LPCMetaClass derived = GetMetaClassByType<MetaTestDerived *>();

  EXPECT_EQ(base, derived->GetParent());
  EXPECT_EQ(nullptr, base->GetParent());
}

TEST_F(MetaTest, MetaSystemAssignsGlobalIds) {
  LPCMetaClass base = GetMetaClassByType<MetaTestBase *>();
  LPCMetaClass derived = GetMetaClassByType<MetaTestDerived *>();

  ASSERT_NE(-1, base->GetId());
  ASSERT_NE(-1, derived->GetId());
  EXPECT_NE(base->GetId(), derived->GetId());

  // A parent is always initialized before its children.
  EXPECT_LT(base->GetId(), derived->GetId());

  EXPECT_EQ(base, GetMetaClassById(base->GetId()));
  EXPECT_EQ(derived, GetMetaClassById(derived->GetId()));
}

TEST_F(MetaTest, MetaSystemAssignsTopologyOrderToBaseClasses) {
  LPCMetaClass base = GetMetaClassByType<MetaTestBase *>();
  LPCMetaClass derived = GetMetaClassByType<MetaTestDerived *>();

  // Only classes that are actually a base of something get a topology order.
  EXPECT_NE(-1, base->GetTopoId());
  EXPECT_EQ(-1, derived->GetTopoId());

  // The derived class records its base's topology order.
  EXPECT_NE(
    derived->m_baseTopoIdList.end(),
    std::find(
      derived->m_baseTopoIdList.begin(),
      derived->m_baseTopoIdList.end(),
      base->GetTopoId()));
}

TEST_F(MetaTest, MetaClassLookupByName) {
  EXPECT_EQ(
    GetMetaClassByType<MetaTestBase *>(), GetMetaClassByName("MetaTestBase"));
  EXPECT_EQ(
    GetMetaClassByType<MetaTestDerived *>(), GetMetaClassByName("MetaTestDerived"));
  EXPECT_EQ(nullptr, GetMetaClassByName("NoSuchClass"));
}

TEST_F(MetaTest, ObjectsCarryTheirMetaClassId) {
  MetaTestBase base;
  MetaTestDerived derived;

  EXPECT_EQ(GetMetaClassByType<MetaTestBase *>()->GetId(), base.GetMetaClassId());
  EXPECT_EQ(GetMetaClassByType<MetaTestDerived *>()->GetId(), derived.GetMetaClassId());
}

TEST_F(MetaTest, NewAndDeleteObject) {
  LPCMetaClass metaClass = GetMetaClassByType<MetaTestBase *>();

  void *raw = metaClass->NewObject();
  ASSERT_NE(nullptr, raw);

  MetaTestBase *object = static_cast<MetaTestBase *>(raw);
  EXPECT_EQ(0, object->health);
  EXPECT_EQ(metaClass->GetId(), object->GetMetaClassId());

  metaClass->DeleteObject(raw);
}

TEST_F(MetaTest, ConstructAndDestructObjectInPlace) {
  LPCMetaClass metaClass = GetMetaClassByType<MetaTestBase *>();
  alignas(MetaTestBase) unsigned char storage[sizeof(MetaTestBase)];

  void *raw = metaClass->ConstructObject(storage);
  ASSERT_EQ(static_cast<void *>(storage), raw);

  MetaTestBase *object = static_cast<MetaTestBase *>(raw);
  EXPECT_EQ(0, object->health);
  EXPECT_STREQ("base", object->Kind());

  metaClass->DestructObject(raw);
}

TEST_F(MetaTest, UpcastAdjustsForThePolymorphicBase) {
  MetaTestDerived derived;
  void *self = &derived;

  Object *upcast = static_cast<Object *>(
    GetMetaClassByType<MetaTestDerived *>()->Upcast(self));
  EXPECT_EQ(static_cast<Object *>(&derived), upcast);

  // MetaTestBase has a vtable, so Object does not sit at offset 0 and the
  // upcast must have moved the pointer.
  EXPECT_NE(self, static_cast<void *>(upcast));
}

TEST_F(MetaTest, UpcastAndDowncastRoundTrip) {
  MetaTestDerived derived;
  derived.extra = 77;

  LPCMetaClass metaClass = GetMetaClassByType<MetaTestDerived *>();
  void *self = &derived;

  Object *upcast = static_cast<Object *>(metaClass->Upcast(self));
  ASSERT_NE(nullptr, upcast);

  void *downcast = metaClass->Downcast(upcast);
  EXPECT_EQ(self, downcast);
  EXPECT_EQ(77, static_cast<MetaTestDerived *>(downcast)->extra);
}

TEST_F(MetaTest, UpcastAndDowncastPassNullThrough) {
  LPCMetaClass metaClass = GetMetaClassByType<MetaTestBase *>();

  void *nullObject = nullptr;
  Object *nullBase = nullptr;
  EXPECT_EQ(nullptr, metaClass->Upcast(nullObject));
  EXPECT_EQ(nullptr, metaClass->Downcast(nullBase));
}

TEST_F(MetaTest, DynamicCastToTheSameClassIsIdentity) {
  MetaTestBase object;
  MetaTestBase *source = &object, *target = nullptr;

  LPCMetaClass metaClass = GetMetaClassByType<MetaTestBase *>();
  metaClass->DynamicCast(&target, &source, metaClass);

  EXPECT_EQ(&object, target);
}

TEST_F(MetaTest, DynamicCastUpTheHierarchySucceeds) {
  MetaTestDerived object;
  object.health = 5;

  MetaTestDerived *source = &object;
  MetaTestBase *target = nullptr;

  GetMetaClassByType<MetaTestBase *>()->DynamicCast(
    &target, &source, GetMetaClassByType<MetaTestDerived *>());

  ASSERT_NE(nullptr, target);
  EXPECT_EQ(static_cast<MetaTestBase *>(&object), target);
  EXPECT_EQ(5, target->health);
  EXPECT_STREQ("derived", target->Kind());
}

TEST_F(MetaTest, DynamicCastDownToTheRuntimeTypeSucceeds) {
  MetaTestDerived object;
  object.extra = 9;

  // Statically a MetaTestBase *, dynamically a MetaTestDerived.
  MetaTestBase *source = &object;
  MetaTestDerived *target = nullptr;

  GetMetaClassByType<MetaTestDerived *>()->DynamicCast(
    &target, &source, GetMetaClassByType<MetaTestBase *>());

  ASSERT_NE(nullptr, target);
  EXPECT_EQ(&object, target);
  EXPECT_EQ(9, target->extra);
}

TEST_F(MetaTest, DynamicCastDownToAWrongTypeFails) {
  MetaTestBase object;
  MetaTestBase *source = &object;
  MetaTestDerived *target = reinterpret_cast<MetaTestDerived *>(~0ull);

  // The object really is a MetaTestBase, so narrowing must refuse.
  GetMetaClassByType<MetaTestDerived *>()->DynamicCast(
    &target, &source, GetMetaClassByType<MetaTestBase *>());

  EXPECT_EQ(nullptr, target);
}

TEST_F(MetaTest, DynamicCastAcrossUnrelatedClassesFails) {
  MetaTestPod object;
  MetaTestPod *source = &object;
  MetaTestBase *target = reinterpret_cast<MetaTestBase *>(~0ull);

  GetMetaClassByType<MetaTestBase *>()->DynamicCast(
    &target, &source, GetMetaClassByType<MetaTestPod *>());

  EXPECT_EQ(nullptr, target);
}

TEST_F(MetaTest, DynamicCastFromANonClassTypeFails) {
  int32_t number = 1;
  void *source = &number;
  MetaTestBase *target = reinterpret_cast<MetaTestBase *>(~0ull);

  GetMetaClassByType<MetaTestBase *>()->DynamicCast(
    &target, &source, GetMetaTypeByType<int32_t>());

  EXPECT_EQ(nullptr, target);
}

TEST_F(MetaTest, MetaClassToNumberAndToStringAreEmpty) {
  MetaTestBase object;
  MetaTestBase *source = &object;
  LPCMetaClass metaClass = GetMetaClassByType<MetaTestBase *>();

  EXPECT_DOUBLE_EQ(0.0, metaClass->ToNumber(&source));
  EXPECT_STREQ("", metaClass->ToString(&source));
}

// ----------------------------------------------------------------------------
// [SECTION] MetaMemberVariable
// ----------------------------------------------------------------------------

TEST_F(MetaTest, MemberVariableKnowsItsNameTypeAndClass) {
  EXPECT_STREQ("health", g_metaMemberVariable_MetaTestBase_health.GetName());
  EXPECT_EQ(
    GetMetaTypeByType<int32_t>(),
    g_metaMemberVariable_MetaTestBase_health.GetType());
  EXPECT_EQ(
    GetMetaClassByType<MetaTestBase *>(),
    g_metaMemberVariable_MetaTestBase_health.GetClass());

  EXPECT_EQ(
    GetMetaTypeByType<float>(),
    g_metaMemberVariable_MetaTestBase_ratio.GetType());
  EXPECT_EQ(
    GetMetaTypeByType<TgcString>(),
    g_metaMemberVariable_MetaTestBase_name.GetType());
}

TEST_F(MetaTest, MemberVariableOfADerivedClassReportsThatClass) {
  EXPECT_EQ(
    GetMetaClassByType<MetaTestDerived *>(),
    g_metaMemberVariable_MetaTestDerived_extra.GetClass());
}

TEST_F(MetaTest, SimpleMemberIsNotAnArray) {
  const MetaMemberVariable &member = g_metaMemberVariable_MetaTestBase_health;

  EXPECT_FALSE(member.IsArray());
  EXPECT_FALSE(member.IsStaticArray());
  EXPECT_FALSE(member.IsDynamicArray());
  EXPECT_EQ(0u, member.StaticArraySize());
  EXPECT_EQ(nullptr, member.CountAddress());
}

TEST_F(MetaTest, StaticArrayMemberReportsItsExtent) {
  const MetaMemberVariable &member = g_metaMemberVariable_MetaTestBase_slots;

  EXPECT_TRUE(member.IsArray());
  EXPECT_TRUE(member.IsStaticArray());
  EXPECT_FALSE(member.IsDynamicArray());
  EXPECT_EQ(8u, member.StaticArraySize());

  // The element type is recorded, not the array type.
  EXPECT_EQ(GetMetaTypeByType<int32_t>(), member.GetType());
  EXPECT_EQ(GetMetaTypeByType<uint16_t>(), member.GetCountType());
  EXPECT_NE(nullptr, member.CountAddress());
}

TEST_F(MetaTest, DynamicArrayMemberHasNoStaticExtent) {
  const MetaMemberVariable &member = g_metaMemberVariable_MetaTestBase_buffer;

  EXPECT_TRUE(member.IsArray());
  EXPECT_FALSE(member.IsStaticArray());
  EXPECT_TRUE(member.IsDynamicArray());
  EXPECT_EQ(0u, member.StaticArraySize());

  EXPECT_EQ(GetMetaTypeByType<int32_t>(), member.GetType());
  EXPECT_EQ(GetMetaTypeByType<uint32_t>(), member.GetCountType());
}

TEST_F(MetaTest, ResolveMemberReadsThroughTheMemberPointer) {
  MetaTestBase object;
  object.health = 99;
  object.ratio = 2.5f;

  MetaTestBase *self = &object;
  LPCMetaClass metaClass = GetMetaClassByType<MetaTestBase *>();

  void *health = metaClass->ResolveMember(
    &self, metaClass, g_metaMemberVariable_MetaTestBase_health.Address());
  ASSERT_NE(nullptr, health);
  EXPECT_EQ(&object.health, health);
  EXPECT_EQ(99, *static_cast<int32_t *>(health));

  void *ratio = metaClass->ResolveMember(
    &self, metaClass, g_metaMemberVariable_MetaTestBase_ratio.Address());
  EXPECT_EQ(&object.ratio, ratio);
  EXPECT_FLOAT_EQ(2.5f, *static_cast<float *>(ratio));
}

TEST_F(MetaTest, ResolveMemberWritesThroughTheMemberPointer) {
  MetaTestBase object;
  MetaTestBase *self = &object;
  LPCMetaClass metaClass = GetMetaClassByType<MetaTestBase *>();

  void *health = metaClass->ResolveMember(
    &self, metaClass, g_metaMemberVariable_MetaTestBase_health.Address());
  *static_cast<int32_t *>(health) = 1234;

  EXPECT_EQ(1234, object.health);
}

TEST_F(MetaTest, ResolveMemberRoundTripsThroughTheTypeErasedInterface) {
  MetaTestBase object;
  object.health = 7;

  MetaTestBase *self = &object;
  LPCMetaClass metaClass = GetMetaClassByType<MetaTestBase *>();
  const MetaMemberVariable &member = g_metaMemberVariable_MetaTestBase_health;

  // How a serializer would use it: resolve the address, then go through the
  // member's own metatype to read the value.
  void *address = metaClass->ResolveMember(&self, metaClass, member.Address());
  EXPECT_DOUBLE_EQ(7.0, member.GetType()->ToNumber(address));
  EXPECT_STREQ("7", member.GetType()->ToString(address));
}

TEST_F(MetaTest, ResolveMemberFindsABaseMemberOnADerivedObject) {
  MetaTestDerived object;
  object.health = 21;

  // The caller has a MetaTestBase * view of a MetaTestDerived object.
  MetaTestBase *self = &object;
  LPCMetaClass metaClass = GetMetaClassByType<MetaTestBase *>();

  void *health = metaClass->ResolveMember(
    &self, metaClass, g_metaMemberVariable_MetaTestBase_health.Address());
  EXPECT_EQ(&object.health, health);
  EXPECT_EQ(21, *static_cast<int32_t *>(health));
}

TEST_F(MetaTest, ResolveMemberResolvesTheArrayCount) {
  MetaTestBase object;
  object.slotCount = 3;
  object.slots[0] = 10;
  object.slots[1] = 20;
  object.slots[2] = 30;

  MetaTestBase *self = &object;
  LPCMetaClass metaClass = GetMetaClassByType<MetaTestBase *>();
  const MetaMemberVariable &member = g_metaMemberVariable_MetaTestBase_slots;

  void *count = metaClass->ResolveMember(
    &self, metaClass, member.CountAddress());
  EXPECT_EQ(3u, static_cast<size_t>(member.GetCountType()->ToNumber(count)));

  void *slots = metaClass->ResolveMember(&self, metaClass, member.Address());
  EXPECT_EQ(static_cast<void *>(object.slots), slots);
  EXPECT_EQ(20, static_cast<int32_t *>(slots)[1]);
}

TEST_F(MetaTest, CountReadsAStaticArrayLength) {
  MetaTestBase object;
  object.slotCount = 3;

  // Count() takes the address of the object pointer, the same "void *ppObject"
  // convention ResolveMember uses.
  MetaTestBase *self = &object;
  EXPECT_EQ(3u, g_metaMemberVariable_MetaTestBase_slots.Count(&self));

  object.slotCount = 0;
  EXPECT_EQ(0u, g_metaMemberVariable_MetaTestBase_slots.Count(&self));

  // The count is read live from the object, it is not the static extent.
  object.slotCount = 8;
  EXPECT_EQ(8u, g_metaMemberVariable_MetaTestBase_slots.Count(&self));
}

TEST_F(MetaTest, CountReadsADynamicArrayLength) {
  int32_t storage[4] = {1, 2, 3, 4};

  MetaTestBase object;
  object.buffer = storage;
  object.bufferCount = 4;

  MetaTestBase *self = &object;
  EXPECT_EQ(4u, g_metaMemberVariable_MetaTestBase_buffer.Count(&self));
}

TEST_F(MetaTest, CountAgreesWithResolveMember) {
  MetaTestBase object;
  object.slotCount = 5;

  MetaTestBase *self = &object;
  LPCMetaClass metaClass = GetMetaClassByType<MetaTestBase *>();
  const MetaMemberVariable &member = g_metaMemberVariable_MetaTestBase_slots;

  void *count = metaClass->ResolveMember(&self, metaClass, member.CountAddress());
  EXPECT_EQ(
    static_cast<size_t>(member.GetCountType()->ToNumber(count)),
    member.Count(&self));
}

TEST_F(MetaTest, CountOfANonArrayMemberIsZero) {
  MetaTestBase object;
  object.health = 99;

  // A simple member has no count address; Count() falls through to the void
  // metatype, which reports 0 rather than dereferencing anything.
  MetaTestBase *self = &object;
  EXPECT_EQ(0u, g_metaMemberVariable_MetaTestBase_health.Count(&self));
}

TEST_F(MetaTest, CountReadsABaseArrayThroughADerivedObject) {
  MetaTestDerived object;
  object.slotCount = 6;

  MetaTestBase *self = &object;
  EXPECT_EQ(6u, g_metaMemberVariable_MetaTestBase_slots.Count(&self));
}

TEST_F(MetaTest, MemberVariablesChainOntoTheGlobalList) {
  bool foundHealth = false, foundSlots = false;
  for (auto *p = MetaObject<MetaMemberVariable>::m_List(); p; p = p->GetPrev()) {
    if (!strcmp(p->GetName(), "health") && p->GetClass() == GetMetaClassByType<MetaTestBase *>())
      foundHealth = true;
    if (!strcmp(p->GetName(), "slots"))
      foundSlots = true;
  }

  EXPECT_TRUE(foundHealth);
  EXPECT_TRUE(foundSlots);
}

TEST_F(MetaTest, MemberVariableCarriesMetaData) {
  EXPECT_STREQ(
    "Hit points",
    g_metaMemberVariable_MetaTestBase_health.GetMetaData("Tool_Description"));
  EXPECT_EQ(
    nullptr, g_metaMemberVariable_MetaTestBase_health.GetMetaData("Missing"));
}

// ----------------------------------------------------------------------------
// [SECTION] MetaMemberFunction
// ----------------------------------------------------------------------------

TEST_F(MetaTest, MemberFunctionKnowsItsNameAndClass) {
  EXPECT_STREQ("Add", g_metaMemberFunction_MetaTestBase_Add.GetName());
  EXPECT_EQ(
    GetMetaClassByType<MetaTestBase *>(),
    g_metaMemberFunction_MetaTestBase_Add.GetClass());
}

TEST_F(MetaTest, MemberFunctionCarriesMetaData) {
  EXPECT_STREQ(
    "Adds two numbers",
    g_metaMemberFunction_MetaTestBase_Add.GetMetaData("Tool_Description"));
}

TEST_F(MetaTest, MetaSystemRegistersMemberFunctionsOnTheirClass) {
  LPCMetaClass metaClass = GetMetaClassByType<MetaTestBase *>();
  ASSERT_NE(nullptr, metaClass->GetData());

  EXPECT_NE(nullptr, FindFunction(metaClass, "Add"));
  EXPECT_NE(nullptr, FindFunction(metaClass, "GetHealth"));
  EXPECT_NE(nullptr, FindFunction(metaClass, "SetHealth"));
  EXPECT_NE(nullptr, FindFunction(metaClass, "Reset"));
  EXPECT_EQ(nullptr, FindFunction(metaClass, "NoSuchFunction"));
}

TEST_F(MetaTest, SignatureOfATwoArgumentFunction) {
  const MetaMemberFunction *fn =
    FindFunction(GetMetaClassByType<MetaTestBase *>(), "Add");
  ASSERT_NE(nullptr, fn);

  const FunctionSignature &sig = fn->GetSignature();
  EXPECT_EQ(2, sig.argCount);
  EXPECT_EQ(GetMetaTypeByType<int32_t>(), sig.ret);
  ASSERT_NE(nullptr, sig.argArray);
  EXPECT_EQ(GetMetaTypeByType<int32_t>(), sig.argArray[0]);
  EXPECT_EQ(GetMetaTypeByType<int32_t>(), sig.argArray[1]);
}

TEST_F(MetaTest, SignatureOfANullaryFunction) {
  const MetaMemberFunction *fn =
    FindFunction(GetMetaClassByType<MetaTestBase *>(), "GetHealth");
  ASSERT_NE(nullptr, fn);

  const FunctionSignature &sig = fn->GetSignature();
  EXPECT_EQ(0, sig.argCount);
  EXPECT_EQ(GetMetaTypeByType<int32_t>(), sig.ret);
}

TEST_F(MetaTest, SignatureOfAVoidFunctionUsesTheVoidType) {
  const MetaMemberFunction *setter =
    FindFunction(GetMetaClassByType<MetaTestBase *>(), "SetHealth");
  ASSERT_NE(nullptr, setter);

  EXPECT_EQ(1, setter->GetSignature().argCount);
  EXPECT_EQ(GetMetaType(), setter->GetSignature().ret);
  EXPECT_EQ(GetMetaTypeByType<int32_t>(), setter->GetSignature().argArray[0]);

  const MetaMemberFunction *reset =
    FindFunction(GetMetaClassByType<MetaTestBase *>(), "Reset");
  ASSERT_NE(nullptr, reset);
  EXPECT_EQ(0, reset->GetSignature().argCount);
  EXPECT_EQ(GetMetaType(), reset->GetSignature().ret);
}

TEST_F(MetaTest, RawFunctionPointerRoundTripsThroughTypeErasure) {
  const MetaMemberFunction *fn =
    FindFunction(GetMetaClassByType<MetaTestBase *>(), "Add");
  ASSERT_NE(nullptr, fn);

  // Member function pointers are stored erased; recovering the real type must
  // give back a callable pointer. This is what /vmg guarantees.
  auto add = reinterpret_cast<int32_t (MetaTestBase::*)(int32_t, int32_t)>(
    fn->Function());

  MetaTestBase object;
  EXPECT_EQ(7, (object.*add)(3, 4));
}

TEST_F(MetaTest, ApplyInvokesAFunctionThroughTypeErasedArguments) {
  MetaTestBase object;

  int32_t a = 3, b = 4, result = 0;
  Variable args[2] = {
    {&a, GetMetaTypeByType<int32_t>()},
    {&b, GetMetaTypeByType<int32_t>()},
  };
  Variable retval = {&result, GetMetaTypeByType<int32_t>()};

  Apply(&MetaTestBase::Add, &object, &retval, args, 2);
  EXPECT_EQ(7, result);
}

TEST_F(MetaTest, ApplyCoercesArgumentsToTheDeclaredTypes) {
  MetaTestBase object;

  // Neither argument has the declared int32_t type; both must be converted.
  double a = 40.7;
  TgcString b = "2";
  int32_t result = 0;

  Variable args[2] = {
    {&a, GetMetaTypeByType<double>()},
    {&b, GetMetaTypeByType<TgcString>()},
  };
  Variable retval = {&result, GetMetaTypeByType<int32_t>()};

  Apply(&MetaTestBase::Add, &object, &retval, args, 2);
  EXPECT_EQ(42, result);
}

TEST_F(MetaTest, ApplyCoercesTheReturnValue) {
  MetaTestBase object;

  int32_t a = 3, b = 4;
  TgcString result;

  Variable args[2] = {
    {&a, GetMetaTypeByType<int32_t>()},
    {&b, GetMetaTypeByType<int32_t>()},
  };
  Variable retval = {&result, GetMetaTypeByType<TgcString>()};

  Apply(&MetaTestBase::Add, &object, &retval, args, 2);
  EXPECT_EQ("7", result);
}

TEST_F(MetaTest, ApplyInvokesAVoidFunctionAndReportsNoResult) {
  MetaTestBase object;
  object.health = 1;

  int32_t value = 55;
  Variable args[1] = {{&value, GetMetaTypeByType<int32_t>()}};
  Variable retval = {reinterpret_cast<void *>(~0ull), GetMetaTypeByType<int32_t>()};

  Apply(&MetaTestBase::SetHealth, &object, &retval, args, 1);

  EXPECT_EQ(55, object.health);
  // A void call clears the result slot and marks it as the void type.
  EXPECT_EQ(nullptr, retval.value);
  EXPECT_EQ(GetMetaType(), retval.type);
}

TEST_F(MetaTest, ApplyInvokesANullaryFunction) {
  MetaTestBase object;
  object.health = 31;

  int32_t result = 0;
  Variable retval = {&result, GetMetaTypeByType<int32_t>()};

  Apply(&MetaTestBase::GetHealth, &object, &retval, nullptr, 0);
  EXPECT_EQ(31, result);
}

TEST_F(MetaTest, ApplyDispatchesVirtuallyOnTheRuntimeType) {
  MetaTestDerived object;
  MetaTestBase *self = &object;

  TgcString result;
  Variable retval = {&result, GetMetaTypeByType<TgcString>()};

  Apply(&MetaTestBase::Kind, self, &retval, nullptr, 0);
  EXPECT_EQ("derived", result);
}
