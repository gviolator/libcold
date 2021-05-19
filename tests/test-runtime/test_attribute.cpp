#include "pch.h"
#include <cold/meta/attribute.h>

using namespace cold;
using namespace testing;

namespace {

struct StrAttribute : meta::Attribute
{
	static const std::string defaultValue() { return "default"; }

	std::string value;
 
	StrAttribute(std::string value_): value(std::move(value_))
	{}
};

struct IntAttribute : meta::Attribute
{
	static constexpr int DefaultValue = 777;

	int value;

	IntAttribute(int value_): value(value_)
	{}
};


struct Attribute1 : meta::Attribute
{};

template<typename T>
struct Attribute1Value : meta::AttributeValue<Attribute1> {
	using type = T;
};


struct Attribute2 : meta::Attribute
{};

template<typename T>
struct Attribute2Value : meta::AttributeValue<Attribute2> {
	using type = T;
};

class MyType1 {

	CLASS_ATTRIBUTES
		StrAttribute{StrAttribute::defaultValue()},
		Attribute1Value<int>{}

	END_CLASS_ATTRIBUTES
};



}


TEST(Common_Attribute, AttributeDefined)
{
	static_assert(meta::AttributeDefined<MyType1, StrAttribute>);
	static_assert(meta::AttributeDefined<MyType1, Attribute1>);
	static_assert(!meta::AttributeDefined<MyType1, IntAttribute>);
	static_assert(!meta::AttributeDefined<MyType1, Attribute2>);
}


TEST(Common_Attribute, AttributeValueType)
{
	static_assert(std::is_same_v<meta::AttributeValueType<MyType1, StrAttribute>, StrAttribute>);
	static_assert(std::is_same_v<meta::AttributeValueType<MyType1, Attribute1>, Attribute1Value<int>>);
}


TEST(Common_Attribute, AttributeValue)
{
	const auto value1 = meta::attributeValue<MyType1, StrAttribute>().value;
	ASSERT_THAT(value1, Eq(StrAttribute::defaultValue()));
}

