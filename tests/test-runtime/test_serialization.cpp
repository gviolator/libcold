#include "pch.h"
#include <cold/meta/classinfo.h>

#include <cold/io/writer.h>
#include <cold/io/reader.h>
#include <cold/memory/bytesbuffer.h>

#include <cold/serialization/jsonserialization.h>
#include <cold/serialization/msgpackserialization.h>
#include <cold/serialization/runtimevaluebuilder.h>
#include <cold/serialization/serialization.h>
#include <cold/com/comclass.h>


using namespace cold;
using namespace cold::serialization;
using namespace testing;

#if 0

class MyStringWriter final : public io::Writer
{
	DECLARE_CLASS_BASE(io::Writer)

	IMPLEMENT_ANYTHING

public:

	MyStringWriter(std::string& str_): m_str(str_)
	{}

	void write(const std::byte* ptr, size_t size) override {
		m_str.append(reinterpret_cast<const char*>(ptr), size);
	}


	virtual size_t offset() const override {
		return m_str.size();
	}

private:
	std::string& m_str;
};


class MyBufferWriter final : public io::Writer 
{
	DECLARE_CLASS_BASE(io::Writer)
	IMPLEMENT_ANYTHING

public:

	MyBufferWriter(BytesBuffer& buffer_): m_buffer(buffer_), m_initialSize(buffer_.size())
	{}

	void write(const std::byte* ptr, size_t size) override {
		memcpy(m_buffer.append(size), ptr, size);
	}

	size_t offset() const override { 
		return m_buffer.size() - m_initialSize;
	}

private:
	BytesBuffer& m_buffer;
	const size_t m_initialSize;
};


class MyStringReader : public io::Reader
{
	using Ch = char;

	DECLARE_CLASS_BASE(io::Reader)
	IMPLEMENT_ANYTHING

public:

	MyStringReader(std::string_view str) : m_str{str}
	{}

	size_t read(std::byte* buffer, size_t readCount) override {
		DEBUG_CHECK((readCount % sizeof(Ch)) == 0, "Invalid read bytes cout: (%1), because size of char:(%2)", readCount, sizeof(Ch))

		const size_t bytesAvailable = (m_str.size() * sizeof(Ch)) - m_offset;
		const auto actualRead = std::min(bytesAvailable, readCount);

		if (actualRead == 0) {
			return 0;
		}

		if (readCount == sizeof(Ch)) {
			*reinterpret_cast<Ch*>(buffer) = m_str[m_offset];
		}
		else
		{
			DEBUG_CHECK((m_offset % sizeof(Ch)) == 0)

			const size_t charsOffset = m_offset / sizeof(Ch);

			memcpy(buffer, m_str.data() + charsOffset, actualRead);
		}

		m_offset += actualRead;

		return actualRead;
	}


private:
	std::string_view m_str;
	size_t m_offset = 0;
};

class Test_Serialization : public testing::Test
{

};


TEST_F(Test_Serialization, Test1) {

	auto& ser = jsonSerialization();
	//auto& ser = msgpackSerialization();

	//std::string buffer;
	//MyStringWriter writer(buffer);
	BytesBuffer buffer;
	MyBufferWriter writer(buffer);

	std::vector<int> ints {10,33,556};

	//ser.jsonSerialize(writer, runtimeValue(60), {}).ignore();
	//ser.serialize(writer, runtimeValue(ints)).ignore();
	ser.serialize(writer, *cold::com::createInstance<RuntimeValue>(ints)).ignore();

	MyStringReader reader(asStringView(buffer));

	// RuntimeValue rt = runtimeValue(ints);

	std::cout << asStringView(buffer) << std::endl;

	auto result = ser.deserialize(reader);

	std::list<float> floats;

	auto rt = cold::com::createInstance<RuntimeValue>(floats);

	RuntimeValue::assign(*rt, **result).ignore();
	for (auto f : floats) {
		std::cout << f << std::endl;
	}



	//ser.as<const ISerialization&>().

}

#endif