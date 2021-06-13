#include "pch.h"

#include "jsonprimitive.h"
// #include "cold/serialization/json.h"
#include "cold/serialization/jsonserialization.h"
#include "cold/com/comclass.h"
#include "cold/utils/scopeguard.h"
#include "cold/utils/intrusivelist.h"

#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <map>

namespace cold::serialization {

namespace {

//
using DefaultEncoding = rapidjson::UTF8<char>;
using JsonDocument = rapidjson::GenericDocument<DefaultEncoding>;
using JsonValue = rapidjson::GenericValue<DefaultEncoding>;



/**

*/
//template<typename Encoding = rapidjson::UTF8<>>
class ReaderStream
{
public:
	using Ch = typename DefaultEncoding::Ch;

	static constexpr Ch NoChar = 0;

	ReaderStream(io::Reader& reader_): m_reader(reader_)
	{
		this->fillBuffer();
	}

	Ch Peek() const
	{
		return (m_inBufferPos == m_avail) ? NoChar : m_buffer[m_inBufferPos];
	}

	Ch Take()
	{
		if (m_inBufferPos == m_avail)
		{
			return NoChar;
		}

		++m_offset;
		const Ch ch = m_buffer[m_inBufferPos];
		if (++m_inBufferPos == m_avail)
		{
			this->fillBuffer();
		}

		return ch;
	}

	size_t Tell() const
	{
		return m_offset;
	}

#pragma region Methods that must be never called, but required for compilation

	Ch* PutBegin()
	{
		RUNTIME_FAILURE("PutBegin must not be caller for json::ReaderStream")
		return nullptr;
	}

	void Put(Ch)
	{
		RUNTIME_FAILURE("Put must not be caller for json::ReaderStream")
	}

	void Flush()
	{
		RUNTIME_FAILURE("Flush must not be caller for json::ReaderStream")
	}

	size_t PutEnd(Ch*)
	{
		RUNTIME_FAILURE("PutEnd must not be caller for json::ReaderStream")
		return 0;
	}

#pragma endregion

private:

	static constexpr size_t PrefetchBufferSize = 64;
	using Buffer = std::array<Ch, PrefetchBufferSize>;

	void fillBuffer()
	{
		DEBUG_CHECK(m_inBufferPos == m_avail)

		std::byte* const ptr = reinterpret_cast<std::byte*>(m_buffer.data());
		const size_t readSize = m_buffer.size() * sizeof(Ch);
		m_avail = m_reader.read(ptr, readSize);
		DEBUG_CHECK(m_avail % sizeof(Ch) == 0)
		m_avail = m_avail / sizeof(Ch);
		m_inBufferPos = 0;
	}

	io::Reader& m_reader;
	Buffer m_buffer;
	size_t m_offset = 0;
	size_t m_avail = 0;
	size_t m_inBufferPos = 0;
};


Result<> parse(JsonDocument& document, ReaderStream& stream) {
	
	constexpr unsigned ParseFlags = rapidjson::kParseCommentsFlag | rapidjson::kParseStopWhenDoneFlag;

	document.ParseStream<ParseFlags>(stream);

	const auto error = document.GetParseError();

	switch (error)
	{
	case rapidjson::kParseErrorNone:
	{
		break;
	}

	case rapidjson::kParseErrorDocumentEmpty:
	{
		return Excpt(EndOfStreamException);
	}

	default:
	{
		return MAKE_Excpt(SerializationException, strfmt(L"Json error:(%1)", error));
	}
	}

	return success;
}


class ValueOrDocument
{
public:
	ValueOrDocument(JsonDocument&& doc): m_valueOrDocument(std::in_place_type<JsonDocument>, std::move(doc))
	{}

	ValueOrDocument(JsonValue&& val): m_valueOrDocument(std::in_place_type<JsonValue>, std::move(val))
	{}

