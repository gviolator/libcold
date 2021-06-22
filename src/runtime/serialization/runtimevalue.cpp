#include "pch.h"
#include "cold/serialization/runtimevalue.h"
#include "cold/diagnostics/exception.h"
#include "cold/diagnostics/runtimecheck.h"
#include "cold/utils/stringconv.h"
#include "cold/com/comclass.h"


namespace cold {

namespace {
/*
template<typename U, typename T>
decltype(auto) cast_(T& src_) 
{
	using Target = std::conditional_t<std::is_const_v<T>, std::add_const_t<U>, U>;
	return src_.as<Target&>();
}


template<typename RT, typename F>
Result<> visitNumericPrimitiveValue(RT& value, F f) 
{
	if (value->is<RuntimeInt8Value>())
	{
		return f(cast_<RuntimeInt8Value>(*value));
	}
	else if (value->is<RuntimeInt16Value>())
	{
		return f(cast_<RuntimeInt16Value>(*value));
	}
	else if (value->is<RuntimeInt32Value>())
	{
		return f(cast_<RuntimeInt32Value>(*value));
	}
	else if (value->is<RuntimeInt64Value>())
	{
		return f(cast_<RuntimeInt64Value>(*value));
	}
	else if (value->is<RuntimeUInt8Value>())
	{
		return f(cast_<RuntimeUInt8Value>(*value));
	}
	else if (value->is<RuntimeUInt16Value>())
	{
		return f(cast_<RuntimeUInt16Value>(*value));
	}
	else if (value->is<RuntimeUInt32Value>())
	{
		return f(cast_<RuntimeUInt32Value>(*value));
	}
	else if (value->is<RuntimeUInt64Value>())
	{
		return f(cast_<RuntimeUInt64Value>(*value));
	}
	else if (value->is<RuntimeSingleValue>())
	{
		return f(cast_<RuntimeSingleValue>(*value));
	}
	else if (value->is<RuntimeDoubleValue>())
	{
		return f(cast_<RuntimeDoubleValue>(*value));
	}
	
	RUNTIME_FAILURE("Unexpected code path")
	return Excpt_("Unexpected code path");
}
*/

Result<> assignPrimitiveValue(RuntimePrimitiveValue& dst, const RuntimePrimitiveValue& src)
{
	if (RuntimeBooleanValue* const dstBool = dst.as<RuntimeBooleanValue*>(); dstBool)
	{
		if (auto srcBool = src.as<const RuntimeBooleanValue*>(); srcBool)
		{
			dstBool->set(srcBool->get());
			return success;
		}

		return Excpt_("Expected Boolean");
	}

	if (src.is<RuntimeBooleanValue>())
	{
		return Excpt_("Boolean can be assigned only to boolean");
	}

	if (RuntimeIntegerValue* const dstInt = dst.as<RuntimeIntegerValue*>(); dstInt)
	{
		if (const RuntimeIntegerValue* const srcInt = src.as<const RuntimeIntegerValue*>(); srcInt)
		{
			if (dstInt->isSigned())
			{
				dstInt->setInt64(srcInt->getInt64());
			}
			else
			{
				dstInt->setUint64(srcInt->getUint64());
			}
		}
		else if (const RuntimeFloatValue* const srcFloat = src.as<const RuntimeFloatValue*>(); srcFloat)
		{
			const auto iValue = static_cast<int64_t>(std::floor(srcFloat->getSingle()));

			if (dstInt->isSigned())
			{
				dstInt->setInt64(iValue);
			}
			else
			{
				dstInt->setUint64(iValue);
			}
		}
		else
		{
			return Excpt_("Can t assgin value to integer");
		}

		return success;
	}

	if (RuntimeFloatValue* const dstFloat = dst.as<RuntimeFloatValue*>(); dstFloat)
	{
		if (const RuntimeFloatValue* const srcFloat = src.as<const RuntimeFloatValue*>(); srcFloat)
		{
			dstFloat->setDouble(srcFloat->getDouble());
		}
		else if (const RuntimeIntegerValue* const srcInt = src.as<const RuntimeIntegerValue*>(); srcInt)
		{
			if (dstFloat->bits() == sizeof(double))
			{
				dstFloat->setDouble(static_cast<double>(srcInt->getInt64()));
			}
			else
			{
				dstFloat->setSingle(static_cast<float>(srcInt->getInt64()));
			}
		}
		else
		{
			return Excpt_("Can t assgin value to float");
		}

		return success;
	}



	//const auto isString = [](const RuntimePrimitiveValue& val) 
	//{
	//	return val.is<RuntimeStringValue>() || val.is<RuntimeWStringValue>();
	//};
	/*
	if (isString(*dst) || isString(*srcPtr))
	{
		if (!isString(*dst) || !isString(*srcPtr))
		{
			return Excpt_("String is assignable only to string");
		}

		if (dst->is<RuntimeStringValue>()) 
		{
			RuntimeStringValue& dstStr = dst->as<RuntimeStringValue&>();

			if (srcPtr->is<RuntimeStringValue>())
			{
				const RuntimeStringValue& srcStr = srcPtr->as<const RuntimeStringValue&>();
				return dstStr.set(srcStr.get());
			}
			
			const RuntimeWStringValue& srcWStr = srcPtr->as<const RuntimeWStringValue&>();
			std::string bytes = strings::toUtf8(srcWStr.get());
			return dstStr.set(std::move(bytes));
		}

		RuntimeWStringValue& dstWStr = dst->as<RuntimeWStringValue&>();
		if (srcRtv.is<RuntimeStringValue>()) {
			const RuntimeStringValue& srcStr = srcPtr->as<const RuntimeStringValue&>();
			std::wstring wstr = strings::wstringFromUtf8(srcStr.get());
			return dstWStr.set(std::move(wstr));
		}

		const RuntimeWStringValue& srcWStr = srcPtr->as<const RuntimeWStringValue&>();
		return dstWStr.set(srcWStr.get());
	}*/

	//return visitNumericPrimitiveValue(dst, [&]<typename T>(TypedRuntimePrimitiveValue<T>& dstRtValue) -> Result<> {
	//	return visitNumericPrimitiveValue(srcPtr, [&]<typename U>(const TypedRuntimePrimitiveValue<U>& srcRtValue) -> Result<>{
	//		const auto srcValue = *srcRtValue;
	//		return dstRtValue.set(srcValue);
	//	});
	//});
	return success;
}

Result<> assignArray(RuntimeArray& arr, const RuntimeReadonlyCollection& collection) 
{
	const size_t size = collection.size();
	arr.clear();
	if (size == 0)
	{
		return success;
	}

	arr.reserve(size);

	for (size_t i = 0; i < size; ++i)
	{
		if (auto pushResult = arr.push(collection[i]); !pushResult)
		{
			return pushResult;
		}
	}
	
	return success;
}


Result<> assignDictionary(RuntimeDictionary& dict, const RuntimeReadonlyCollection& src)
{
	return Excpt_("assignDictionary not implemented");
}


Result<> assignObject(RuntimeObject& obj, const RuntimeReadonlyDictionary& dict)
{
	for (size_t i = 0, size = obj.size(); i < size; ++i)
	{
		const auto [key, fieldValue] = obj[i];

		if (const RuntimeValue::Ptr value = dict[key]; value)
		{
			if (auto assignResult = RuntimeValue::assign(fieldValue, value); !assignResult)
			{
				return assignResult;
			}
		}
		else
		{

		}
	}

	return success;
}

} // namespace


Result<> RuntimeValue::assign(RuntimeValue::Ptr dst, RuntimeValue::Ptr src)
{
	DEBUG_CHECK(dst)
	DEBUG_CHECK(src)

	DEBUG_CHECK(dst->isMutable())

	if (src->is<RuntimeOptionalValue>()) 
	{
		auto& srcOpt = src->as<const RuntimeOptionalValue&>();
		if (srcOpt.hasValue()) 
		{
			//return RuntimeValue::assign(dst, srcOpt.value());
		}
		
		if (!dst->is<RuntimeOptionalValue>()) 
		{
			return Excpt_("Optional is null");
		}
		else 
		{
			auto& dstOpt = dst->as<RuntimeOptionalValue&>();
			return dstOpt.setValue();
		}
	}

	if (dst->is<RuntimeOptionalValue>()) 
	{
		auto& dstOpt = dst->as<RuntimeOptionalValue&>();
		// Can this code be refactored to takes into account, that next line can be failed ?!
		//return RuntimeValue::assign(dstOpt.emplace(), src);
	}

	if (RuntimePrimitiveValue* const dstValue = dst->as<RuntimePrimitiveValue*>(); dstValue)
	{
		auto srcValue = src->as<const RuntimePrimitiveValue*>();
		return assignPrimitiveValue(*dstValue, *srcValue);
	}
	
	if (RuntimeArray* const arr = dst->as<RuntimeArray*>(); arr)
	{
		const RuntimeReadonlyCollection* const collection = src->as<const RuntimeReadonlyCollection*>();
		if (!collection)
		{
			return Excpt_("Collection required");
		}

		return assignArray(*arr, *collection);
	}

	if (RuntimeDictionary* const dict = dst->as<RuntimeDictionary*>(); dict)
	{
		
	}

	if (RuntimeObject* const obj = dst->as<RuntimeObject*>(); obj)
	{
		const RuntimeReadonlyDictionary* const dict = src->as<const RuntimeReadonlyDictionary*>();
		if (!dict)
		{
			return Excpt_("Collection required");
		}

		return assignObject(*obj, *dict);
	}

	return Excpt_("Do not known how to assign runtime value");
}

}
