#if 0
#include "pch.h"

#include "com/comclass.h"
#include "io/writer.h"
#include "serialization/msgpackserialization.h"
#include "serialization/runtimevalue.h"
#include "serialization/serialization.h"
#include "utils/scopeguard.h"


#include "msgpack.hpp"


namespace cold::serialization {
namespace mp {

class WriterStream
{
public:
	WriterStream(io::Writer &writer_): m_writer(writer_) {}

	void write(const char *buf, size_t len) {
		m_writer.write(reinterpret_cast<const std::byte *>(buf), len);
	}

private:
	io::Writer &m_writer;
};

using Packer = msgpack::packer<WriterStream>;

Result<> writeMsgpackValue(Packer &packer, const RuntimeValue &value) {

	switch (value.category()) {
	case RuntimeValueCategory::Primitive: {
		const auto type = static_cast<const RuntimePrimitiveValue&>(value).primitiveType();

		if (type == PrimitiveValueType::Int32) {
			packer.pack(*static_cast<const RuntimeInt32Value&>(value));
		}
		else if (type == PrimitiveValueType::UInt32) {
			packer.pack(*static_cast<const RuntimeUInt32Value&>(value));
		}
		break;
	}

	case RuntimeValueCategory::Array:
	case RuntimeValueCategory::Tuple:
	{
		const RuntimeReadonlyCollection& array = static_cast<const RuntimeReadonlyCollection&>(value);

		packer.pack_array(static_cast<uint32_t>(array.size()));

		for (size_t i = 0, sz = array.size(); i < sz; ++i) {
			if (auto result = writeMsgpackValue(packer, array[i]); !result) {
				return result;
			}
		}

		break;
	}

	case RuntimeValueCategory::Object: {

		const RuntimeObjectValue& obj = static_cast<const RuntimeObjectValue&>(value);

		packer.pack_map(static_cast<uint32_t>(obj.size()));

		for (size_t i = 0, sz = obj.size(); i < sz; ++i) {
			RuntimeObjectValue::ConstField field = obj.field(i);
			std::string_view name = field.name();
			packer.pack(name);
			if (auto result = writeMsgpackValue(packer, field.value()); !result) {
				return result;
			}
		}

		break;
	}

	}

	return success;
}

} // namespace mp


/**
*/
class MsgpackSerialization final : public ISerialization
{
	CLASS_INFO(
		CLASS_BASE(ISerialization)
	)

	IMPLEMENT_ANYTHING

private:
	Result<> serialize(io::Writer &writer, const RuntimeValue& value) const override {

		mp::WriterStream stream{writer};
		mp::Packer packer{stream};

		return mp::writeMsgpackValue(packer, value);
	}

	Result<std::unique_ptr<RuntimeValue>> deserialize(io::Reader&) const override {
		return Excpt(SerializationException, "Failed");
	}

	Result<> deserializeInplace(io::Reader&, RuntimeValue& target) const override {
		return Excpt_("Failed");
	}
};

//-----------------------------------------------------------------------------

const ISerialization& msgpackSerialization() {
	static MsgpackSerialization mpSerialization;
	return (mpSerialization);
}

} // namespace cold::serialization

#endif