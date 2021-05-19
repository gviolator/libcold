#if 0
#include "pch.h"
#include "meta/classinfo.h"
#include <type_traits>
#include <string>


using namespace testing;
using namespace cold;


namespace {

struct SuperBase
{};

struct Base1 : virtual SuperBase
{
	CLASS_INFO(
		CLASS_BASE(SuperBase)
	)

};

struct Base2
{};

struct Base3 : Base2, virtual SuperBase
{
	CLASS_INFO(
		CLASS_BASE(Base2, SuperBase)
	)
};

struct MyClass : Base1, Base3
{
	CLASS_INFO(
		CLASS_BASE(Base1, Base3)
	)
};



struct StructWithFields
{
	static constexpr const char* DefaultStr = "default";
	static constexpr int DefaultInt = 77;

	int intField = DefaultInt;
	std::string strField = DefaultStr;

	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(intField),
			CLASS_FIELD(strField)
		)
	)
};


class ClassWithNamedFields
{
public:
	static constexpr int DefaultInt1 = 11;
	static constexpr int DefaultInt2 = 22;

	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_NAMED_FIELD(m_field1, "field1"),
			CLASS_NAMED_FIELD(m_field2, "field2")
		)
	)

private:
	int m_field1 = DefaultInt1;
	int m_field2 = DefaultInt2;
};


class ClassCompoundFileds : public StructWithFields, public ClassWithNamedFields
{
	CLASS_INFO(
		CLASS_BASE(StructWithFields, ClassWithNamedFields),
		CLASS_FIELDS(
			CLASS_FIELD(m_field3),
			CLASS_FIELD(m_field4)
		)
	)

private:
	int m_field3 = DefaultInt1;
	float m_field4 = DefaultInt2;
};


template<typename ExpectedT, typename ... ExpectedAttribs, typename T, typename ... Attribs>
AssertionResult checkField(const meta::FieldInfo<T, Attribs...> field, std::string_view name, std::optional<T> value = std::nullopt)
{
	static_assert(std::is_same_v<ExpectedT, T>, "field type mismatch");
	static_assert((std::is_same_v<ExpectedAttribs, Attribs> && ...), "field attribute type mismatch");

	if (!field.name() != name) {
		return AssertionFailure() << format("Field name ({0}) mismatch ({1})", field.name(), name);
	}

	return AssertionSuccess();
}


template<typename ... Fields, typename ... ExpectedFields>
AssertionResult checkFields(const std::tuple<Fields...>& fields, ExpectedFields... expected) {

	using namespace cold::meta;

	static_assert(sizeof ... (Fields) == sizeof ... (ExpectedFields), "Invalid fields count");
	static_assert((std::is_same_v<Fields, ExpectedFields> && ...), "Field value type mismatch");

	const auto compareAllFields = [&] <size_t ... I>(std::index_sequence<I...> indexes, const std::tuple<ExpectedFields...> fields2) {
		return ((std::get<I>(fields).name() == std::get<I>(fields2).name()) && ... );
	};

	const bool success = compareAllFields(cold::Tuple::Indexes<decltype(fields)>{}, std::tuple {expected...});
	if (!success) {
		return AssertionFailure() << "Field name mismatch";
	}

	return AssertionSuccess();
}


}

TEST(Common_ClassInfo, Bases) {

	using ActualDirectBase = meta::ClassDirectBase<MyClass>;
	static_assert(std::is_same_v<ActualDirectBase, TypeList<Base1, Base3>>);

	using ActualAllBase = meta::ClassAllBase<MyClass>;
	static_assert(std::is_same_v<ActualAllBase, TypeList<Base1, Base3, SuperBase, Base2, SuperBase>>);

	using ActualAllUniqueBase = meta::ClassAllUniqueBase<MyClass>;  
	static_assert(std::is_same_v<ActualAllUniqueBase, TypeList<Base1, Base3, SuperBase, Base2>>);
}


TEST(Common_ClassInfo, Fields) {
	const StructWithFields instance;
	auto fields = meta::classFields(instance);

	const auto result = checkFields(fields,
		meta::FieldInfo<const int>(&instance.intField, "intField"),
		meta::FieldInfo<const std::string>(&instance.strField, "strField")
	);

	ASSERT_TRUE(result);
}


TEST(Common_ClassInfo, NamedFields) {
	ClassWithNamedFields instance;
	auto fields = meta::classFields(instance);

	const auto result = checkFields(fields,
		meta::FieldInfo<int>(nullptr, "field1"),
		meta::FieldInfo<int>(nullptr, "field2")
	);

	ASSERT_TRUE(result);
}

TEST(Common_ClassInfo, FieldInheritance) {
	ClassCompoundFileds instance;
	auto fields = meta::classFields(instance);

	const auto result = checkFields(fields,
		meta::FieldInfo<int>(nullptr, "intField"),
		meta::FieldInfo<std::string>(nullptr, "strField"),
		meta::FieldInfo<int>(nullptr, "field1"),
		meta::FieldInfo<int>(nullptr, "field2"),
		meta::FieldInfo<int>(nullptr, "m_field3"),
		meta::FieldInfo<float>(nullptr, "m_field4")
	);

	ASSERT_TRUE(result);
}
#endif
