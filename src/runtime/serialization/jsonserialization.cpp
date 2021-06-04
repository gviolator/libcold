#include "pch.h"
#if 0
#include "jsonprimitive.h"
#include "cold/serialization/json.h"
#include "cold/serialization/jsonserialization.h"
#include "cold/com/comclass.h"
#include "cold/serialization/runtimevalue.h"
#include "cold/utils/scopeguard.h"

#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

namespace cold::serialization {

namespace json {

using DefaultEncoding = rapidjson::UTF8<char>;
using JsonDocument = rapidjson::GenericDocument<DefaultEncoding>;
using JsonValue = rapidjson::GenericValue<DefaultEncoding>;


/**
*/
template<typename C = char>
class WriterStream
{
public:
	using Ch = C;

	WriterStream(io::Writer& writer_) : m_writer(writer_)
	{}

	void Put(Ch c) {
		m_writer.write(reinterpret_cast<std::byte*>(&c), sizeof(c));
	}

	void Flush()
	{}

private:

	io::Writer& m_writer;

	friend void PutUnsafe(WriterStream<C>& stream, Ch c) {
		stream.m_writer.write(reinterpret_cast<std::byte*>(&c), sizeof(c));
	}
};

/**
*/ 
template<typename Stream, typename SrcEnc, typename TargetEnc>
Result<> writeJsonValue(rapidjson::Writer<Stream, SrcEnc, TargetEnc>& writer, const RuntimeValue::Ptr value) {
	
	if (value->is<RuntimePrimitiveValue>())
	{
		writeJsonPrimitiveValue(writer, value);
	}
	else if (value->is<RuntimeArrayValue>() || value->is<RuntimeTupleValue>())
	{
		writer.StartArray();

		SCOPE_Success{
			writer.EndArray();
		};

		const RuntimeReadonlyCollection& array = value->as<const RuntimeReadonlyCollection&>();

		for (size_t i = 0, sz = array.size(); i < sz; ++i)
		{
			if (auto result = writeJsonValue(writer, array[i]); !result)
			{
				return result;
			}
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
			auto name = field.name();
			writer.String(name.data(), name.size());

			if (auto result = writeJsonValue(writer, field.value()); !result)
			{
				return result;
			}
		}
	}

	return success;
}

class JsonValueHolder
{
public:
	JsonValueHolder(json::JsonDocument&& doc_): m_document(std::move(doc_))
	{}

	JsonValueHolder(json::JsonValue&& val_): m_value(std::move(val_))
	{}

	JsonValueHolder(JsonValueHolder&& other): m_document(std::move(other.m_document)), m_value(std::move(other.m_value))
	{
		DEBUG_CHECK(m_document || m_value)
	}

	json::JsonValue& jsonValue() {
		DEBUG_CHECK(m_document || m_value)

		if (m_document) {
			return *m_document;
		}

		return *m_value;
	}

	const json::JsonValue& jsonValue() const {
		DEBUG_CHECK(m_document || m_value)

		if (m_document) {
			return *m_document;
		}

		return *m_value;
	}	

	rapidjson::Type jsonType() const {
		return this->jsonValue().GetType();
	}

private:
	std::optional<json::JsonDocument> m_document;
	std::optional<json::JsonValue> m_value;
};


RuntimeValue::Ptr createJsonRuntimeValue(JsonValueHolder&&);



class JsonNumberValue final
	: public JsonValueHolder
	, public RuntimeDoubleValue
{
	COMCLASS_(RuntimeDoubleValue, JsonValueHolder)

public:
	//
	 JsonNumberValue(JsonValueHolder&& value_): JsonValueHolder(std::move(value_))
	 {}

	bool isMutable() const override {
		return true;
	}

	double get() const override {
		DEBUG_CHECK(jsonType() == rapidjson::Type::kNumberType)
		return jsonValue().GetDouble();
	}

	Result<> set(double value) override {
		jsonValue().SetDouble(value);
		return success;
	}
};

class JsonArrayValue final 
	: public JsonValueHolder
	, public RuntimeArrayValue
{
public:
	//using JsonValueHolder::JsonValueHolder;

	JsonArrayValue(JsonValueHolder&& value_): JsonValueHolder(std::move(value_))
	{
		initArrayElements();
	}

	bool isMutable() const override {
		return true;
	}

	size_t size() const override {
		return m_runtimeValues.size();
	}

	const RuntimeValue::Ptr element(size_t index) const override {
		return m_runtimeValues[index];
	}

	RuntimeValue::Ptr element(size_t index) override {
		return m_runtimeValues[index];
	}

	void clear(std::optional<size_t> reserve) override {
	}

	void pushBack(RuntimeValue::Ptr value) override {

	}

private:

	using OptionalRuntimeValue = std::optional<std::unique_ptr<RuntimeValue::Ptr>>;

	void initArrayElements() const
	{
		if (m_inited) {
			return;
		}
		m_inited = true;
		JsonValue::Array arr = const_cast<JsonArrayValue&>(*this).jsonValue().GetArray();

		for (size_t i = 0; i < arr.Size(); ++i) {
			json::JsonValue& el = arr[i];
			m_runtimeValues.emplace_back(createJsonRuntimeValue(std::move(el)));
		}
	}

	mutable bool m_inited = false;
	mutable std::vector<RuntimeValue::Ptr> m_runtimeValues;
};


//class Json

RuntimeValue::Ptr createJsonRuntimeValue(JsonValueHolder&& value) {

	if (value.jsonType() == rapidjson::kNumberType) {
		return cold::com::createInstance<JsonNumberValue>(std::move(value));
	}
	
	if (value.jsonType() == rapidjson::kArrayType) {
		return cold::com::createInstance<JsonNumberValue>(std::move(value));
	}

	return {};

}



} // namespace json


/**
*/
class JsonSerialization final : public IJsonSerialization
{
	CLASS_INFO(
		CLASS_BASE(IJsonSerialization)
	)

	IMPLEMENT_ANYTHING

private:

	Result<> serialize(io::Writer& writer, const RuntimeValue::Ptr value) const override {
		JsonSettings settings;
		settings.pretty = true;

		return this->jsonSerialize(writer, value, settings);
	}

	Result<RuntimeValue::Ptr> deserialize(io::Reader& reader) const override {

		json::JsonDocument document;
		json::ReaderStream stream{reader};

		if (auto result = json::parse(document, stream); !result) {
			return result.err();
		}

		return json::createJsonRuntimeValue(std::move(document));


//		return Excpt_("Failed");
	}

	Result<> deserializeInplace(io::Reader&, RuntimeValue::Ptr target) const override {
		return Excpt_("Failed");
	}

	Result<> jsonSerialize(io::Writer& writer, const RuntimeValue::Ptr value, const JsonSettings& settings) const override {
		json::WriterStream<> stream{writer};

		if (settings.pretty) {
			rapidjson::PrettyWriter<decltype(stream)> jsonWriter{stream};
			return json::writeJsonValue(jsonWriter, value);
		}
		
		rapidjson::Writer<decltype(stream)> jsonWriter{stream};
		return json::writeJsonValue(jsonWriter, value);
	}
};

//-----------------------------------------------------------------------------

const IJsonSerialization& jsonSerialization() {
	static JsonSerialization serializer;
	return (serializer);
}

}


#endif