	ValueOrDocument(ValueOrDocument&& other) noexcept: m_valueOrDocument(std::move(other.m_valueOrDocument))// m_document(std::move(other.m_document)), m_value(std::move(other.m_value))
	{}

	JsonValue* operator-> ()
	{
		DEBUG_CHECK(std::holds_alternative<JsonValue>(m_valueOrDocument) || std::holds_alternative<JsonDocument>(m_valueOrDocument))

		JsonValue& valueRef = std::holds_alternative<JsonValue>(m_valueOrDocument) ? std::get<JsonValue>(m_valueOrDocument) : static_cast<JsonValue&>(std::get<JsonDocument>(m_valueOrDocument));
		return &valueRef;
	}

	const JsonValue* operator-> () const
	{
		DEBUG_CHECK(std::holds_alternative<JsonValue>(m_valueOrDocument) || std::holds_alternative<JsonDocument>(m_valueOrDocument))

		const JsonValue& valueRef = std::holds_alternative<JsonValue>(m_valueOrDocument) ? std::get<JsonValue>(m_valueOrDocument) : static_cast<const JsonValue&>(std::get<JsonDocument>(m_valueOrDocument));
		return &valueRef;
	}

	JsonValue& operator* ()
	{
		DEBUG_CHECK(std::holds_alternative<JsonValue>(m_valueOrDocument) || std::holds_alternative<JsonDocument>(m_valueOrDocument))

		return std::holds_alternative<JsonValue>(m_valueOrDocument) ? std::get<JsonValue>(m_valueOrDocument) : static_cast<JsonValue&>(std::get<JsonDocument>(m_valueOrDocument));
	}

	const JsonValue& operator* () const
	{
		DEBUG_CHECK(std::holds_alternative<JsonValue>(m_valueOrDocument) || std::holds_alternative<JsonDocument>(m_valueOrDocument))

		return std::holds_alternative<JsonValue>(m_valueOrDocument) ? std::get<JsonValue>(m_valueOrDocument) : static_cast<const JsonValue&>(std::get<JsonDocument>(m_valueOrDocument));
	}

private:
	std::variant<JsonValue, JsonDocument> m_valueOrDocument;
};


RuntimeValue::Ptr createRuntimeValueFromJson(ValueOrDocument value);


class JsonRuntimeValueBase : public virtual RuntimePrimitiveValue
{
	DECLARE_CLASS_BASE(RuntimePrimitiveValue)

public:

	JsonRuntimeValueBase(ValueOrDocument jsonValue): m_jsonValue(std::move(jsonValue))
	{}

	bool isMutable() const override
	{
		return false;
	}

	//bool hasValue() const override
	//{
	//	return m_jsonValue->GetType() != rapidjson::kNullType;
	//}

	//RuntimeValue::Ptr value() const
	//{
	//	if (!this->hasValue())
	//	{
	//		return nothing;
	//	}

	//	return com::Acquire{const_cast<JsonRuntimeValueBase&>(*this).as<RuntimeValue*>()};
	//}

	//void reset() override
	//{

	//}

	//RuntimeValue::Ptr emplace(RuntimeValue::Ptr value) override
	//{
	//	return nothing;
	//}

	JsonValue& jsonValue()
	{
		return *m_jsonValue;
	}

	const JsonValue& jsonValue() const
	{
		return *m_jsonValue;
	}

private:

	ValueOrDocument m_jsonValue;
};


class JsonRuntimeIntegerValue final : public virtual JsonRuntimeValueBase, public virtual RuntimeIntegerValue
{
	COMCLASS_(JsonRuntimeValueBase, RuntimeIntegerValue)

public:

	using JsonRuntimeValueBase::JsonRuntimeValueBase;

	bool isSigned() const override
	{
		return this->jsonValue().IsInt() || this->jsonValue().IsInt64();
	}

	size_t bits() const
	{
		return this->jsonValue().IsInt64() || this->jsonValue().IsUint64() ? sizeof(int64_t) : sizeof(int32_t);
	}


