#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/utils/result.h>
#include <cold/serialization/runtimevalue.h>
#include <cold/serialization/serialization.h>


namespace cold::serialization {

/**
*/
struct JsonSettings
{
	bool pretty = false;
};

RUNTIME_EXPORT Result<> jsonWrite(io::Writer&, const RuntimeValue::Ptr&, JsonSettings);

RUNTIME_EXPORT Result<RuntimeValue::Ptr> jsonParse(io::Reader&);

// RUNTIME_EXPORT Result<> jsonDeserialize(io::Reader&, RuntimeValue::Ptr);


/**
*/
//struct INTERFACE_API IJsonSerialization : ISerialization
//{
//	DECLARE_CLASS_BASE(ISerialization)
//
//	virtual Result<> jsonSerialize(io::Writer& writer, const RuntimeValue::Ptr, const JsonSettings&) const = 0;
//};

/**
*/
// RUNTIME_EXPORT const IJsonSerialization& jsonSerialization();

}
