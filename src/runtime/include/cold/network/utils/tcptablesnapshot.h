#pragma once
#include <cold/network/networkexport.h>
#include <cold/diagnostics/exceptionguard.h>

#include <type_traits>
#include <tuple>
#include <utility>

#ifdef _WIN32
#include <iphlpapi.h>
#include <winsock.h>
#endif


namespace cold::network {

#ifdef _WIN32
using TcpEntryHandle = MIB_TCPROW;
#endif

enum class TcpState
{
	Closed,
	Listen,
	Unknown

};

struct RUNTIME_EXPORT TcpTableEntry
{
	unsigned localAddr = 0;
	unsigned localPort = 0;
	unsigned remoteAddr = 0;
	unsigned remotePort = 0;
	TcpState state = TcpState::Unknown;

	TcpTableEntry();

	TcpTableEntry(const TcpEntryHandle& handle);
};




NETWORK_EXPORT void getTcpTableSnapshot__(void (* callback)(const TcpEntryHandle* entries, size_t count, void*) noexcept, void*);


template<typename Callable, typename ... Args>
requires (std::is_invocable_v<Callable, const TcpEntryHandle*, size_t, Args ...>)
auto getTcpTableSnapshot(Callable callable, Args ... args) -> std::invoke_result_t<Callable, const TcpEntryHandle*, size_t, Args ...>
{
	using Result = std::invoke_result_t<Callable, const TcpEntryHandle*, size_t, Args ...>;

	Result result{};
	auto data = std::tie(args..., result, callable);

	using DataTuple = decltype(data);


	static auto callback__ = []<size_t ... Indexes>(std::index_sequence<Indexes...>, const TcpEntryHandle* entries, size_t count, void* ptr) noexcept
	{
		constexpr size_t CallableIndex = (sizeof ... (Args)) + 1;
		constexpr size_t ResultIndex = (sizeof ... (Args));

		// DEBUG_NOEXCEPT_Guard {
			DataTuple& data = *reinterpret_cast<DataTuple*>(ptr);
			Callable& callable = std::get<CallableIndex>(data);
			std::get<ResultIndex>(data) = callable(entries, count, std::get<Indexes>(data) ...);
		// };
	};

	getTcpTableSnapshot__([](const TcpEntryHandle* entries, size_t count, void* ptr) noexcept
	{
		callback__(std::make_index_sequence<sizeof ... (Args)>{}, entries, count, ptr);
	}, &data);

	return result;
}

}
