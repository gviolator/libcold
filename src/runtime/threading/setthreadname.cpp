#include "pch.h"
#include "cold/threading/setthreadname.h"

namespace cold::threading {

namespace {

#pragma pack(push,8)
struct THREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
};
#pragma pack(pop)

} // namespace

void setCurrentThreadName(const char* name) noexcept
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID =  GetCurrentThreadId();
	info.dwFlags = 0;

	__try
	{
		const DWORD MS_VC_EXCEPTION=0x406D1388;
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

}
