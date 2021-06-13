#include "pch.h"
#include <cold/meta/attribute.h>
#include <cold/meta/classinfo.h>

#include <cold/com/ianything.h>
#include <cold/com/comclass.h>
#include <cold/com/weakcomptr.h>

//#include <cold/serialization/valuerepresentation.h>
//#include <cold/serialization/json.h>
//#include <cold/serialization/runtimevaluebuilder.h>

#if 0

using namespace cold;
using namespace testing;


struct Person
{
#pragma region Info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(name),
			CLASS_FIELD(age)
		)
	)
#pragma endregion

	std::string name;
	int age;
};



struct Value1
{
#pragma region Info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(fieldData)
		)
	)
#pragma endregion

	int fieldData;
};


struct Data1
{
#pragma region Info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(person),
			CLASS_FIELD(values),
			CLASS_FIELD(fvalue)
		)
	)
#pragma endregion

	Person person;
	std::vector<Value1> values;
	float fvalue;
};



void printObjectFields(RuntimeValue& value)
{
	const auto& obj = static_cast<RuntimeObjectValue&>(value);

	for (size_t i = 0; i < obj.size(); ++i)
	{
		const auto& field = obj.field(i);

		std::cout << field.name() << std::endl;
	}

	/*switch (value.category())
	{

	}*/
}





TEST(TEST_Samples, attribs1)
{
	Data1 data;

	data.person.name = "name1";
	data.person.age = 25;

	data.values.push_back({1});
	data.values.push_back({2});


	// const auto rep = meta::represent(data.values);
	auto value = runtimeValue(const_cast<const Data1&>(data));

	printObjectFields(value);

}

#endif