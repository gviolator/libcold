#ifndef _WIN32
#error "Windows specific source"
#endif

#include "pch.h"
#include "cold/utils/uid.h"

#include <rpc.h> 
#include <vcruntime_string.h>

namespace cold {

namespace {

static_assert(sizeof(Uid) == sizeof(GUID), "Uid size does not match to UUID/GUID size");

template<typename Char>
struct RpcString
{
	using Str = std::conditional_t<std::is_same_v<Char, char>, RPC_CSTR, RPC_WSTR>;

	Str str = nullptr;

	~RpcString()
	{
		if (str)
		{
			freeString(str);
		}
	}

	operator Str* ()
	{
		return &str;
	}

	std::basic_string<Char> stdString() const
	{
		return str == nullptr ?
			std::basic_string<Char>{} :
			std::basic_string<Char>{reinterpret_cast<const Char*>(str)};
	}

	static void freeString(RPC_CSTR& str)
	{
		RpcStringFreeA(&str);
	}

	static void freeString(RPC_WSTR& str)
	{
		RpcStringFreeW(&str);
	}
};

GUID nullGuid()
{
	struct NullGuid : GUID
	{
		NullGuid() {
			memset(reinterpret_cast<void*>(this), 0, sizeof(GUID));
		}
	};

	static NullGuid value;
	return value;
}

} // namespace

//-----------------------------------------------------------------------------

Uid::Uid() noexcept : data__(nullGuid())
{}

Uid::operator bool () const noexcept
{
	return data__ != nullGuid();
}

Uid Uid::generate()
{
	Uid uid;
	UuidCreate(&uid.data__);
	return uid;
}

std::optional<Uid> Uid::parse(std::string_view strView)
{
	RPC_CSTR str = reinterpret_cast<RPC_CSTR>(const_cast<char*>(strView.data()));

	Uid uid;
	return UuidFromStringA(str, &uid.data__) == S_OK ? std::optional{uid} : std::nullopt;
}

std::optional<Uid> Uid::parse(std::wstring_view strView)
{
	RPC_WSTR str = reinterpret_cast<RPC_WSTR>(const_cast<wchar_t*>(strView.data()));

	Uid uid;
	return UuidFromStringW(str, &uid.data__) == S_OK ? std::optional{uid} : std::nullopt;
}

std::string Uid::toString__(Uid uid) noexcept
{
	RpcString<char> str;

	UuidToStringA(&uid.data__, str);
	return str.stdString();
}

std::wstring Uid::toWString__(Uid uid) noexcept
{
	RpcString<wchar_t> str;

	UuidToStringW(&uid.data__, str);
	return str.stdString();
}

bool operator < (Uid uid1, Uid uid2) noexcept
{
	const auto cmpRes = memcmp(reinterpret_cast<const void*>(&uid1.data__), reinterpret_cast<const void*>(&uid1.data__), sizeof(GUID));
	return cmpRes < 0;
}

bool operator == (Uid uid1, Uid uid2) noexcept
{
	return uid1.data__ == uid2.data__;
}

bool operator != (Uid uid1, Uid uid2) noexcept
{
	return !(uid1.data__ == uid2.data__);
}

}
