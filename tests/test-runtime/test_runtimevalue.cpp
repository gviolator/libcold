#include "pch.h"
#include <cold/utils/scopeguard.h>
#include <cold/utils/stringconv.h>
#include <cold/utils/uid.h>

#include <cold/serialization/runtimevalue.h>
#include <cold/serialization/runtimevaluebuilder.h>

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

using namespace testing;
using namespace cold;



using DefaultEncoding = rapidjson::UTF8<char>;

using JsonDocument = rapidjson::GenericDocument<DefaultEncoding>;
using JsonValue = rapidjson::GenericValue<DefaultEncoding>;

namespace {

struct OneFieldStruct1
{
	int field = 77;

	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(field)
		)
	)
	
};



struct FooObject1
{
	int field1 = 1;
	std::vector<unsigned> fieldArr;
	OneFieldStruct1 fieldObj;
	

	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(field1),
			CLASS_FIELD(fieldArr),
			CLASS_FIELD(fieldObj)
		)
	)
	
};
}

using StrBuffer = rapidjson::GenericStringBuffer<DefaultEncoding>;
using JsonStrWriter = rapidjson::PrettyWriter<StrBuffer>;

void writeJsonValue(JsonStrWriter& writer, const RuntimeValue::Ptr value)
{
	if (value->is<RuntimePrimitiveValue>())
	{
		if (value->is<RuntimeInt32Value>()) {
			writer.Int(value->as<RuntimeInt32Value&>().get());
		}
		else if (value->is<RuntimeUInt32Value>()) {
			writer.Uint(value->as<RuntimeUInt32Value&>().get());
		}
	}
	else if (value->is<RuntimeArrayValue>())
	{
		writer.StartArray();

		SCOPE_Success{
			writer.EndArray();
		};

		const RuntimeArrayValue& array = value->as<RuntimeArrayValue&>();

		for (size_t i = 0, sz = array.size(); i < sz; ++i)
		{
			writeJsonValue(writer, array[i]);
		}
	} 
	else if (value->is<RuntimeObjectValue>())
	{
		writer.StartObject();

		SCOPE_Success{
			writer.EndObject();
		};

		const RuntimeObjectValue& obj = value->as<const RuntimeObjectValue&>();
		for (size_t i = 0, sz = obj.size(); i < sz; ++i)
		{
			auto field = obj.field(i);

			//auto name = strings::wstringFromUtf8(field.name());
			auto name = field.name();

			writer.String(name.data(), name.size());
			writeJsonValue(writer, field.value());
		}
	}
}


std::string serializeToJson(const RuntimeValue::Ptr value)
{
	using namespace rapidjson;

	StrBuffer buffer;
	JsonStrWriter writer(buffer);

	writeJsonValue(writer, value);

	return std::string(buffer.GetString(), buffer.GetSize());
}


TEST(TestRuntimeValue, Test1) {

	const int value = 77;
	const auto val = runtimeValue(value);
	*val;

	//processVal(val);

	const std::vector<int> ints = {1,2,3,4};
	//ints.push_back(1);

	auto val2 = runtimeValue(ints);
	val2->size();
	
	std::cout << val2->element(2)->as<RuntimeInt32Value&>().get() << std::endl;


	FooObject1 obj;
	obj.fieldArr.push_back(10);
	obj.fieldArr.push_back(22);
	obj.fieldArr.push_back(333);

	auto val3 = runtimeValue(obj);

	const auto json = serializeToJson(val3);

	std::cout << json << std::endl;

	Uid uid = Uid::generate();
	Uid uid2 = Uid::generate();
	auto uval = runtimeValue(uid);

	std::string str = *(*uval);
	uval->set(toString(uid2)).ignore();

	static_cast<bool>(uval->set("abra-cadabra"));


	for (size_t c = 0; c < val3->size(); ++c) 
	{
		auto field = val3->field(c);

		if (field.value()->is<RuntimePrimitiveValue>()) 
		{
	 		auto x = field.value()->as<RuntimeInt32Value&>().set(25);
		}


		std::cout << field.name() << std::endl;
	}
}


TEST(TestRuntimeValue, Test2) {

	 int val1 = 100;

	 std::optional<float> val2;
	 float temp = 222.f;
	 val2.emplace(temp);

	 auto rt1 = runtimeValue(val1);
	 auto rt2 = runtimeValue(val2);
	
	 if (auto res = RuntimeValue::assign(rt2, rt1); !res)
	 {
	 	try {
	 		res.rethrowIfException();
	 	}
	 	catch (const std::exception& excpt) {
	 		std::cout << "Fail:" << excpt.what() << std::endl;
	 	}
	 }
	 else {
	 	std::cout << "All is ok\n";
	 }

	/*std::vector<int> ints = {1,2,3,4,5,6};
	const std::list<float> floats = {7.f, 8.f, 9.f};

	auto rtInts = runtimeValue(ints);
	const auto rtFloats = runtimeValue(floats);

	RuntimeValue::assign(rtInts, rtFloats).ignore();*/
	

}