	void setInt64(int64_t) override
	{
	}

	void setUint64(uint64_t) override
	{
	}

	int64_t getInt64() const override
	{
		return this->getWithRangeCheck<int64_t>();
	}

	uint64_t getUint64() const override
	{
		return this->getWithRangeCheck<uint64_t>();
	}

private:

	template<typename T>
	T getWithRangeCheck() const requires(std::is_integral_v<T>)
	{
		decltype(auto) value = this->jsonValue();

		if (value.IsInt())
		{
			return castWithRangeCheck<T>(value.GetInt());
		}
		else if (value.IsUint())
		{
			return castWithRangeCheck<T>(value.GetUint());
		}
		else if (value.IsInt64())
		{
			return castWithRangeCheck<T>(value.GetInt64());
		}

		DEBUG_CHECK(value.IsUint64())

		return castWithRangeCheck<T>(value.GetUint64());
	}
	
	template<typename T, typename U>
	inline static T castWithRangeCheck(U value)
	{
		if constexpr (sizeof(T) < sizeof(U))
		{
			// if (static_cast<std::numeric_limits<T>::max() < value
		}

		return static_cast<T>(value);
	}
};


class JsonRuntimeFloatValue : public virtual JsonRuntimeValueBase, public RuntimeFloatValue
{
	COMCLASS_(JsonRuntimeValueBase, RuntimeFloatValue)

public:

	using JsonRuntimeValueBase::JsonRuntimeValueBase;

	size_t bits() const override
	{
		return this->jsonValue().IsDouble() ? sizeof(double) : sizeof(float);
	}

	void setDouble(double) override
	{
	}

	void setSingle(float) override
	{

	}

	double getDouble() const override
	{
		if (this->jsonValue().IsDouble())
		{
			return this->jsonValue().GetDouble();
		}

		DEBUG_CHECK(this->jsonValue().IsFloat())

		return static_cast<double>(this->jsonValue().GetFloat());
	}
	
	float getSingle() const override
	{
		return this->jsonValue().GetFloat();
	}
};

/*

class JsonRuntimeNumberValue final : public JsonRuntimeValueBase, public RuntimeIntegerValue, public RuntimeFloatValue
{
	COMCLASS_(JsonRuntimeValueBase, RuntimeIntegerValue, RuntimeFloatValue)

public:

	using JsonRuntimeValueBase::JsonRuntimeValueBase;

	void setInt64(int64_t) override
	{
	}

	void setI32(int32_t) override
	{
	}

	void setI16(int16_t) override
	{
	}

	void setI8(int8_t) override
	{
	}

	void setUint64(uint64_t) override
	{
	}

	void setU32(uint32_t) override
	{
	}

	void setU16(uint16_t) override
	{
	}

	void setU8(uint8_t) override
	{
	}

	int64_t getInt64() const override
	{
		return jsonValue().GetInt64();
	}

	uint64_t getUint64() const override
	{
		return jsonValue().GetUint64();
	}

	void setDouble(double) override
	{
	}

	void setSingle(float) override
	{
	}

	double getDouble() const override
	{
		return jsonValue().GetDouble();
	}
	
	float getSingle() const override
	{
		return jsonValue().GetFloat();
	}
};
*/
class JsonRuntimeArray final : public JsonRuntimeValueBase, public virtual RuntimeReadonlyCollection, public virtual RuntimeTuple
{
	COMCLASS_(JsonRuntimeValueBase, RuntimeReadonlyCollection, RuntimeTuple)

public:

	JsonRuntimeArray(ValueOrDocument value)
		: JsonRuntimeValueBase(std::move(value))
		, m_jsonArray(arrayOfValue(this->jsonValue()))
		, m_size(static_cast<size_t>(this->m_jsonArray.Size()))
	{
		m_elements.resize(this->size());
	}

	size_t size() const override
	{
		return m_size;
	}

