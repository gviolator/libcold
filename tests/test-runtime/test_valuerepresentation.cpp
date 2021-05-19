#include "pch.h"


#include <cold/serialization/valuerepresentation.h>
#include <cold/serialization/valuerepresentation/primitiverepresentation.h>

using namespace cold;
using namespace testing;


namespace {

class FooPrimitive
{
public:
	
	FooPrimitive() = default;

	FooPrimitive(int value): m_value(value)
	{}

	int operator* () const {
		return m_value;
	}

private:

	int m_value = 0;


	friend std::string getPrimitiveValue(const FooPrimitive& foo_)
	{
		if (foo_.m_value == 0) {
			return "zero";
		}
		else if (foo_.m_value == 1) {
			return "one";
		}

		return "two";
	}

	friend void setPrimitiveValue(FooPrimitive& foo_, const std::string_view str)
	{
		if (str == "zero"){
			foo_.m_value = 0;
		}
		else if (str == "one"){
			foo_.m_value = 1;
		}
		else {
			foo_.m_value = 2;
		}
	}
};


struct OneFieldStruct
{
	int field = 77;

	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(field)
		)
	)
	
};



struct FooObject
{
	int field1 = 1;
	FooPrimitive field2;
	std::vector<std::string> fieldArr;
	OneFieldStruct fieldObj;
	

	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(field1),
			CLASS_FIELD(field2),
			CLASS_FIELD(fieldArr),
			CLASS_FIELD(fieldObj)
		)
	)
	
};


template<template <typename> class ExpectedRep, typename ExpectedType, typename ExpectedValueType = ExpectedType, typename T>
constexpr bool checkTypes([[maybe_unused]] const T&) {
	static_assert(std::is_same_v<T, ExpectedRep<ExpectedType>>, "Represenation type mismatch");
	static_assert(std::is_same_v<typename T::Type, ExpectedType>, "Type mismatch");
	static_assert(std::is_same_v<typename T::ValueType, ExpectedValueType>, "Value type mismatch");

	return true;
}


template<typename ... Field, typename Accessor>
bool checkField(const std::tuple<Field...>& fields, std::string_view name, Accessor accessor) {

	const auto accessOneField__ = [&]<typename T>([[maybe_unused]] const T& field) -> bool {
		
		using Rep = meta::FieldRepresentationType<T>;

		if constexpr (std::is_invocable_r_v<bool, Accessor, Rep>) {
			if (field.name() == name) {
				return accessor(field.value());
			}
		}

		return false;
	};

	const auto accessFields__ = [&]<size_t ... I>(std::index_sequence<I...>) -> bool {
		return (accessOneField__(std::get<I>(fields)) || ...);
	};

	return accessFields__(Tuple::indexes(fields));
}

}

//-----------------------------------------------------------------------------

TEST(Common_ValueRepresentation, Primitive){

	using namespace cold::meta;
	
	int intValue = 77;

	auto intRepresentation = represent(intValue);
	checkTypes<PrimitiveRepresentation, int>(intRepresentation);

	ASSERT_THAT(intRepresentation.value(), Eq(intValue));
	intRepresentation.setValue(50);
	ASSERT_THAT(intValue, Eq(50));

	FooPrimitive foo;
	auto fooRepresentation = represent(foo);
	checkTypes<PrimitiveRepresentation, FooPrimitive, std::string>(fooRepresentation);
	ASSERT_THAT(fooRepresentation.value(), Eq("zero"));
	fooRepresentation.setValue("one");
	ASSERT_THAT(*foo, Eq(1));
}

TEST(Common_ValueRepresentation, PrimitiveConst){
	using namespace cold::meta;
	
	const int intValue = 77;

	auto intRepresentation = represent(intValue);
	checkTypes<ConstPrimitiveRepresentation, int>(intRepresentation);
	ASSERT_THAT(intRepresentation.value(), Eq(intValue));

	const FooPrimitive foo(1);
	auto fooRepresentation = represent(foo);
	checkTypes<ConstPrimitiveRepresentation, FooPrimitive, std::string>(fooRepresentation);
	ASSERT_THAT(fooRepresentation.value(), Eq("one"));
}

TEST(Common_ValueRepresentation, PrimitiveAssignable){
	using namespace cold::meta;

	constexpr bool x1 = IsPrimitiveAssignable<FooPrimitive, std::wstring>;

	FooPrimitive fo{};
	setPrimitiveValue(fo, "two");

}


