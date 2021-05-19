#include "pch.h"
#include "cold/serialization/runtimevalue.h"
#include "cold/diagnostics/exception.h"
#include "cold/diagnostics/runtimecheck.h"
#include "cold/utils/stringconv.h"
#include "cold/com/comclass.h"


namespace cold {

namespace {

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


Result<> assignPrimitiveValue(ComPtr<RuntimePrimitiveValue> dst, const RuntimeValue::Ptr src)
{
	const RuntimePrimitiveValue* srcPtr = src->as<const RuntimePrimitiveValue*>();
	if (!srcPtr)
	{
		return Excpt_("Value category mismatch.");
	}

	const RuntimePrimitiveValue& srcRtv = *srcPtr;

	if (dst->is<RuntimeBooleanValue>() || srcPtr->is<RuntimeBooleanValue>())
	{
		if (!dst->is<RuntimeBooleanValue>() || !srcPtr->is<RuntimeBooleanValue>())
		{
			return Excpt_("Boolean is assignable only to boolean");
		}

		return success;
	}

	const auto isString = [](const RuntimePrimitiveValue& val) 
	{
		return val.is<RuntimeStringValue>() || val.is<RuntimeWStringValue>();
	};

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
	}

	return visitNumericPrimitiveValue(dst, [&]<typename T>(TypedRuntimePrimitiveValue<T>& dstRtValue) -> Result<> {
		return visitNumericPrimitiveValue(srcPtr, [&]<typename U>(const TypedRuntimePrimitiveValue<U>& srcRtValue) -> Result<>{
			const auto srcValue = *srcRtValue;
			return dstRtValue.set(srcValue);
		});
	});
}

Result<> assignArrayValue(ComPtr<RuntimeArrayValue> dstArray, const ComPtr<RuntimeValue> srcValue) 
{
	if (!srcValue->is<RuntimeArrayValue>()) 
	{
		return Excpt_("Expected array value");
	}

	const auto& srcCollection = srcValue->as<const RuntimeReadonlyCollection&>();
	const size_t srcSize = srcCollection.size();
	dstArray->clear(srcSize);

	for (size_t i = 0; i < srcSize; ++i) {
		dstArray->pushBack();
		RuntimeValue::Ptr dstElement = dstArray->back();
		//RuntimeValue& dstElement = dstArray.emplaceBack();
		const RuntimeValue::Ptr srcElement = srcCollection[i];

		if (auto res = RuntimeValue::assign(dstElement, srcElement); !res) {
			return res;
		}
	}
	
	return success;
}


} // namespace


Result<> RuntimeValue::assign(RuntimeValue::Ptr dst, const RuntimeValue::Ptr src)
{
	DEBUG_CHECK(dst->isMutable())

	if (src->is<RuntimeOptionalValue>()) 
	{
		auto& srcOpt = src->as<RuntimeOptionalValue&>();

		if (srcOpt.hasValue()) 
		{
			return RuntimeValue::assign(dst, srcOpt.value());
		}
		
		if (!dst->is<RuntimeOptionalValue>()) 
		{
			return Excpt_("Optional is null");
		}
		else 
		{
			auto& dstOpt = dst->as<RuntimeOptionalValue&>();
			dstOpt.reset();
			return success;
		}
	}

	if (dst->is<RuntimeOptionalValue>()) 
	{
		auto& dstOpt = dst->as<RuntimeOptionalValue&>();
		// Can this code be refactored to takes into account, that next line can be failed ?!
		return RuntimeValue::assign(dstOpt.emplace(), src);
	}

	if (src->is<RuntimePrimitiveValue>()) 
	{
		//ComPtr<RuntimePrimitiveValue> value = cold::com::createInstance<RuntimePrimitive>(src);
		return assignPrimitiveValue(dst, src);
	}
	
	if (dst->is<RuntimeArrayValue>()) 
	{
		return assignArrayValue(dst, src);
	}

	return success;
}

}
