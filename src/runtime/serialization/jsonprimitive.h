#pragma once
#include "cold/serialization/runtimevalue.h"
#include <rapidjson/writer.h>


namespace cold::serialization::json {

template<typename Stream, typename SrcEnc, typename TargetEnc>
void writeJsonPrimitiveValue(rapidjson::Writer<Stream, SrcEnc, TargetEnc>& writer, const RuntimeValue::Ptr& value)
{
	if (value->is<const RuntimeInt32Value>())
	{
		writer.Int(value->as<const RuntimeInt32Value&>().get());
	}
	else if (value->is<const RuntimeUInt32Value>())
	{
		writer.Uint(value->as<const RuntimeUInt32Value&>().get());
	}
}

}