	RuntimeValue::Ptr element(size_t index) const override
	{
		if (this->size() <= index)
		{
			throw Excpt_("Index ({0}) out of bounds [0, {1}]", index, this->size());
		}

		if (!m_elements[index])
		{
			JsonValue& el = m_jsonArray[index];
			m_elements[index] = createRuntimeValueFromJson(std::move(el));
		}

		return m_elements[index];
	}

	//void clear() override
	//{
	//}

	//void reserve(size_t) override
	//{
	//}

	//void push(RuntimeValue::Ptr) override
	//{}

private:

	static inline JsonValue::Array arrayOfValue(JsonValue& value)
	{
		DEBUG_CHECK(value.IsArray())

		return value.GetArray();
	}


	JsonValue::Array m_jsonArray;
	const size_t m_size;
	mutable std::vector<RuntimeValue::Ptr> m_elements;
};


class JsonRuntimeDictionary final : public JsonRuntimeValueBase, public virtual RuntimeReadonlyDictionary
{
	COMCLASS_(JsonRuntimeValueBase, RuntimeReadonlyDictionary)

public:
	JsonRuntimeDictionary(ValueOrDocument value): JsonRuntimeValueBase(std::move(value))
	{
		DEBUG_CHECK(this->jsonValue().IsObject())

		JsonValue::Object obj = this->jsonValue().GetObj();
		
		for (auto& member : obj)
		{
			std::string_view memberName = member.name.GetString();
			auto value = createRuntimeValueFromJson(std::move(member.value));

			m_members.emplace(std::move(memberName), std::move(value));
			// member.value
		}
	}

	size_t size() const override
	{
		return m_members.size();
	}

	std::string_view key(size_t index) const override
	{
		DEBUG_CHECK(index < this->size(), "Invalid index ({0}) > size:({1})", index, m_members.size())

		auto iter = m_members.begin();
		std::advance(iter, index);

		return iter->first;
	}

	RuntimeValue::Ptr value(std::string_view key) const override
	{
		if (auto iter = m_members.find(key); iter != m_members.end())
		{
			return iter->second;
		}

		return nothing;
	}

	bool hasKey(std::string_view key) const override
	{
		return m_members.find(key) != m_members.end();
	}

private:

	std::map<std::string_view, ComPtr<JsonRuntimeValueBase>, std::less<>> m_members;

};

//-----------------------------------------------------------------------------
RuntimeValue::Ptr createRuntimeValueFromJson(ValueOrDocument value)
{
	if (value->GetType() == rapidjson::kNumberType)
	{
		if (value->IsFloat() || value->IsDouble())
		{
			return com::createInstance<JsonRuntimeFloatValue>(std::move(value));
		}

		DEBUG_CHECK(value->IsInt() || value->IsUint() || value->IsInt64() || value->IsUint64())

		return com::createInstance<JsonRuntimeIntegerValue, RuntimeValue>(std::move(value));
	}

	if (value->GetType() == rapidjson::kArrayType)
	{
		return com::createInstance<JsonRuntimeArray>(std::move(value));
	}

	if (value->GetType() == rapidjson::kObjectType)
	{
		return com::createInstance<JsonRuntimeDictionary>(std::move(value));
	}

	return {};

}

} // namespace



#if 0

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

const IJsonSerialization& jsonSerialization()
{
	static JsonSerialization serializer;
	return (serializer);
}

#endif


Result<> jsonWrite(io::Writer&, const RuntimeValue::Ptr&, JsonSettings)
{
	return success;
}

Result<RuntimeValue::Ptr> jsonParse(io::Reader& reader)
{
	JsonDocument document;
	ReaderStream stream{reader};

	if (auto result = parse(document, stream); !result)
	{
		return result.err();
	}

	return createRuntimeValueFromJson(std::move(document));

	// return RuntimeValue::Ptr{};
}

Result<> jsonDeserialize(io::Reader&, RuntimeValue::Ptr)
{
	return success;
}


}


