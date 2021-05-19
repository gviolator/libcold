#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/serialization/serialization.h>


namespace cold::serialization {

/**
*/
struct JsonSettings
{
	bool pretty = false;
};

/**
*/
struct INTERFACE_API IJsonSerialization : ISerialization
{
	DECLARE_CLASS_BASE(ISerialization)

	virtual Result<> jsonSerialize(io::Writer& writer, const RuntimeValue::Ptr, const JsonSettings&) const = 0;
};

/**
*/
RUNTIME_EXPORT const IJsonSerialization& jsonSerialization();

}