//-----------------------------------------------------------------------------

template<typename T>
class Common_ArrayRepresentation : public Test
{
protected:

	T m_container;
};



TYPED_TEST_SUITE_P(Common_ArrayRepresentation);

TYPED_TEST_P(Common_ArrayRepresentation, Access) {
	using namespace cold::meta;

	auto arr = represent(this->m_container);
	checkTypes<ArrayRepresentation, TypeParam>(arr);

	if (arr.size() == 0){ // size specified for std::array

		constexpr size_t Size = 30;
		constexpr size_t ReserveSize = Size;

		arr.clear(ReserveSize);
		for (size_t i = 0; i < Size; ++i){
			auto element = arr.emplaceBack();
			element.setValue(i);
			ASSERT_THAT(arr[i].value(), Eq(i));
		}
	}

	for (size_t i = 0, size = arr.size(); i < size; ++i){
		arr[i].setValue(i);
		ASSERT_THAT(arr[i].value(), Eq(i));
	}
}


TYPED_TEST_P(Common_ArrayRepresentation, ConstAccess) {
	using namespace cold::meta;

	const TypeParam& nonMutableContainer = this->m_container;

	auto arr = represent(nonMutableContainer);
	checkTypes<ConstArrayRepresentation, TypeParam>(arr);

	auto mutableArr = represent(this->m_container);

	if (arr.size() == 0){ // size specified for std::array

		constexpr size_t Size = 30;
		constexpr size_t ReserveSize = Size;

		mutableArr.clear(ReserveSize);
		for (size_t i = 0; i < Size; ++i){
			mutableArr.emplaceBack().setValue(i);
		}
	}
	else {
		for (size_t i = 0, size = mutableArr.size(); i < size; ++i){
			mutableArr[i].setValue(i);
		}
	}


	ASSERT_THAT(arr.size(), Eq(mutableArr.size()));
	for (size_t i = 0, size = arr.size(); i < size; ++i){
		ASSERT_THAT(arr[i].value(), Eq(i));
	}
}


REGISTER_TYPED_TEST_SUITE_P(Common_ArrayRepresentation,
	Access, ConstAccess);

using ArrayTypes = testing::Types<std::vector<int>, std::list<int>, std::array<int, 30>>;
INSTANTIATE_TYPED_TEST_SUITE_P(Default, Common_ArrayRepresentation, ArrayTypes);


//-----------------------------------------------------------------------------

TEST(Common_ObjectRepresenation, Access) {

	using namespace cold::meta;

	FooObject obj1;
	auto objRep = represent(obj1);
	checkTypes<ObjectRepresentation, FooObject>(objRep);

	auto fields = objRep.fields();

	ASSERT_TRUE(checkField(fields, "field1", [](PrimitiveRepresentation<int>) {
		return true;
	}));

	ASSERT_FALSE(checkField(fields, "unknown", [](PrimitiveRepresentation<int>) {
		return true;
	}));

	ASSERT_FALSE(checkField(fields, "field1", [](PrimitiveRepresentation<double>) {
		return true;
	}));

	ASSERT_TRUE(checkField(fields, "field2", [](PrimitiveRepresentation<FooPrimitive>) {
		return true;
	}));

	ASSERT_TRUE(checkField(fields, "fieldArr", [](ArrayRepresentation<std::vector<std::string>>) {
		return true;
	}));

	ASSERT_TRUE(checkField(fields, "fieldObj", [](ObjectRepresentation<OneFieldStruct>) {
		return true;
	}));
}

TEST(Common_ObjectRepresenation, ConstAccess) {

	using namespace cold::meta;

	const FooObject obj1;
	auto objRep = represent(obj1);
	checkTypes<ConstObjectRepresentation, FooObject>(objRep);

	auto fields = objRep.fields();

	ASSERT_TRUE(checkField(fields, "field1", [](PrimitiveRepresentation<int>) {
		return true;
	}));

	ASSERT_TRUE(checkField(fields, "field2", [](PrimitiveRepresentation<FooPrimitive>) {
		return true;
	}));

	ASSERT_TRUE(checkField(fields, "fieldArr", [](ConstArrayRepresentation<std::vector<std::string>>) {
		return true;
	}));

	ASSERT_TRUE(checkField(fields, "fieldObj", [](ConstObjectRepresentation<OneFieldStruct>) {
		return true;
	}));
}


